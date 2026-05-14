# tests/CMakeLists.txt
#
# GoogleTest is fetched automatically via FetchContent so the project
# builds out of the box on Windows, Linux, and macOS without a pre-
# installed GTest. Set -DPATHTRACER_USE_SYSTEM_GTEST=ON to prefer an
# already-installed copy.

include(FetchContent)

option(PATHTRACER_USE_SYSTEM_GTEST "Use system-installed GoogleTest instead of FetchContent" OFF)

if(PATHTRACER_USE_SYSTEM_GTEST)
    find_package(GTest REQUIRED)
else()
    # Make gtest play nicely with MSVC's /MD runtime used by this project.
    set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)

    FetchContent_Declare(
        googletest
        GIT_REPOSITORY https://github.com/google/googletest.git
        GIT_TAG        v1.14.0
        GIT_SHALLOW    TRUE
    )
    FetchContent_MakeAvailable(googletest)
endif()

# Portable "threads" library (maps to pthread on Unix, no-op on MSVC).
find_package(Threads REQUIRED)

include(GoogleTest)

set(PATHTRACER_TESTS
    test_mesh_io
    test_hair_pdf
    test_halton
    test_refract
    test_dielectric
    test_pdf_normalization
    test_common_material
    test_restir_reservoir
    test_restir_target
)

foreach(test_name IN LISTS PATHTRACER_TESTS)
    add_executable(${test_name} ${test_name}.cpp)
    target_link_libraries(${test_name}
        PRIVATE
        pathtracer
        GTest::gtest
        GTest::gtest_main
        Threads::Threads
    )
    target_include_directories(${test_name}
        PRIVATE
        ${PATHTRACER_CPU_INCLUDE_DIR}
        ${CMAKE_CURRENT_SOURCE_DIR}
    )

    # Pass the absolute path to cpu/tests/data/ into each test so they
    # don't have to guess where the cwd is at launch time.
    target_compile_definitions(${test_name}
        PRIVATE
        PATHTRACER_TEST_DATA_DIR="${CMAKE_CURRENT_SOURCE_DIR}/data"
    )

    # Register each TEST() case with CTest individually.
    gtest_discover_tests(${test_name}
        WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
        DISCOVERY_TIMEOUT 30
    )
endforeach()
