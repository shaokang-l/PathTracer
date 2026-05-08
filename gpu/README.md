# GPU Backend - OWL/OptiX Path Tracer

This directory contains the experimental GPU backend for the path tracer. It is
an OWL/OptiX renderer designed to port the CPU path tracer incrementally while
keeping the architecture readable: OptiX handles traversal and shader binding,
raygen owns the integrator loop, and material polymorphism is represented as
POD data plus device-side dispatch.

## What's in here

```text
gpu/
├── CMakeLists.txt
├── README.md
└── src/
    ├── main.cpp          thin entry point; supports `--frames N` for profiling
    ├── Viewer.{h,cpp}    GLFW window + camera controls
    ├── Renderer.{h,cpp}  owns OWL context, module, programs, groups, SBT,
    │                     launch params, material/light buffers, and timing
    ├── Scene.{h,cpp}     host-side POD scene: meshes, materials, lights
    ├── deviceCode.cu     OptiX raygen / closest-hit / miss programs
    ├── deviceCode.h      compatibility umbrella for shared POD headers
    ├── geometryData.h    geometry SBT records, currently TriangleMeshSBT
    ├── launchParams.h    RayGenData, MissProgData, LaunchParams
    ├── material.h        MaterialKind and MaterialGPU
    ├── light.h           LightKind, LightGPU, and light sampling helpers
    ├── rayTypes.h        radiance and shadow ray type declarations
    ├── bsdf.h            world-space BSDF wrapper
    ├── bxdfDispatch.h    tagged-union dispatch over MaterialGPU.kind
    └── bxdf/             per-BxDF implementations
```

## Architecture at a glance

```
                 +-----------------------+
 main.cpp  --->  | Viewer (OWLViewer)    |---+
                 | - GLFW window         |   |  fbPointer (RGBA8, GL-shared)
                 | - camera manipulator  |   |  camera changes
                 +-----------------------+   v
                 +-----------------------+
                 | Renderer (host-side)  |
                 | - OWL context/module  |
                 | - geoms / groups / SBT|
                 | - OWLLaunchParams     |
                 +-----------------------+
                            |
                 device-side programs (deviceCode.cu)
                 - radiance TriangleMesh CH
                 - shadow TriangleMesh CH
                 - path-tracing raygen
                 - environment / shadow miss
```

Key design choices:

- **`Renderer` is the ONLY class that touches the OWL C API.** Your
  future `Integrator`, `Camera`, `Sampler`, `Light`, etc. classes can
  layer on top without ever seeing `OWLContext`.
- **Launch parameters, not SBT rebuilds, for per-frame data.** Camera
  pose, frame index, spp, bounces all live in `LaunchParams` — the
  main path for interactive/progressive rendering in OptiX.
- **Thin closest-hit, raygen-side shading.** The radiance closest-hit
  program reconstructs `hitP`, the geometric normal, and `materialId`.
  The raygen program fetches `MaterialGPU`, builds the BSDF wrapper, and
  owns throughput, emission, direct lighting, Russian roulette, and next-ray
  spawning.
- **SBT as scene binding metadata.** The per-geometry SBT record stores
  vertex/index buffers and `materialId`. Materials and lights live in global
  device buffers referenced by launch params, so their contents can be updated
  without rebuilding the SBT.
- **GPU-friendly material polymorphism.** CPU virtual `Material::scatter()`
  dispatch is represented as `MaterialGPU.kind` plus switches in
  `bxdfDispatch.h`. This keeps material math in small device functions rather
  than spreading integrator logic across many closest-hit programs.
- **Two ray types.** Radiance rays use the main closest-hit/miss programs.
  Shadow rays use a minimal closest-hit and miss pair for binary visibility
  queries during direct-light sampling.
- **Progressive accumulator in float4 HDR + tone-map + gamma in the
  raygen.** Accumulator resets on any camera change.
- **Built-in profiling hooks.** `Renderer` records CUDA event timings and
  prints smoothed frame time / primary-ray throughput periodically. `main`
  supports `--frames N` for Nsight Compute / Systems runs.

## Building

The top-level CMake project builds the CPU backend by default. Enable this
backend with `PATHTRACER_BUILD_GPU=ON`:

```bash
git submodule update --init --recursive
cmake -S . -B build -DPATHTRACER_BUILD_GPU=ON
cmake --build build --target mypt --config Release
```

The root `CMakeLists.txt` defaults CUDA architecture to Ada / RTX 40-series
(`89`) when the GPU backend is enabled and no architecture is provided:

```cmake
set(CMAKE_CUDA_ARCHITECTURES 89 CACHE STRING "CUDA arch list")
```

Run interactively:

```bash
./build/gpu/mypt
```

Run a fixed number of frames for profiling:

```bash
./build/gpu/mypt --frames 300
```

## Current GPU Pipeline

```text
raygen
  generate camera ray
  trace RadianceRay
    -> TriangleMesh CH fills PRD(hitP, N, materialId, emission)
    -> miss returns sky emission
  if hit:
    fetch MaterialGPU by materialId
    build BSDF frame
    sample one quad light if the BSDF has a non-delta component
    trace ShadowRay for visibility
    sample BSDF for the next bounce
    update throughput and Russian roulette
```

This is intentionally close to the CPU path tracer's mental model:

```text
CPU: object hit -> Material::scatter() virtual dispatch
GPU: OptiX hit  -> MaterialGPU.kind switch dispatch
```

## Porting Notes

Near-term work should keep the current separation of concerns:

1. **Materials** - grow `MaterialGPU` and `bxdf/` one material family at a
   time. Keep per-BxDF math in device helpers and let raygen own the
   integrator loop.
2. **Direct lighting** - the first quad-light sampler and shadow ray path are
   in place. The next step is making the estimator robust enough for multiple
   lights and adding MIS with the BSDF pdf.
3. **Geometry** - keep host scene conversion in `Scene.cpp` / `Renderer.cpp`.
   If a single CPU mesh contains multiple materials, split it into multiple
   `TriangleMesh` records or add per-primitive material indexing later.
4. **Profiling** - use Nsight data before changing architecture. Per-material
   closest-hit programs can remove a material switch, but they also spread
   shading logic across shader entry points. Wavefront scheduling is a larger
   step and should wait until material/light coverage is stable.

## Known limitations of the scaffold (on purpose)

- Single geometry type and one radiance closest-hit program; material dispatch
  is still via `MaterialGPU.kind`.
- Direct lighting is present but still simple: one flat `LightGPU` buffer,
  quad-light sampling, binary shadow visibility, no MIS yet.
- Dielectric BxDF is still a stub on the GPU.
- No textures, normal maps, alpha masks, or per-primitive material ids yet.
- No motion blur, no instancing (one BLAS containing everything).
- No volumes or participating media on the GPU.
- No wavefront scheduler.
- No denoiser yet.

All of these are expected follow-ups; the current focus is keeping the GPU
backend small enough to reason about while it catches up to CPU features.
