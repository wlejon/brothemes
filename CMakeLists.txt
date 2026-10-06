cmake_minimum_required(VERSION 3.24)
project(brothemes LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# Coverage. Scoped to a top-level build: a consumer that add_subdirectory()s
# brothemes has no business having its own build instrumented from in here.
option(BROTHEMES_COVERAGE "Instrument the build for gcov coverage (GCC/Clang)" OFF)
if(BROTHEMES_COVERAGE AND PROJECT_IS_TOP_LEVEL)
    if(MSVC)
        message(FATAL_ERROR "BROTHEMES_COVERAGE requires GCC or Clang (gcov instrumentation)")
    endif()
    add_compile_options(--coverage -O0 -g)
    add_link_options(--coverage)
endif()

if(MSVC)
    add_compile_options(/MP /permissive- /utf-8)
endif()

option(BROTHEMES_BUILD_TESTS "Build the brothemes test suite" ${PROJECT_IS_TOP_LEVEL})
option(BROTHEMES_BUILD_TOOLS "Build brothemes_pack (regenerates the scheme oracle archive)" ${PROJECT_IS_TOP_LEVEL})

add_subdirectory(src)

# --- Standalone Bronze JavaScript API ---
option(BROTHEMES_ENABLE_API "Build Bronze JavaScript API" ON)
if(BROTHEMES_ENABLE_API)
    if(NOT TARGET bronze_runtime_shared)
        set(BRONZE_BUILD_SHARED_RUNTIME ON CACHE BOOL "" FORCE)
        set(BRONZE_BUILD_TESTS OFF CACHE BOOL "" FORCE)
        if(DEFINED BRONZE_DIR AND EXISTS "${BRONZE_DIR}/CMakeLists.txt")
            # Explicit BRONZE_DIR supplied
        elseif(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/../bronze/CMakeLists.txt")
            set(BRONZE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/../bronze" CACHE PATH "bronze source tree")
        elseif(EXISTS "${CMAKE_SOURCE_DIR}/../bronze/CMakeLists.txt")
            set(BRONZE_DIR "${CMAKE_SOURCE_DIR}/../bronze" CACHE PATH "bronze source tree")
        endif()

        if(NOT BRONZE_DIR OR NOT EXISTS "${BRONZE_DIR}/CMakeLists.txt")
            message(FATAL_ERROR
                "bronze not found: check it out beside this repository (../bronze, with ../brass beside it) "
                "or pass -DBRONZE_DIR=<path>.")
        endif()

        add_subdirectory("${BRONZE_DIR}" "${CMAKE_BINARY_DIR}/bronze-build" EXCLUDE_FROM_ALL)
    endif()

    add_subdirectory(src/api)
endif()

if(BROTHEMES_BUILD_TESTS OR BROTHEMES_BUILD_TOOLS)
    add_subdirectory(tools)
endif()

if(BROTHEMES_BUILD_TESTS)
    enable_testing()
    add_subdirectory(tests)
endif()
