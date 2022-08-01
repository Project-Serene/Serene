/*

    C++ Headers

 */

#include <iostream>
#include <locale>
#include <optional>
#include <string>
#include <functional>


/*

    Luau Analysis Headers

*/

#include "Luau/ModuleResolver.h"
#include "Luau/TypeInfer.h"
#include "Luau/BuiltinDefinitions.h"
#include "Luau/Frontend.h"
#include "Luau/TypeAttach.h"
#include "Luau/Transpiler.h"

#include "FileUtils.h"
#include "Flags.h"

LUAU_FASTFLAG(DebugLuauTimeTracing)
LUAU_FASTFLAG(LuauTypeMismatchModuleNameResolution)


/*

    Luau Compiler Headers

*/

#include "Luau/Compiler.h"
#include "Luau/BytecodeBuilder.h"
#include "Luau/JsonEncoder.h"


#ifdef _WIN32

#include <io.h>
#include <fcntl.h>

#endif

/*
 
    Compile Options
 
 */

struct GlobalOptions {
    int optimizationLevel = 1;
    int debugLevel = 1;
} globalOptions;

static Luau::CompileOptions copts() {
    Luau::CompileOptions result = {};
    result.optimizationLevel = globalOptions.optimizationLevel;
    result.debugLevel = globalOptions.debugLevel;
    result.coverageLevel = 0;
    return result;
}

/*

    Error Reporting

 */

enum class ReportFormat {
    Default,
    Luacheck,
    Gnu,
};


static void report(const char *name, const Luau::Location &location, const char *type, const char *message) {
    fprintf(stderr, "%s(%d,%d): %s: %s\n", name, location.begin.line + 1, location.begin.column + 1, type, message);
}

static void
report(ReportFormat format, const char *name, const Luau::Location &loc, const char *type, const char *message) {
    switch (format) {
        case ReportFormat::Default:
            fprintf(stderr, "%s(%d,%d): %s: %s\n", name, loc.begin.line + 1, loc.begin.column + 1, type, message);
            break;

        case ReportFormat::Luacheck: {
            // Note: luacheck's end column is inclusive but our end column is exclusive
            // In addition, luacheck doesn't support multi-line messages, so if the error is multiline we'll fake end column as 100 and hope for the best
            int columnEnd = (loc.begin.line == loc.end.line) ? loc.end.column : 100;

            // Use stdout to match luacheck behavior
            fprintf(stdout, "%s:%d:%d-%d: (W0) %s: %s\n", name, loc.begin.line + 1, loc.begin.column + 1, columnEnd,
                    type, message);
            break;
        }

        case ReportFormat::Gnu:
            // Note: GNU end column is inclusive but our end column is exclusive
            fprintf(stderr, "%s:%d.%d-%d.%d: %s: %s\n", name, loc.begin.line + 1, loc.begin.column + 1,
                    loc.end.line + 1, loc.end.column, type, message);
            break;
    }
}

static void reportError(const Luau::Frontend &frontend, ReportFormat format, const Luau::TypeError &error) {
    std::string humanReadableName = frontend.fileResolver->getHumanReadableModuleName(error.moduleName);

    if (const auto *syntaxError = Luau::get_if<Luau::SyntaxError>(&error.data))
        report(format, humanReadableName.c_str(), error.location, "SyntaxError", syntaxError->message.c_str());
    else if (FFlag::LuauTypeMismatchModuleNameResolution)
        report(format, humanReadableName.c_str(), error.location, "TypeError",
               Luau::toString(error, Luau::TypeErrorToStringOptions{frontend.fileResolver}).c_str());
    else
        report(format, humanReadableName.c_str(), error.location, "TypeError", Luau::toString(error).c_str());
}


static void reportError(const char *name, const Luau::ParseError &error) {
    report(name, error.getLocation(), "SyntaxError", error.what());
}

static void reportError(const char *name, const Luau::CompileError &error) {
    report(name, error.getLocation(), "CompileError", error.what());
}

static void reportWarning(ReportFormat format, const char *name, const Luau::LintWarning &warning) {
    report(format, name, warning.location, Luau::LintWarning::getName(warning.code), warning.text.c_str());
}

/*

    Analysing Lua Files

 */

static bool analyzeFile(Luau::Frontend &frontend, const char *name, ReportFormat format, bool annotate) {
    Luau::CheckResult cr;

    if (frontend.isDirty(name))
        cr = frontend.check(name);

    if (!frontend.getSourceModule(name)) {
        fprintf(stderr, "Error opening %s\n", name);
        return false;
    }

    for (auto &error: cr.errors)
        reportError(frontend, format, error);

    Luau::LintResult lr = frontend.lint(name);

    std::string humanReadableName = frontend.fileResolver->getHumanReadableModuleName(name);
    for (auto &error: lr.errors)
        reportWarning(format, humanReadableName.c_str(), error);
    for (auto &warning: lr.warnings)
        reportWarning(format, humanReadableName.c_str(), warning);

    if (annotate) {
        Luau::SourceModule *sm = frontend.getSourceModule(name);
        Luau::ModulePtr m = frontend.moduleResolver.getModule(name);

        Luau::attachTypeData(*sm, *m);

        std::string annotated = Luau::transpileWithTypes(*sm->root);

        printf("%s", annotated.c_str());
    }

    return cr.errors.empty() && lr.errors.empty();
}

/*

    File resolver

 */


struct CliFileResolver : Luau::FileResolver {
    std::optional<Luau::SourceCode> readSource(const Luau::ModuleName &name) override {
        Luau::SourceCode::Type sourceType;
        std::optional<std::string> source = readFile(name);

        sourceType = Luau::SourceCode::Module;

        if (!source)
            return std::nullopt;

        return Luau::SourceCode{*source, sourceType};
    }

    std::optional<Luau::ModuleInfo> resolveModule(const Luau::ModuleInfo *context, Luau::AstExpr *node) override {
        if (auto *expr = node->as<Luau::AstExprConstantString>()) {
            Luau::ModuleName name = std::string(expr->value.data, expr->value.size) + ".luau";
            if (!readFile(name)) {
                // fall back to .lua if a module with .luau doesn't exist
                name = std::string(expr->value.data, expr->value.size) + ".lua";
            }

            return {{name}};
        }

        return std::nullopt;
    }

    [[nodiscard]] std::string getHumanReadableModuleName(const Luau::ModuleName &name) const override {
        return name;
    }
};

/*

    Config Resolver

 */

struct CliConfigResolver : Luau::ConfigResolver {
    Luau::Config defaultConfig;

    mutable std::unordered_map<std::string, Luau::Config> configCache;
    mutable std::vector<std::pair<std::string, std::string>> configErrors;

    explicit CliConfigResolver(Luau::Mode mode) {
        defaultConfig.mode = mode;
    }

    const Luau::Config &getConfig(const Luau::ModuleName &name) const override {
        std::optional<std::string> path = getParentPath(name);
        if (!path)
            return defaultConfig;

        return readConfigRec(*path);
    }

    const Luau::Config &readConfigRec(const std::string &path) const {
        auto it = configCache.find(path);
        if (it != configCache.end())
            return it->second;

        std::optional<std::string> parent = getParentPath(path);
        Luau::Config result = parent ? readConfigRec(*parent) : defaultConfig;

        std::string configPath = joinPaths(path, Luau::kConfigName);

        if (std::optional<std::string> contents = readFile(configPath)) {
            std::optional<std::string> error = Luau::parseConfig(*contents, result);
            if (error)
                configErrors.emplace_back(configPath, *error);
        }

        return configCache[path] = result;
    }
};

/*

    Compiling Lua Files

 */

static bool compileFile(const std::string& name) {
    std::optional<std::string> source = readFile(name);
    if (!source) {
        fprintf(stderr, "Error opening %s\n", name.c_str());
        return false;
    }

    try {
        Luau::BytecodeBuilder bcb;
        Luau::compileOrThrow(bcb, *source, copts());
        writeFile(name+".bin", bcb.getBytecode().data(), 1, bcb.getBytecode().size());

        return true;
    }
    catch (Luau::ParseErrors &e) {
        for (auto &error: e.getErrors())
            reportError(name.c_str(), error);
        return false;
    }
    catch (Luau::CompileError &e) {
        reportError(name.c_str(), e);
        return false;
    }
}

int main(int argc, char **argv) {

    const std::vector<std::string> files = getSourceFiles(argc, argv);

    if (files.empty()) {
        std::cout << "fatal error: no source files given" << std::endl;
        return 1;
    }

    /*

        Command line args

     */

    ReportFormat format = ReportFormat::Default;
    Luau::Mode mode = Luau::Mode::Nonstrict;
    bool annotate = false;

    for (int i = 1; i < argc; ++i) {
        if (argv[i][0] != '-')
            continue;

        if (strcmp(argv[i], "--formatter=plain") == 0)
            format = ReportFormat::Luacheck;
        else if (strcmp(argv[i], "--formatter=gnu") == 0)
            format = ReportFormat::Gnu;
        else if (strcmp(argv[i], "--mode=strict") == 0)
            mode = Luau::Mode::Strict;
        else if (strcmp(argv[i], "--annotate") == 0)
            annotate = true;
        else if (strcmp(argv[i], "--timetrace") == 0)
            FFlag::DebugLuauTimeTracing.value = true;
        else if (strncmp(argv[i], "--fflags=", 9) == 0)
            setLuauFlags(argv[i] + 9);
    }

    /*

        Script Analysis

        Checks and verifies the scripts before
        sending for compilation.

        Performs all sorts of advance type analysis'

     */

    std::cout << "Starting Script Analysis..." << std::endl;
    {

        Luau::FrontendOptions frontendOptions;
        frontendOptions.retainFullTypeGraphs = annotate;

        CliFileResolver fileResolver;
        CliConfigResolver configResolver(mode);
        Luau::Frontend frontend(&fileResolver, &configResolver, frontendOptions);

        Luau::registerBuiltinTypes(frontend.typeChecker);
        Luau::freeze(frontend.typeChecker.globalTypes);

        int failed = 0;

        for (const std::string &path: files) {
            int f = analyzeFile(frontend, path.c_str(), format, annotate);
            if (f)
                std::cout << "Analyzed " << path << " [OK]\n";
            else
                fprintf(stderr, "Analyzed %s [FAILED]\n", path.c_str());
            failed += !f;
        }

        if (!configResolver.configErrors.empty()) {
            failed += int(configResolver.configErrors.size());

            for (const auto &pair: configResolver.configErrors)
                fprintf(stderr, "%s: %s\n", pair.first.c_str(), pair.second.c_str());
        }

        if (failed) {
            fprintf(stderr, "Compilation terminated.  %i files failed script analysis.", failed);
            return 1;
        }

    }

    /*

        Now compiling scripts

     */

    std::cout << "Analyzed files\nStarting Script Compilation..." << std::endl;

    {

        int failed = 0;

        for (const std::string &path: files)
            failed += !compileFile(path);

        if (failed) {
            fprintf(stderr, "Compilation Terminated.");
            return 1;
        }

    }

    std::cout << "Compilation Successful." << std::endl;
    return 0;
}

