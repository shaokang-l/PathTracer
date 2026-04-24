cmake_minimum_required(VERSION 3.19)

# ──────────────────────────────────────────────────────────────────────
# Backend selection
#
# The CPU and GPU renderers live as two parallel subprojects under
# cpu/ and gpu/. They share nothing at the code level — each one has
# its own CMakeLists, its own headers, its own build rules. The only
# thing that crosses the boundary is file-format-level data (scene
# assets, not C++ types).
#
# Default: CPU on, GPU off.  Flip PATHTRACER_BUILD_GPU=ON to also
# build the OWL/OptiX backend under gpu/.
# ──────────────────────────────────────────────────────────────────────
option(PATHTRACER_BUILD_CPU "Build the CPU path tracer (cpu/)"         ON)
option(PATHTRACER_BUILD_GPU "Build the OWL/OptiX GPU renderer (gpu/)"  OFF)

if(PATHTRACER_BUILD_GPU)
    project(PathTracer LANGUAGES C CXX CUDA)
    if(NOT CMAKE_CUDA_ARCHITECTURES)
        set(CMAKE_CUDA_ARCHITECTURES 89 CACHE STRING "CUDA arch list (89 = RTX 40-series / Ada)")
    endif()
else()
    project(PathTracer LANGUAGES C CXX)
endif()

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

# ──────────────────────────────────────────────────────────────────────
# MSVC / Windows quality-of-life (applies to both backends)
#
# Scope the MSVC-only flags to C/CXX via a generator expression so they
# don't leak into nvcc (where `/utf-8` etc. would be misread as input
# filenames). Preprocessor defines are safe for all languages.
# ──────────────────────────────────────────────────────────────────────
if(MSVC)
    add_compile_options(
        "$<$<COMPILE_LANGUAGE:C,CXX>:/utf-8>"
        "$<$<COMPILE_LANGUAGE:C,CXX>:/bigobj>"
        "$<$<COMPILE_LANGUAGE:C,CXX>:/permissive->"
    )
    add_compile_definitions(_USE_MATH_DEFINES NOMINMAX _CRT_SECURE_NO_WARNINGS)
endif()

# Enable CTest at the root so `ctest --test-dir build` works regardless
# of which subproject(s) own the test targets. Subprojects can still
# call add_test() / gtest_discover_tests() from their own CMakeLists.
enable_testing()

# ──────────────────────────────────────────────────────────────────────
# Subprojects
# ──────────────────────────────────────────────────────────────────────
if(PATHTRACER_BUILD_CPU)
    add_subdirectory(cpu)
endif()

# GPU backend: OWL + gpu/
# EXCLUDE_FROM_ALL on OWL so we don't pull its own samples into our
# default build (the `mypt` target still resolves owl::owl / owl_viewer
# via add_subdirectory, it just doesn't force them to compile unless
# something links them).
if(PATHTRACER_BUILD_GPU)
    if(NOT EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/third_party/owl/CMakeLists.txt)
        message(FATAL_ERROR
            "PATHTRACER_BUILD_GPU=ON but third_party/owl/ is missing. "
            "Did you run: git submodule update --init --recursive ?")
    endif()
    add_subdirectory(third_party/owl EXCLUDE_FROM_ALL)
    add_subdirectory(gpu)
endif()
