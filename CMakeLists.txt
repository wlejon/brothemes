cmake_minimum_required(VERSION 3.24)
project(brothemes LANGUAGES CXX)

# Dependencies (cmake/bro_deps.cmake): an outer project's target wins, else the
# ../<name> working tree beside the top-level project, else the pinned commit,
# fetched at configure.
include(${CMAKE_CURRENT_SOURCE_DIR}/cmake/bro_deps.cmake)

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
# On by default only when brothemes is the top-level project. A library that
# consumes brothemes (bropty, bromux, ...) then never needs bronze checked out;
# bro, the one consumer that wants the api, forces BROTHEMES_ENABLE_API ON.
option(BROTHEMES_ENABLE_API "Build Bronze JavaScript API" ${PROJECT_IS_TOP_LEVEL})
if(BROTHEMES_ENABLE_API)
    # bronze (with brass, its code generator): an outer project's target, else
    # ../bronze beside the top-level project, else the pinned commit, fetched.
    bro_dependency(bronze GITHUB wlejon/bronze TARGET bronze_runtime_shared
                   OPTIONS BRONZE_BUILD_SHARED_RUNTIME=ON BRONZE_BUILD_TESTS=OFF)

    add_subdirectory(src/api)
endif()

if(BROTHEMES_BUILD_TESTS OR BROTHEMES_BUILD_TOOLS)
    add_subdirectory(tools)
endif()

if(BROTHEMES_BUILD_TESTS)
    enable_testing()
    add_subdirectory(tests)
endif()
