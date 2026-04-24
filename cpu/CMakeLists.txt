# ──────────────────────────────────────────────────────────────────────
# CPU backend
#
# This is the self-contained CPU path-tracer subproject. Everything that
# only matters to the CPU build (OpenMP detection, FBX SDK lookup, the
# unit tests, the `pathtracer` library and `pathtracer_app` exe) lives
# under cpu/. The GPU backend under gpu/ stays completely independent.
# ──────────────────────────────────────────────────────────────────────

option(PATHTRACER_BUILD_TESTS "Build CPU unit tests" ON)

# Path to the CPU public headers. Exposed as a variable so sibling
# subdirectories (src/, tests/) can reference it without knowing where
# cpu/ itself lives in the tree.
set(PATHTRACER_CPU_INCLUDE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/include)

# Find modules that only the CPU build needs (currently just FindFBX).
list(APPEND CMAKE_MODULE_PATH ${CMAKE_CURRENT_SOURCE_DIR}/cmake)

# ──────────────────────────────────────────────────────────────────────
# OpenMP
#
# Try find_package first (works for MSVC, GCC, Clang on Linux, and
# AppleClang when libomp is discoverable via CMake). On macOS with
# Homebrew libomp we fall back to a manual interface target so the rest
# of the CPU build can uniformly link against `OpenMP::OpenMP_CXX`.
# ──────────────────────────────────────────────────────────────────────
find_package(OpenMP QUIET)

if(OpenMP_CXX_FOUND)
    message(STATUS "OpenMP found (C: ${OpenMP_C_VERSION}, CXX: ${OpenMP_CXX_VERSION})")
    set(USE_OPENMP TRUE)
elseif(APPLE)
    execute_process(
        COMMAND brew --prefix libomp
        OUTPUT_VARIABLE LIBOMP_ROOT
        OUTPUT_STRIP_TRAILING_WHITESPACE
        ERROR_QUIET
    )

    if(LIBOMP_ROOT AND EXISTS "${LIBOMP_ROOT}/include/omp.h")
        message(STATUS "Using Homebrew libomp at ${LIBOMP_ROOT}")

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

# ──────────────────────────────────────────────────────────────────────
# Subprojects
# ──────────────────────────────────────────────────────────────────────
add_subdirectory(src)

if(PATHTRACER_BUILD_TESTS)
    add_subdirectory(tests)
endif()
