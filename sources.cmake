# Luau.Common Sources
# Note: Until 3.19, INTERFACE targets couldn't have SOURCES property set
if(NOT ${CMAKE_VERSION} VERSION_LESS "3.19")
    target_sources(Luau.Common PRIVATE
            Common/include/Luau/Common.h
            Common/include/Luau/Bytecode.h
            Common/include/Luau/ExperimentalFlags.h
            )
endif()

# Luau.Ast Sources
target_sources(Luau.Ast PRIVATE
        Ast/include/Luau/Ast.h
        Ast/include/Luau/Confusables.h
        Ast/include/Luau/DenseHash.h
        Ast/include/Luau/Lexer.h
        Ast/include/Luau/Location.h
        Ast/include/Luau/ParseOptions.h
        Ast/include/Luau/Parser.h
        Ast/include/Luau/ParseResult.h
        Ast/include/Luau/StringUtils.h
        Ast/include/Luau/TimeTrace.h

        Ast/src/Ast.cpp
        Ast/src/Confusables.cpp
        Ast/src/Lexer.cpp
        Ast/src/Location.cpp
        Ast/src/Parser.cpp
        Ast/src/StringUtils.cpp
        Ast/src/TimeTrace.cpp
        )

# Luau.Compiler Sources
target_sources(Luau.Compiler PRIVATE
        Compiler/include/Luau/BytecodeBuilder.h
        Compiler/include/Luau/Compiler.h
        Compiler/include/luacode.h

        Compiler/src/BytecodeBuilder.cpp
        Compiler/src/Compiler.cpp
        Compiler/src/Builtins/Builtins.cpp
        Compiler/src/BuiltinFolding.cpp
        Compiler/src/ConstantFolding.cpp
        Compiler/src/CostModel.cpp
        Compiler/src/TableShape.cpp
        Compiler/src/ValueTracking.cpp
        Compiler/src/lcode.cpp
        Compiler/src/Builtins/Builtins.h
        Compiler/src/BuiltinFolding.h
        Compiler/src/ConstantFolding.h
        Compiler/src/CostModel.h
        Compiler/src/TableShape.h
        Compiler/src/ValueTracking.h
        )

# Luau.CodeGen Sources
target_sources(Luau.CodeGen PRIVATE
        CodeGen/include/Luau/AssemblyBuilderX64.h
        CodeGen/include/Luau/Condition.h
        CodeGen/include/Luau/Label.h
        CodeGen/include/Luau/OperandX64.h
        CodeGen/include/Luau/RegisterX64.h

        CodeGen/src/AssemblyBuilderX64.cpp
        )

# Luau.Analysis Sources
target_sources(Luau.Analysis PRIVATE
        Analysis/include/Luau/AstQuery.h
        Analysis/include/Luau/Autocomplete.h
        Analysis/include/Luau/BuiltinDefinitions.h
        Analysis/include/Luau/Clone.h
        Analysis/include/Luau/Config.h
        Analysis/include/Luau/Constraint.h
        Analysis/include/Luau/ConstraintGraphBuilder.h
        Analysis/include/Luau/ConstraintSolver.h
        Analysis/include/Luau/ConstraintSolverLogger.h
        Analysis/include/Luau/Documentation.h
        Analysis/include/Luau/Error.h
        Analysis/include/Luau/FileResolver.h
        Analysis/include/Luau/Frontend.h
        Analysis/include/Luau/Instantiation.h
        Analysis/include/Luau/IostreamHelpers.h
        Analysis/include/Luau/JsonEncoder.h
        Analysis/include/Luau/Linter.h
        Analysis/include/Luau/LValue.h
        Analysis/include/Luau/Module.h
        Analysis/include/Luau/ModuleResolver.h
        Analysis/include/Luau/Normalize.h
        Analysis/include/Luau/Predicate.h
        Analysis/include/Luau/Quantify.h
        Analysis/include/Luau/RecursionCounter.h
        Analysis/include/Luau/RequireTracer.h
        Analysis/include/Luau/Scope.h
        Analysis/include/Luau/Substitution.h
        Analysis/include/Luau/Symbol.h
        Analysis/include/Luau/ToDot.h
        Analysis/include/Luau/TopoSortStatements.h
        Analysis/include/Luau/ToString.h
        Analysis/include/Luau/Transpiler.h
        Analysis/include/Luau/TxnLog.h
        Analysis/include/Luau/TypeArena.h
        Analysis/include/Luau/TypeAttach.h
        Analysis/include/Luau/TypeChecker2.h
        Analysis/include/Luau/TypedAllocator.h
        Analysis/include/Luau/TypeInfer.h
        Analysis/include/Luau/TypePack.h
        Analysis/include/Luau/TypeUtils.h
        Analysis/include/Luau/TypeVar.h
        Analysis/include/Luau/Unifiable.h
        Analysis/include/Luau/Unifier.h
        Analysis/include/Luau/UnifierSharedState.h
        Analysis/include/Luau/Variant.h
        Analysis/include/Luau/VisitTypeVar.h

        Analysis/src/AstQuery.cpp
        Analysis/src/Autocomplete.cpp
        Analysis/src/BuiltinDefinitions.cpp
        Analysis/src/Clone.cpp
        Analysis/src/Config.cpp
        Analysis/src/Constraint.cpp
        Analysis/src/ConstraintGraphBuilder.cpp
        Analysis/src/ConstraintSolver.cpp
        Analysis/src/ConstraintSolverLogger.cpp
        Analysis/src/Error.cpp
        Analysis/src/Frontend.cpp
        Analysis/src/Instantiation.cpp
        Analysis/src/IostreamHelpers.cpp
        Analysis/src/JsonEncoder.cpp
        Analysis/src/Linter.cpp
        Analysis/src/LValue.cpp
        Analysis/src/Module.cpp
        Analysis/src/Normalize.cpp
        Analysis/src/Quantify.cpp
        Analysis/src/RequireTracer.cpp
        Analysis/src/Scope.cpp
        Analysis/src/Substitution.cpp
        Analysis/src/Symbol.cpp
        Analysis/src/ToDot.cpp
        Analysis/src/TopoSortStatements.cpp
        Analysis/src/ToString.cpp
        Analysis/src/Transpiler.cpp
        Analysis/src/TxnLog.cpp
        Analysis/src/TypeArena.cpp
        Analysis/src/TypeAttach.cpp
        Analysis/src/TypeChecker2.cpp
        Analysis/src/TypedAllocator.cpp
        Analysis/src/TypeInfer.cpp
        Analysis/src/TypePack.cpp
        Analysis/src/TypeUtils.cpp
        Analysis/src/TypeVar.cpp
        Analysis/src/Unifiable.cpp
        Analysis/src/Unifier.cpp
        Analysis/src/EmbeddedBuiltinDefinitions.cpp
        )


if(TARGET Luau.Repl.CLI)
    # Luau.Repl.CLI Sources
    target_sources(Luau.Repl.CLI PRIVATE
            CLI/Coverage.h
            CLI/Coverage.cpp
            CLI/FileUtils.h
            CLI/FileUtils.cpp
            CLI/Flags.h
            CLI/Flags.cpp
            CLI/Profiler.h
            CLI/Profiler.cpp
            CLI/Repl.cpp
            CLI/ReplEntry.cpp)
endif()

if(TARGET Luau.Analyze.CLI)
    # Luau.Analyze.CLI Sources
    target_sources(Luau.Analyze.CLI PRIVATE
            CLI/FileUtils.h
            CLI/FileUtils.cpp
            CLI/Flags.h
            CLI/Flags.cpp
            CLI/Analyze.cpp)
endif()

if(TARGET Luau.Ast.CLI)
    target_sources(Luau.Ast.CLI PRIVATE
            CLI/Ast.cpp
            CLI/FileUtils.h
            CLI/FileUtils.cpp
            )
endif()

if (TARGET Serene.Compiler)
    target_sources(Serene.Compiler PRIVATE
            SereneCompiler/Flags.h
            SereneCompiler/Flags.cpp

            SereneCompiler/FileUtils.h
            SereneCompiler/FileUtils.cpp

            SereneCompiler/SereneCompiler.h
            SereneCompiler/SereneCompiler.cpp
            )
endif()