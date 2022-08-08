# This file is part of the Luau programming language and is licensed under MIT License; see LICENSE.txt for details
if (EXT_PLATFORM_STRING)
    include(EXTLuau.cmake)
    return()
endif ()

cmake_minimum_required(VERSION 3.0)

option(LUAU_BUILD_CLI "Build CLI" ON)
option(LUAU_BUILD_WEB "Build Web module" OFF)
option(LUAU_WERROR "Warnings as errors" OFF)
option(LUAU_STATIC_CRT "Link with the static CRT (/MT)" OFF)
option(LUAU_EXTERN_C "Use extern C for all APIs" OFF)

if (LUAU_STATIC_CRT)
    cmake_minimum_required(VERSION 3.15)
    cmake_policy(SET CMP0091 NEW)
    set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")
endif ()

project(Luau LANGUAGES CXX C)
add_library(Luau.Common INTERFACE)
add_library(Luau.Ast STATIC)
add_library(Luau.Compiler STATIC)
add_library(Luau.Analysis STATIC)
add_library(Luau.CodeGen STATIC)
add_library(Serene.Compiler SHARED)

if (LUAU_BUILD_CLI)
    add_executable(Luau.Repl.CLI)
    add_executable(Luau.Analyze.CLI)
    add_executable(Luau.Ast.CLI)

    # This also adds target `name` on Linux/macOS and `name.exe` on Windows
    set_target_properties(Luau.Repl.CLI PROPERTIES OUTPUT_NAME luau)
    set_target_properties(Luau.Analyze.CLI PROPERTIES OUTPUT_NAME luau-analyze)
    set_target_properties(Luau.Ast.CLI PROPERTIES OUTPUT_NAME luau-ast)
endif ()


include(Sources.cmake)

target_include_directories(Luau.Common INTERFACE Common/include)

target_compile_features(Luau.Ast PUBLIC cxx_std_17)
target_include_directories(Luau.Ast PUBLIC Ast/include)
target_link_libraries(Luau.Ast PUBLIC Luau.Common)

target_compile_features(Luau.Compiler PUBLIC cxx_std_17)
target_include_directories(Luau.Compiler PUBLIC Compiler/include)
target_link_libraries(Luau.Compiler PUBLIC Luau.Ast)

target_compile_features(Luau.Analysis PUBLIC cxx_std_17)
target_include_directories(Luau.Analysis PUBLIC Analysis/include)
target_link_libraries(Luau.Analysis PUBLIC Luau.Ast)

target_compile_features(Luau.CodeGen PRIVATE cxx_std_17)
target_include_directories(Luau.CodeGen PUBLIC CodeGen/include)
target_link_libraries(Luau.CodeGen PUBLIC Luau.Common)


target_compile_features(Serene.Compiler PUBLIC cxx_std_17)
target_include_directories(Serene.Compiler PRIVATE Analysis/include Compiler/include Ast/Compiler -static)
target_link_libraries(Serene.Compiler PRIVATE Luau.Analysis Luau.Compiler Luau.Ast -static)


set(LUAU_OPTIONS)

if (MSVC)
    list(APPEND LUAU_OPTIONS /D_CRT_SECURE_NO_WARNINGS) # We need to use the portable CRT functions.
    list(APPEND LUAU_OPTIONS /MP) # Distribute single project compilation across multiple cores
else ()
    list(APPEND LUAU_OPTIONS -Wall) # All warnings
endif ()

if (CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
    # Some gcc versions treat var in `if (type var = val)` as unused
    # Some gcc versions treat variables used in constexpr if blocks as unused
    list(APPEND LUAU_OPTIONS -Wno-unused)
endif ()

if(MSVC)
    target_link_options(Luau.Ast INTERFACE /NATVIS:${CMAKE_CURRENT_SOURCE_DIR}/tools/natvis/Ast.natvis)
    target_link_options(Luau.Analysis INTERFACE /NATVIS:${CMAKE_CURRENT_SOURCE_DIR}/tools/natvis/Analysis.natvis)
    target_link_options(Luau.CodeGen INTERFACE /NATVIS:${CMAKE_CURRENT_SOURCE_DIR}/tools/natvis/CodeGen.natvis)
endif()

# make .natvis visible inside the solution
if(MSVC_IDE)
    target_sources(Luau.Ast PRIVATE tools/natvis/Ast.natvis)
    target_sources(Luau.Analysis PRIVATE tools/natvis/Analysis.natvis)
    target_sources(Luau.CodeGen PRIVATE tools/natvis/CodeGen.natvis)
    target_sources(Luau.VM PRIVATE tools/natvis/VM.natvis)
endif()

# Enabled in CI; we should be warning free on our main compiler versions but don't guarantee being warning free everywhere
if (LUAU_WERROR)
    if (MSVC)
        list(APPEND LUAU_OPTIONS /WX) # Warnings are errors
    else ()
        list(APPEND LUAU_OPTIONS -Werror) # Warnings are errors
    endif ()
endif ()

target_compile_options(Luau.Ast PRIVATE ${LUAU_OPTIONS})
target_compile_options(Luau.Analysis PRIVATE ${LUAU_OPTIONS})
target_compile_options(Luau.CodeGen PRIVATE ${LUAU_OPTIONS})

if (LUAU_EXTERN_C)
    target_compile_definitions(Luau.Compiler PUBLIC LUACODE_API=extern\"C\")
endif ()

