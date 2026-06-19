cmake_minimum_required(VERSION 3.20)

# ============================================================================
# PyLua / Lua Windows Build System
# ============================================================================

project(
        PyLua
        VERSION 1.0.0
        LANGUAGES C
)

# ============================================================================
# Compiler Configuration
# ============================================================================

set(CMAKE_C_STANDARD 99)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_C_EXTENSIONS OFF)

# ============================================================================
# Build Directories
# ============================================================================

set(BUILD_DATA_DIR "${CMAKE_BINARY_DIR}/data")

# ============================================================================
# Source Files
# ============================================================================

set(PYLUA_SOURCES
        ${CMAKE_CURRENT_SOURCE_DIR}/src/core/lapi.c
        ${CMAKE_CURRENT_SOURCE_DIR}/src/parser/lcode.c
        ${CMAKE_CURRENT_SOURCE_DIR}/src/utils/lctype.c
        ${CMAKE_CURRENT_SOURCE_DIR}/src/tools/ldebug.c
        ${CMAKE_CURRENT_SOURCE_DIR}/src/core/ldo.c
        ${CMAKE_CURRENT_SOURCE_DIR}/src/bytecode/ldump.c
        ${CMAKE_CURRENT_SOURCE_DIR}/src/runtime/lfunc.c
        ${CMAKE_CURRENT_SOURCE_DIR}/src/memory/lgc.c
        ${CMAKE_CURRENT_SOURCE_DIR}/src/parser/llex.c
        ${CMAKE_CURRENT_SOURCE_DIR}/src/memory/lmem.c
        ${CMAKE_CURRENT_SOURCE_DIR}/src/runtime/lobject.c
        ${CMAKE_CURRENT_SOURCE_DIR}/src/bytecode/lopcodes.c
        ${CMAKE_CURRENT_SOURCE_DIR}/src/parser/lparser.c
        ${CMAKE_CURRENT_SOURCE_DIR}/src/lstate.c
        ${CMAKE_CURRENT_SOURCE_DIR}/src/bytecode/lstring.c
        ${CMAKE_CURRENT_SOURCE_DIR}/src/bytecode/ltable.c
        ${CMAKE_CURRENT_SOURCE_DIR}/src/parser/ltm.c
        ${CMAKE_CURRENT_SOURCE_DIR}/src/parser/lundump.c
        ${CMAKE_CURRENT_SOURCE_DIR}/src/runtime/lvm.c
        ${CMAKE_CURRENT_SOURCE_DIR}/src/bytecode/lzio.c
        ${CMAKE_CURRENT_SOURCE_DIR}/src/libraries/lauxlib.c
        ${CMAKE_CURRENT_SOURCE_DIR}/src/libraries/lbaselib.c
        ${CMAKE_CURRENT_SOURCE_DIR}/src/libraries/ldblib.c
        ${CMAKE_CURRENT_SOURCE_DIR}/src/libraries/liolib.c
        ${CMAKE_CURRENT_SOURCE_DIR}/src/libraries/lmathlib.c
        ${CMAKE_CURRENT_SOURCE_DIR}/src/libraries/loslib.c
        ${CMAKE_CURRENT_SOURCE_DIR}/src/libraries/ltablib.c
        ${CMAKE_CURRENT_SOURCE_DIR}/src/libraries/lstrlib.c
        ${CMAKE_CURRENT_SOURCE_DIR}/src/libraries/lutf8lib.c
        ${CMAKE_CURRENT_SOURCE_DIR}/src/libraries/loadlib.c
        ${CMAKE_CURRENT_SOURCE_DIR}/src/libraries/lcorolib.c
        ${CMAKE_CURRENT_SOURCE_DIR}/src/libraries/lveclib.c
        ${CMAKE_CURRENT_SOURCE_DIR}/src/libraries/lhashlib.c
        ${CMAKE_CURRENT_SOURCE_DIR}/src/linit.c
)


if (WIN32)
    list(APPEND PYLUA_SOURCES
            ${CMAKE_CURRENT_SOURCE_DIR}/src/libraries/lguilib.c
            ${CMAKE_CURRENT_SOURCE_DIR}/src/libraries/lrequestlib.c
    )
else ()
    list(APPEND PYLUA_SOURCES
            ${CMAKE_CURRENT_SOURCE_DIR}/src/libraries/lguilib_stub.c
            ${CMAKE_CURRENT_SOURCE_DIR}/src/libraries/lrequestlib_stub.c
    )
endif ()

set(PYLUA_MAIN
        ${CMAKE_CURRENT_SOURCE_DIR}/src/lua.c
)

# ============================================================================
# Static Library
# ============================================================================

add_library(
        pylua_static
        STATIC
        ${PYLUA_SOURCES}
)

target_include_directories(
        pylua_static
        PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/include
)

if (WIN32)
    target_compile_definitions(
            pylua_static
            PRIVATE
            WIN32
            _WINDOWS
            PYLUA_WINDOWS
    )
endif ()

if (MSVC)
    target_compile_options(
            pylua_static
            PRIVATE
            /W4
            /permissive-
    )
else ()
    target_compile_options(
            pylua_static
            PRIVATE
            -Wall
            -Wextra
            -Wshadow
            -Wconversion
            -Wundef
            -Wdouble-promotion
            -Wwrite-strings
    )
endif ()

# ============================================================================
# Executable
# ============================================================================

add_executable(
        pylua
        ${PYLUA_MAIN}
)

if (WIN32)
    target_link_libraries(
            pylua
            PRIVATE
            pylua_static
            user32
            gdi32
            comctl32
            winhttp
    )
elseif (ANDROID)
    target_link_libraries(
            pylua
            PRIVATE
            pylua_static
            log
            android
    )
else ()
    target_link_libraries(
            pylua
            PRIVATE
            pylua_static
            m
            dl
    )
endif ()

target_include_directories(
        pylua
        PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/include
)

# ============================================================================
# Output Directories
# ============================================================================

set_target_properties(
        pylua
        PROPERTIES
        RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin"
)

set_target_properties(
        pylua_static
        PROPERTIES
        ARCHIVE_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/lib"
)

# ============================================================================
# Data Directory
# ============================================================================

file(
        MAKE_DIRECTORY
        "${BUILD_DATA_DIR}"
)

# ============================================================================
# Build Information
# ============================================================================

message(STATUS "================================")
message(STATUS "PyLua Build Configuration")
message(STATUS "================================")
message(STATUS "Source Dir : ${CMAKE_SOURCE_DIR}")
message(STATUS "Binary Dir : ${CMAKE_BINARY_DIR}")
message(STATUS "Executable : ${CMAKE_BINARY_DIR}/bin/pylua${CMAKE_EXECUTABLE_SUFFIX}")
message(STATUS "Library    : ${CMAKE_BINARY_DIR}/lib")
message(STATUS "Data Dir   : ${BUILD_DATA_DIR}")
message(STATUS "================================")
