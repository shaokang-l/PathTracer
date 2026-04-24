# my-pt — interactive OWL path-tracing scaffold

A minimal, OOP-friendly starting point for porting a CPU path tracer
to OWL/OptiX. Everything is split into clear layers so you can drop
your own abstractions on top without fighting a monolithic sample.

## What's in here

```
my-pt/
├── CMakeLists.txt
├── README.md          (this file)
└── src/
    ├── main.cpp        thin entry point
    ├── Viewer.{h,cpp}  GLFW window + camera controls (owl_viewer subclass)
    ├── Renderer.{h,cpp} owns ALL OWL handles (context, module, raygen,
    │                    launch params, groups). Only this file talks to
    │                    the OWL C API.
    ├── Scene.{h,cpp}    pure host-side POD scene (vector<TriangleMesh>).
    │                    This is where you plug your existing OOP scene.
    ├── deviceCode.h     shared host<->device structs (LaunchParams,
    │                    TriangleMeshSBT, MaterialGPU, ...). Included
    │                    from both host .cpp and device .cu.
    ├── deviceCode.cu    the ONE file compiled to PTX. Contains only
    │                    the OptiX raygen / closest-hit / miss programs.
    └── Materials.h      device-only BSDF sampling helpers. This is the
                         file you'll grow the most during porting.
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
                 - TriangleMesh CH
                 - path-tracing raygen
                 - env-light miss
```

Key design choices:

- **`Renderer` is the ONLY class that touches the OWL C API.** Your
  future `Integrator`, `Camera`, `Sampler`, `Light`, etc. classes can
  layer on top without ever seeing `OWLContext`.
- **Launch parameters, not SBT rebuilds, for per-frame data.** Camera
  pose, frame index, spp, bounces all live in `LaunchParams` — the
  main path for interactive/progressive rendering in OptiX.
- **One `OWL_TRIANGLES` geom type with `MaterialGPU` as a per-geom SBT
  variable.** This is the simplest starting point; split into multiple
  geom types per material family only when it hurts (e.g. when you
  want per-material closest-hits, specialised attribute generation).
- **Progressive accumulator in float4 HDR + tone-map + gamma in the
  raygen.** Accumulator resets on any camera change.

## Building (inside the OWL repo — quick test)

1. Add one line to `c:/Users/lshk7/owl/CMakeLists.txt` near the bottom
   (e.g. after the `samples` block) — only if you want to build it in
   the main OWL solution:

   ```cmake
   add_subdirectory(my-pt)
   ```

2. Re-configure and build (from Cursor: *CMake: Configure* then *Build*).
   The target name is `mypt`. It will open a window showing a tiny
   test Cornell-box-style scene with one mirror, one light, and two
   diffuse walls.

## Building (integrated into YOUR CPU path tracer project)

This is the path you probably want.

### 1. Folder layout

Inside your path-tracer project:

```
<your-pt-repo>/
├── CMakeLists.txt
├── third_party/
│   └── owl/          <-- OWL, e.g. as a git submodule of NVIDIA/OWL
├── gpu/              <-- rename 'my-pt/' to whatever fits your repo
│   ├── CMakeLists.txt
│   └── src/ ...
└── ... your existing CPU code ...
```

### 2. Copy only this one folder

Copy the **entire** `my-pt/` directory (the one this README lives in)
into your project. Rename it to `gpu/` or whatever you prefer. You do
**not** need to copy anything else from the OWL repo — OWL itself
comes in via `add_subdirectory` on the submodule.

### 3. Add OWL as a submodule

```bash
cd <your-pt-repo>
git submodule add https://github.com/NVIDIA/OWL.git third_party/owl
git submodule update --init --recursive
```

### 4. Wire it up in your top-level `CMakeLists.txt`

Add near the top (before you define your own targets):

```cmake
# --- OWL + my-pt ---
add_subdirectory(third_party/owl EXCLUDE_FROM_ALL)
add_subdirectory(gpu)  # or whatever you called the copied my-pt/
```

`EXCLUDE_FROM_ALL` prevents OWL's sample binaries from being built by
default — you'll still get the `owl::owl` and `owl_viewer` targets you
need.

### 5. Pass CUDA architecture

In your root CMakeLists.txt (or via the `-D` flag), set your GPU:

```cmake
set(CMAKE_CUDA_ARCHITECTURES 89 CACHE STRING "")  # 89 = RTX 40xx (Ada)
```

### 6. Build and run

The target `mypt` should now appear in your build. Once it runs and
opens the window, you're ready to start porting.

## Porting notes (next step)

When you come back with your OOP CPU path tracer, here's the order I'd
tackle things in — each step is small enough to ask a follow-up chat
about in isolation:

1. **Materials** — write a translator in `Scene.cpp` that converts
   your polymorphic `Material*` (or whatever base class) into
   `MaterialGPU`. Extend the tagged union in `deviceCode.h` and the
   `sampleBSDF` switch in `Materials.h` to match your BRDFs. When the
   switch gets too big, split into multiple `OWLGeomType`s — one per
   material family — each with its own closest-hit. The renderer
   already supports that; just add another `OWLGeomType`/closest-hit
   pair and route meshes to it based on material.
2. **Geometry** — implement your `Mesh*` → `mypt::TriangleMesh`
   conversion in one place (e.g. a free `meshToOwl(...)` function or
   a visitor). Keep transforms out of the mesh: use OWL instances
   (`owlInstanceGroupCreate`) for that — it's how you get multi-level
   instancing for free.
3. **Lights** — add a `std::vector<LightGPU>` to `Scene`, upload it as
   an `OWLBuffer`, add `OWL_BUFPTR` field + count to `LaunchParams`,
   then do next-event estimation inside the raygen loop (or in a
   helper called from closest-hit).
4. **Camera** — `Viewer::cameraChanged()` currently forwards to
   `Renderer::setCamera` with a pinhole model. To add DoF, bokeh,
   orthographic, equirect, etc., extend `LaunchParams::camera` and
   the ray-generation code in `deviceCode.cu::rayGen`. Your host-side
   `Camera` class can stay exactly as it is; just translate it to
   `setCamera(...)`.
5. **Sampler** — replace `owl::common::LCG` with your own sampler.
   Pass per-pixel seeds via `LaunchParams`.
6. **Integrators** — if you want path/BDPT/etc. as separate
   integrators, either (a) add a switch in raygen driven by a
   launch-param enum, or (b) create a separate `OWLRayGen` per
   integrator and let `Renderer` pick which one to launch.
7. **Denoiser** — not in OWL. See the main OWL README's FAQ / the
   OptiX SDK `optixDenoiser` sample. Plugs in cleanly after step 6
   because `Renderer` already owns an HDR float4 accumulator that's
   ideal denoiser input.

## Known limitations of the scaffold (on purpose)

- Single ray type (no shadow rays / NEE yet).
- Single closest-hit program; material dispatch is via a switch.
- No textures / normal maps.
- No motion blur, no instancing (one BLAS containing everything).
- No denoiser (would be additive, see step 7 above).

All of these are easy follow-ups once the basic pipeline is running
on your scene.
