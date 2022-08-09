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
#include "ByteCodeWriter.h"

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

static bool compileFile(const std::string &name, const std::string &output_file) {
    std::optional<std::string> source = readFile(name);
    if (!source) {
        fprintf(stderr, "Error opening %s\n", name.c_str());
        return false;
    }

    try {
        Luau::BytecodeBuilder bcb;
        Luau::compileOrThrow(bcb, *source, copts());
        writeByteCode(output_file.c_str(), bcb.getBytecode().data(),bcb.getBytecode().size());

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

#if defined(_WIN32) || defined(__WIN32__)
#    define  MYLIB_EXPORT extern "C" __declspec(dllexport)
#elif defined(linux) || defined(__linux)
# define MYLIB_EXPORT
#endif

MYLIB_EXPORT bool CompileFile(const char *source_file,const char *output_file) {

    /*

        Set our current working directory

        this is so that, we can find files easily.

     */

    chdir(getParentPath(source_file)->c_str());

    /*

        Command line args

     */

    ReportFormat format = ReportFormat::Default;
    Luau::Mode mode = Luau::Mode::Nonstrict;
    bool annotate = false;

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

        bool failed = false;

        if (analyzeFile(frontend, source_file, format, annotate)) {
            std::cout << "Analyzed files [OK]\n";
        } else {
            fprintf(stderr, "Analyzed %s [FAILED]\n", source_file);
            failed = true;
        }

        if (!configResolver.configErrors.empty()) {
            for (const auto &pair: configResolver.configErrors)
                fprintf(stderr, "%s: %s\n", pair.first.c_str(), pair.second.c_str());
        }

        if (failed) {
            fprintf(stderr, "Compilation terminated.  [ERROR]");
            return false;
        }

    }

    /*

        Now compiling scripts

     */

    std::cout << "Starting Script Compilation..." << std::endl;


    if (!compileFile(source_file,output_file)) {
        fprintf(stderr, "Compilation Terminated. [ERROR]");
        return false;
    }

    std::cout << "Compilation Successful. [SUCCESS]\n" << std::endl;
    return true;
}

