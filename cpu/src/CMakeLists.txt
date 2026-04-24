set(SRC_CPP
    utils/objectTransform.cpp
    PDF/hairPDF.cpp
    mesh_io/meshLoader.cpp
    primitives/triangleMesh.cpp
    primitives/meshBVH.cpp
    sampler/sampler.cpp
    external/scene_io.cpp
    material/hairMarsch.cpp
    material/disneyPrincipledBSDF.cpp
    base/camera.cpp
    base/framebuffer.cpp
    texture/texture.cpp
    base/primitive.cpp
    base/object3D.cpp
    base/objectList.cpp
)

add_library(pathtracer STATIC ${SRC_CPP})

target_compile_definitions(pathtracer
    PUBLIC
    GL_SIMD
    BIASED_SAMPLING

    # Uncomment to enable discard sampling instead
    # DISCARD_SAMPLING
)

target_include_directories(pathtracer
    PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${PATHTRACER_CPU_INCLUDE_DIR}
)

# ────────────────────────────────────────────────────────────────────
# OpenMP
# USE_OPENMP / OpenMP::OpenMP_CXX come from cpu/CMakeLists.txt.
# Linked PUBLIC so both main.cpp and tests see `_OPENMP` and <omp.h>.
# ────────────────────────────────────────────────────────────────────
if(USE_OPENMP)
    target_link_libraries(pathtracer PUBLIC OpenMP::OpenMP_CXX)
endif()

# ────────────────────────────────────────────────────────────────────
# FBX SDK (optional)
# ────────────────────────────────────────────────────────────────────
find_package(FBX QUIET)

if(FBX_FOUND)
    message(STATUS "FBX SDK found - enabling FBX support")
    target_sources(pathtracer PRIVATE mesh_io/fbxLoader.cpp)
    target_compile_definitions(pathtracer PUBLIC HAS_FBX_SDK)
    target_include_directories(pathtracer PUBLIC ${FBX_INCLUDE_DIRS})
    target_link_libraries(pathtracer PRIVATE ${FBX_LIBRARIES})
else()
    message(STATUS "FBX SDK not found - FBX support will be disabled")
endif()

# ────────────────────────────────────────────────────────────────────
# Main executable
#
# The CMake target is named `pathtracer_app` to avoid colliding with
# the root project name (`PathTracer`) under the Visual Studio
# generator, which would otherwise produce two `PathTracer.vcxproj`
# files in the same solution. OUTPUT_NAME keeps the produced binary
# called `PathTracer(.exe)`.
# ────────────────────────────────────────────────────────────────────
add_executable(pathtracer_app main.cpp)
set_target_properties(pathtracer_app PROPERTIES OUTPUT_NAME PathTracer)
target_link_libraries(pathtracer_app PRIVATE pathtracer)

# Copy FBX SDK libraries to build directory if FBX is enabled (macOS only)
if(FBX_FOUND AND APPLE)
    add_custom_command(TARGET pathtracer_app POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_directory
        "/Applications/Autodesk/FBX SDK/2020.3.7/lib/clang/release"
        $<TARGET_FILE_DIR:pathtracer_app>
    )
endif()
