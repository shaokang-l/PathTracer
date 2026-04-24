cmake_minimum_required(VERSION 3.19)
project(PathTracer LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_C_STANDARD 11)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# Default to a Release build for single-config generators if the user didn't pick one.
if(NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
    set(CMAKE_BUILD_TYPE Release CACHE STRING "Build type" FORCE)
    set_property(CACHE CMAKE_BUILD_TYPE PROPERTY STRINGS Debug Release RelWithDebInfo MinSizeRel)
endif()

option(PATHTRACER_BUILD_TESTS "Build unit tests" ON)

# Add cmake modules directory
list(APPEND CMAKE_MODULE_PATH ${CMAKE_CURRENT_SOURCE_DIR}/cmake)

# ────────────────────────────────────────────────────────────────────
# MSVC / Windows quality-of-life
# ────────────────────────────────────────────────────────────────────
if(MSVC)
    # UTF-8 source + execution charset, big object files (stb_image.h etc.),
    # and exposing M_PI / M_SQRT2 / ... from <cmath>.
    add_compile_options(/utf-8 /bigobj /permissive-)
    add_compile_definitions(_USE_MATH_DEFINES NOMINMAX _CRT_SECURE_NO_WARNINGS)
endif()

# ────────────────────────────────────────────────────────────────────
# OpenMP
#
# We try find_package first (works for MSVC, GCC, Clang on Linux, and
# AppleClang when libomp is discoverable via CMake). On macOS with
# Homebrew libomp we fall back to a manual interface target so the rest
# of the build can uniformly link against `OpenMP::OpenMP_CXX`.
# ────────────────────────────────────────────────────────────────────
find_package(OpenMP QUIET)

if(OpenMP_CXX_FOUND)
    message(STATUS "OpenMP found (C: ${OpenMP_C_VERSION}, CXX: ${OpenMP_CXX_VERSION})")
    set(USE_OPENMP TRUE)
elseif(APPLE)
    # Manual OpenMP support on macOS/Homebrew
    execute_process(
        COMMAND brew --prefix libomp
        OUTPUT_VARIABLE LIBOMP_ROOT
        OUTPUT_STRIP_TRAILING_WHITESPACE
        ERROR_QUIET
    )

    if(LIBOMP_ROOT AND EXISTS "${LIBOMP_ROOT}/include/omp.h")
        message(STATUS "Using Homebrew libomp at ${LIBOMP_ROOT}")

        # Create an imported target that mirrors OpenMP::OpenMP_CXX so
        # downstream code can use a single name.
        add_library(OpenMP::OpenMP_CXX INTERFACE IMPORTED)
        target_compile_options(OpenMP::OpenMP_CXX INTERFACE -Xpreprocessor -fopenmp)
        target_include_directories(OpenMP::OpenMP_CXX INTERFACE "${LIBOMP_ROOT}/include")
        target_link_directories(OpenMP::OpenMP_CXX INTERFACE "${LIBOMP_ROOT}/lib")
        target_link_libraries(OpenMP::OpenMP_CXX INTERFACE omp)
        set(USE_OPENMP TRUE)
    else()
        message(WARNING "libomp not found via Homebrew, disabling OpenMP")
        set(USE_OPENMP FALSE)
    endif()
else()
    message(WARNING "OpenMP not found, disabling parallel rendering")
    set(USE_OPENMP FALSE)
endif()

# ────────────────────────────────────────────────────────────────────
# Subprojects
# ────────────────────────────────────────────────────────────────────
add_subdirectory(src)

if(PATHTRACER_BUILD_TESTS)
    enable_testing()
    add_subdirectory(tests)
endif()
