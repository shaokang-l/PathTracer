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
    ├── main.cpp          entry point; CLI scene/render settings, headless output
    ├── Viewer.{h,cpp}    GLFW window + camera controls
    ├── Renderer.{h,cpp}  owns OWL context, module, programs, groups, SBT,
    │                     launch params, material/light buffers, denoiser, timing
    ├── Scene.{h,cpp}     host-side POD scene: meshes, materials, lights
    ├── SceneXml.cpp      lowers the shared Mitsuba XML scene description
    ├── SceneExport.*     exports the built-in GPU scene to Mitsuba XML
    ├── Denoiser.{h,cpp}  host-side OptiX denoiser wrapper
    ├── postprocess.*     CUDA tone-map/gamma/pack pass
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
- **Launch parameters, not SBT rebuilds, for per-frame data.** Camera pose,
  frame index, spp, bounces, debug view, material/light buffers, and output
  pointers all live in `LaunchParams`, which keeps interactive updates cheap.
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
- **Progressive accumulator in float4 HDR, then CUDA post-process.** Raygen
  writes linear HDR; optional OptiX denoising and tone-map/gamma packing happen
  after the OptiX launch. Accumulator resets on any camera change.
- **Built-in profiling hooks.** `Renderer` records CUDA event timings and
  prints smoothed frame time / primary-ray throughput periodically. `main`
  supports `--frames N` for Nsight Compute / Systems runs.
- **Headless validation path.** `main` supports headless PNG output and shared
  CPU/GPU render settings, including `--scene-xml`, camera overrides, tone-map
  settings, and `--debug-view`.
- **Runtime debug-view switching.** In the interactive viewer, keys `1` through
  `6` switch between `beauty`, `normal`, `albedo`, `visibility`,
  `material-id`, and `light-id`.

## Building

Use the checked-in CMake presets from the repository root:

```powershell
git submodule update --init --recursive
cmake --preset gpu-ninja-relwithdebinfo
cmake --build --preset gpu-ninja-relwithdebinfo
```

The generated executable is:

```text
build-gpu-ninja/gpu/mypt.exe
```

Run interactively:

```powershell
.\build-gpu-ninja\gpu\mypt.exe
```

Run a fixed number of frames for profiling:

```powershell
.\build-gpu-ninja\gpu\mypt.exe --frames 300
```

Run a headless XML validation render:

```powershell
.\build-gpu-ninja\gpu\mypt.exe `
  --scene-xml assets\validation\cornell_box.xml `
  --headless --width 64 --height 64 --spp 1 --frames 1 `
  --camera-origin 0,1,-18 --camera-target 0,1,0 `
  --debug-view normal --output artifacts\debug_views\gpu_normal.png
```

Useful shared render/debug flags:

```text
--scene-xml <path>
--width <int> --height <int> --spp <int> --max-depth <int>
--gamma <float> --tonemap clamp|reinhard --background r,g,b
--camera-origin x,y,z --camera-target x,y,z --camera-up x,y,z --fov <deg>
--debug-view beauty|normal|albedo|visibility|material-id|light-id
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
  write linear HDR accumulator
postprocess
  optionally denoise accumulated beauty
  tone-map/gamma/pack to RGBA8 framebuffer
```

Debug views branch early in raygen. They use deterministic pixel-center primary
rays, do not use temporal accumulation, and skip the denoiser so they remain
useful for CPU/GPU alignment.

This is intentionally close to the CPU path tracer's mental model:

```text
CPU: object hit -> Material::scatter() virtual dispatch
GPU: OptiX hit  -> MaterialGPU.kind switch dispatch
```

## Porting Notes

Near-term work should keep the current separation of concerns:

1. **Materials** - keep growing `MaterialGPU` and `bxdf/` one material family
   at a time. Keep per-BxDF math in device helpers and let raygen own the
   integrator loop.
2. **Direct lighting** - quad-light sampling and binary shadow rays are in
   place. The next step is making the estimator robust for many lights, then
   adding MIS or ReSTIR DI.
3. **Geometry** - keep host scene conversion in `Scene.cpp` / `Renderer.cpp`.
   If a single CPU mesh contains multiple materials, split it into multiple
   `TriangleMesh` records or add per-primitive material indexing later.
4. **Validation** - keep XML alignment and debug views green before adding
   temporal/spatial reuse or more complex light sampling.
5. **Profiling** - use Nsight data before changing architecture. Per-material
   closest-hit programs can remove a material switch, but they also spread
   shading logic across shader entry points. Wavefront scheduling is a larger
   step and should wait until material/light coverage is stable.

## Known GPU Limitations

- Single geometry type and one radiance closest-hit program; material dispatch
  is still via `MaterialGPU.kind`.
- Direct lighting is present but still simple: one flat `LightGPU` buffer,
  quad-light sampling, binary shadow visibility, no MIS yet.
- No textures, normal maps, alpha masks, or per-primitive material ids yet.
- No motion blur, no instancing (one BLAS containing everything).
- No volumes or participating media on the GPU.
- No wavefront scheduler.
- ReSTIR DI currently has a no-reuse baseline and debug plumbing; see
  `../docs/restir_di_roadmap.md` for the remaining temporal/spatial reuse plan.
- Denoiser exists as an optional post-process, but it is not a substitute for
  fixing sampling/debug-view correctness.

All of these are expected follow-ups; the current focus is keeping the GPU
backend small enough to reason about while it catches up to CPU features.
