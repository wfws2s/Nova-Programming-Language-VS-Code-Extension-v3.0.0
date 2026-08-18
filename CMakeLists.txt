cmake_minimum_required(VERSION 3.20)

project(nova
    VERSION 0.2.2
    DESCRIPTION "NOVA programming language interpreter"
    LANGUAGES CXX RC
)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

if(NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
    set(CMAKE_BUILD_TYPE Release CACHE STRING "Build type" FORCE)
    set_property(CACHE CMAKE_BUILD_TYPE PROPERTY STRINGS Debug Release RelWithDebInfo MinSizeRel)
endif()

# Core static library (lexer, AST, parser, runtime, interpreter, diagnostics)
add_library(nova_core STATIC
    src/lexer/token.cpp
    src/lexer/lexer_error.cpp
    src/lexer/lexer.cpp
    src/ast/ast.cpp
    src/parser/parser.cpp
    src/runtime/value.cpp
    src/runtime/environment.cpp
    src/runtime/builtins.cpp
    src/interpreter/interpreter.cpp
    src/interpreter/diagnostics.cpp
)

target_include_directories(nova_core PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/src)

target_compile_options(nova_core PRIVATE
    $<$<CXX_COMPILER_ID:GNU,Clang>:-Wall -Wextra -Wpedantic>
    $<$<CXX_COMPILER_ID:MSVC>:/W4>
)

# Windows resource file (icon + version info)
if(WIN32)
    set(RC_FILE ${CMAKE_CURRENT_SOURCE_DIR}/resource.rc)
    set_source_files_properties(${RC_FILE} PROPERTIES
        COMPILE_FLAGS "--include-dir=${CMAKE_CURRENT_SOURCE_DIR}"
    )
endif()

# Main interpreter executable (Console Application)
add_executable(nova src/main.cpp ${RC_FILE})
set_target_properties(nova PROPERTIES OUTPUT_NAME "nova")
target_link_libraries(nova PRIVATE nova_core)

target_compile_options(nova PRIVATE
    $<$<CXX_COMPILER_ID:GNU,Clang>:-Wall -Wextra -Wpedantic>
    $<$<CXX_COMPILER_ID:MSVC>:/W4>
)

# Tests
enable_testing()
add_subdirectory(tests)

# Installer (Windows GUI Application)
if(WIN32)
    add_subdirectory(installer)
endif()
