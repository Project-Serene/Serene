# This file is part of the Luau programming language and is licensed under MIT License; see LICENSE.txt for details
if(EXT_PLATFORM_STRING)
    include(EXTLuau.cmake)
    return()
endif()

cmake_minimum_required(VERSION 3.0)

option(LUAU_BUILD_CLI "Build CLI" ON)
option(LUAU_BUILD_TESTS "Build tests" ON)
option(LUAU_BUILD_WEB "Build Web module" ON)
option(LUAU_WERROR "Warnings as errors" OFF)
option(LUAU_STATIC_CRT "Link with the static CRT (/MT)" OFF)
option(LUAU_EXTERN_C "Use extern C for all APIs" OFF)

option(SERENE_BUILD_TOOLCHAIN "Build Toolchain" ON)


if(LUAU_STATIC_CRT)
    cmake_minimum_required(VERSION 3.15)
    cmake_policy(SET CMP0091 NEW)
    set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")
endif()

project(Luau LANGUAGES CXX C)
add_library(Luau.Common INTERFACE)
add_library(Luau.Ast STATIC)
add_library(Luau.Compiler STATIC)
add_library(Luau.Analysis STATIC)
add_library(Luau.CodeGen STATIC)
add_library(Luau.VM STATIC)
add_library(isocline STATIC)
add_library(Serene.ToolChain STATIC)

if(LUAU_BUILD_CLI)
    add_executable(Luau.Repl.CLI)
    add_executable(Luau.Analyze.CLI)
    add_executable(Luau.Ast.CLI)

    # This also adds target `name` on Linux/macOS and `name.exe` on Windows
    set_target_properties(Luau.Repl.CLI PROPERTIES OUTPUT_NAME luau)
    set_target_properties(Luau.Analyze.CLI PROPERTIES OUTPUT_NAME luau-analyze)
    set_target_properties(Luau.Ast.CLI PROPERTIES OUTPUT_NAME luau-ast)
endif()

if(SERENE_BUILD_TOOLCHAIN)

    set(CMAKE_FIND_LIBRARY_SUFFIXES ".a")
    set(BUILD_SHARED_LIBS OFF)
    set(CMAKE_EXE_LINKER_FLAGS "-static")

    add_executable(Serene.ToolChain.CLI)

    # This also adds target `name` on Linux/macOS and `name.exe` on Windows
    set_target_properties(Serene.ToolChain.CLI PROPERTIES OUTPUT_NAME Serene)
endif()

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

target_compile_features(Luau.VM PRIVATE cxx_std_11)
target_include_directories(Luau.VM PUBLIC src/VM/include)
target_link_libraries(Luau.VM PUBLIC Luau.Common)

target_include_directories(isocline PUBLIC extern/isocline/include)

#
# Links Libraries for Serene Tool Chain
#
target_compile_features(Serene.ToolChain PRIVATE cxx_std_17)
target_include_directories(Serene.ToolChain PUBLIC Analysis/include)
target_include_directories(Serene.ToolChain PUBLIC Compiler/include)
target_include_directories(Serene.ToolChain PUBLIC Ast/include)
target_link_libraries(Serene.ToolChain PUBLIC Luau.Common)

set(LUAU_OPTIONS)

if(MSVC)
    list(APPEND LUAU_OPTIONS /D_CRT_SECURE_NO_WARNINGS) # We need to use the portable CRT functions.
    list(APPEND LUAU_OPTIONS /MP) # Distribute single project compilation across multiple cores
else()
    list(APPEND LUAU_OPTIONS -Wall) # All warnings
endif()

if (CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
    # Some gcc versions treat var in `if (type var = val)` as unused
    # Some gcc versions treat variables used in constexpr if blocks as unused
    list(APPEND LUAU_OPTIONS -Wno-unused)
endif()

# Enabled in CI; we should be warning free on our main compiler versions but don't guarantee being warning free everywhere
if(LUAU_WERROR)
    if(MSVC)
        list(APPEND LUAU_OPTIONS /WX) # Warnings are errors
    else()
        list(APPEND LUAU_OPTIONS -Werror) # Warnings are errors
    endif()
endif()


set(ISOCLINE_OPTIONS)

if (NOT MSVC)
    list(APPEND ISOCLINE_OPTIONS -Wno-unused-function)
endif()

target_compile_options(Luau.Ast PRIVATE ${LUAU_OPTIONS})
target_compile_options(Luau.Analysis PRIVATE ${LUAU_OPTIONS})
target_compile_options(Luau.CodeGen PRIVATE ${LUAU_OPTIONS})
target_compile_options(Luau.VM PRIVATE ${LUAU_OPTIONS})
target_compile_options(isocline PRIVATE ${LUAU_OPTIONS} ${ISOCLINE_OPTIONS})
target_compile_options(Serene.ToolChain PRIVATE ${LUAU_OPTIONS})

if(LUAU_EXTERN_C)
    # enable extern "C" for VM (lua.h, lualib.h) and Compiler (luacode.h) to make Luau friendlier to use from non-C++ languages
    # note that we enable LUA_USE_LONGJMP=1 as well; otherwise functions like luaL_error will throw C++ exceptions, which can't be done from extern "C" functions
    target_compile_definitions(Luau.VM PUBLIC LUA_USE_LONGJMP=1)
    target_compile_definitions(Luau.VM PUBLIC LUA_API=extern\"C\")
    target_compile_definitions(Luau.Compiler PUBLIC LUACODE_API=extern\"C\")
endif()

if (MSVC AND MSVC_VERSION GREATER_EQUAL 1924)
    # disable partial redundancy elimination which regresses interpreter codegen substantially in VS2022:
    # https://developercommunity.visualstudio.com/t/performance-regression-on-a-complex-interpreter-lo/1631863
    set_source_files_properties(src/VM/lvmexecute.cpp PROPERTIES COMPILE_FLAGS /d2ssa-pre-)
endif()

if(MSVC AND LUAU_BUILD_CLI)
    # the default stack size that MSVC linker uses is 1 MB; we need more stack space in Debug because stack frames are larger
    set_target_properties(Luau.Analyze.CLI PROPERTIES LINK_FLAGS_DEBUG /STACK:2097152)
    set_target_properties(Luau.Repl.CLI PROPERTIES LINK_FLAGS_DEBUG /STACK:2097152)
endif()

if(MSVC AND Serene.ToolChain.CLI)
    set_target_properties(Serene.ToolChain.CLI PROPERTIES LINK_FLAGS_DEBUG /STACK:2097152)
endif()

# embed .natvis inside the library debug information
if(MSVC)
    target_link_options(Luau.Ast INTERFACE /NATVIS:${CMAKE_CURRENT_SOURCE_DIR}/tools/natvis/Ast.natvis)
    target_link_options(Luau.Analysis INTERFACE /NATVIS:${CMAKE_CURRENT_SOURCE_DIR}/tools/natvis/Analysis.natvis)
    target_link_options(Luau.CodeGen INTERFACE /NATVIS:${CMAKE_CURRENT_SOURCE_DIR}/tools/natvis/CodeGen.natvis)
    target_link_options(Luau.VM INTERFACE /NATVIS:${CMAKE_CURRENT_SOURCE_DIR}/tools/natvis/VM.natvis)
    target_link_options(Serene.ToolChain INTERFACE /NATVIS:${CMAKE_CURRENT_SOURCE_DIR}/tools/natvis/Analysis.natvis)
endif()

# make .natvis visible inside the solution
if(MSVC_IDE)
    target_sources(Luau.Ast PRIVATE tools/natvis/Ast.natvis)
    target_sources(Luau.Analysis PRIVATE tools/natvis/Analysis.natvis)
    target_sources(Luau.CodeGen PRIVATE tools/natvis/CodeGen.natvis)
    target_sources(Luau.VM PRIVATE tools/natvis/VM.natvis)
    target_sources(Serene.ToolChain PRIVATE tools/natvis/Analysis.natvis)
endif()

if(LUAU_BUILD_CLI)
    target_compile_options(Luau.Repl.CLI PRIVATE ${LUAU_OPTIONS})
    target_compile_options(Luau.Analyze.CLI PRIVATE ${LUAU_OPTIONS})
    target_compile_options(Luau.Ast.CLI PRIVATE ${LUAU_OPTIONS})

    target_include_directories(Luau.Repl.CLI PRIVATE extern extern/isocline/include)

    target_link_libraries(Luau.Repl.CLI PRIVATE Luau.Compiler Luau.VM isocline)

    if(UNIX)
        find_library(LIBPTHREAD pthread)
        if (LIBPTHREAD)
            target_link_libraries(Luau.Repl.CLI PRIVATE pthread)
        endif()
    endif()

    target_link_libraries(Luau.Analyze.CLI PRIVATE Luau.Analysis)

    target_link_libraries(Luau.Ast.CLI PRIVATE Luau.Ast Luau.Analysis)
endif()

if(SERENE_BUILD_TOOLCHAIN)

    target_compile_options(Serene.ToolChain.CLI PRIVATE ${LUA_OPTIONS})
    target_include_directories(Serene.ToolChain.CLI PRIVATE
            Analysis/include
            Compiler/include
            Ast/include
            Common/
            include/Luau
            include/
            )

    target_link_libraries(Serene.ToolChain.CLI PRIVATE
            Luau.Compiler Luau.Ast Luau.Analysis Luau.Common Serene.ToolChain)

    if(UNIX)
        find_library(LIBPTHREAD pthread)
        if (LIBPTHREAD)
            target_link_libraries(Serene.ToolChain.CLI PRIVATE pthread)
        endif()
    endif()

endif()
