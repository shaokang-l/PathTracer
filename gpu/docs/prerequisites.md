# GPU Path Tracing — Prerequisites Crash Course

> A tight, self-checking crash course on the minimum you need to know
> *before* you start porting your CPU path tracer to OWL/OptiX.
>
> Expected time: **one long evening (4–6 h)** to skim everything once,
> a second pass while you implement Step 0–2 of the roadmap.
>
> Sibling doc: [`learning_roadmap.md`](./learning_roadmap.md)

---

## Table of contents

0. [The mental shift](#0-the-mental-shift)
1. [CUDA: the minimum viable model](#1-cuda-the-minimum-viable-model)
2. [OptiX 7: the ray-tracing runtime](#2-optix-7-the-ray-tracing-runtime)
3. [OWL: the wrapper layer](#3-owl-the-wrapper-layer)
4. [POD design rules](#4-pod-design-rules)
5. [Debugging GPU code](#5-debugging-gpu-code)
6. [Reading list in priority order](#6-reading-list-in-priority-order)
7. [Pre-flight self-assessment](#7-pre-flight-self-assessment)

---

## 0. The mental shift

Your CPU renderer is organised around **control flow**: deep class
hierarchies, virtual dispatch, recursion, smart pointers, STL
everywhere. Each ray has its own call stack.

On the GPU, every concept above is either illegal or a performance
trap. GPU code is organised around **data flow**: flat PODs in
`__constant__` or global buffers, explicit switches, iterative loops,
32 threads marching in lockstep through the same instructions.

| CPU reflex                                    | GPU replacement                                        |
| --------------------------------------------- | ------------------------------------------------------ |
| `virtual float scatter_pdf(...) = 0;`         | `switch (mat.kind) { case kLambertian: ... }`          |
| `std::shared_ptr<Material>`                   | `int32_t materialId;` (index into a device buffer)     |
| `std::vector<Vec3> normals;`                  | `Vec3f* d_normals;` allocated once, SBT-bound          |
| Recursive `trace(ray, depth)`                 | Iterative `for (int b = 0; b < MAX_BOUNCES; ++b) { ... }` |
| `if (hit) {...} else {...}` at tight scope    | Design so all warp threads take the same branch        |
| RAII, exceptions                              | `cudaFree`, explicit error codes                       |
| Object lifetimes via shared ownership          | Buffers owned by the `Renderer`, freed at shutdown     |

**The single most important psychological anchor:** your existing CPU
code is the *oracle*. Every time you add a feature on the GPU, the CPU
image is the correct answer. If they diverge, the GPU is wrong until
proven otherwise. You have a free test suite sitting in `cpu/`.

---

## 1. CUDA: the minimum viable model

You do not need to become a CUDA expert. You need to understand five
things well enough to debug them when they go wrong.

### 1.1 SIMT execution and warps

A warp is **32 threads** that execute the *same instruction* at the
same time. If 16 threads hit `if (x)` and 16 hit the `else`, the GPU
runs both branches sequentially while masking the inactive threads.
This is called **warp divergence**.

```cpp
// BAD on GPU: each pixel picks a different case based on scene layout
switch (complicated_expr) { case 0: ...; case 1: ...; case 7: ... }
```

Divergence is not a correctness problem, it's a throughput problem.
For a first GPU renderer it is **fine**. Don't prematurely optimise;
write the switch, measure, then worry.

### 1.2 Memory hierarchy (ranked by speed)

| Tier       | Latency (ballpark)    | Scope                | Use for                                 |
| ---------- | --------------------- | -------------------- | --------------------------------------- |
| Registers  | ~1 cycle              | per-thread           | ray state, local variables              |
| Shared mem | ~20 cycles            | per-block (CUDA only)| rarely used in OptiX programs           |
| L1/L2      | ~30–200 cycles        | chip-wide            | automatic cache                         |
| Global     | ~400–800 cycles       | whole device         | vertex/index buffers, textures          |
| Constant   | ~1 cycle (if cached)  | whole device (read-only, 64 KB) | `optixLaunchParams`          |
| Texture    | ~100 cycles, hw filter| whole device         | image textures, env maps                |

Practical rules:

- Keep per-ray state **small** so it fits in registers.
- Put large read-only tables in global memory (buffers).
- Put broadcast-style data (camera, frame index, pointers to buffers)
  in `__constant__` via `optixLaunchParams`.
- Use `cudaTextureObject_t` for image textures — the hardware does
  filtering and mipmapping for you.

### 1.3 Function qualifiers you will see

```cpp
__host__                // runs on CPU (default)
__device__              // runs on GPU, callable from GPU only
__host__ __device__     // compilable on both sides (what your PODs want)
__global__              // CUDA kernel launched from CPU; NOT used in OptiX
__constant__ T var;     // read-only global on device, small (<=64KB total)
```

Your PODs should look like:

```cpp
struct MaterialGPU {
    MaterialKind kind;
    Vec3f albedo;
    // ...
    inline __host__ __device__ Vec3f eval(...) const { ... }
};
```

### 1.4 Synchronization

- `cudaDeviceSynchronize()` — block the CPU until all pending GPU work
  finishes. Call this before reading back a framebuffer or
  benchmarking.
- OptiX launches are **asynchronous** by default; `owlLaunch2D` can be
  sync or async depending on flavour. When images look "stale" after a
  parameter change, suspect a missing sync.

### 1.5 Alignment and struct layout

CUDA requires natural alignment. A `float4` wants 16-byte alignment,
a `double` wants 8, a `float3` is **three** floats (alignment 4 — it
is *not* padded to 16, which surprises everyone).

```cpp
struct MyPOD {
    int   a;     // offset 0
    float b;     // offset 4
    // implicit 8 bytes padding
    double c;    // offset 16 because double wants 8-byte align
};
static_assert(sizeof(MyPOD) == 24);
```

For cross-boundary types (shared by host `.cpp` and device `.cu`) add:

```cpp
static_assert(std::is_trivially_copyable_v<MyPOD>);
static_assert(sizeof(MyPOD) % 8 == 0);  // or 16 if you use float4
```

### 1.6 Self-test

1. If two threads in the same warp take different `if` branches, what
   happens? What if they are in different warps?
2. Where should the camera parameters live, and in which memory tier?
3. Why is `std::vector<float>` illegal inside device code?
4. What does `cudaDeviceSynchronize` do and when do you need it?

---

## 2. OptiX 7: the ray-tracing runtime

OptiX is a **programming model**, not a library of functions. You
provide code snippets ("programs") for fixed slots in the ray-tracing
pipeline; the driver does traversal, intersection testing on BVH
nodes, and calls your snippets at the right moments.

### 2.1 The programs (ROLES, not functions you "call")

| Program         | When OptiX runs it                                   | Typical work                                           |
| --------------- | ---------------------------------------------------- | ------------------------------------------------------ |
| `raygen`        | Once per launch index (pixel)                        | Build primary ray, loop bounces, write pixel           |
| `miss`          | Ray leaves the scene without hitting anything        | Sample env map / return background radiance            |
| `closest-hit`   | Ray closes the *closest* intersection                | Shading, sampling BSDF, setting up next bounce payload |
| `any-hit`       | *Every* hit along the ray (before sorting)           | Alpha testing, shadow-ray early-out                    |
| `intersection`  | Intersect a *custom* primitive (sphere, curve)       | You compute `t`, normal, report hit                    |

For a first GPU renderer you need: **raygen + miss + closest-hit**.
You get any-hit for free (default "always accept") and intersection
is only for non-triangle primitives.

### 2.2 Ray types

A "ray type" is a label that tells OptiX *which* set of programs to
invoke for a given ray. The classic pair is:

- **Radiance** rays — closest-hit does shading, miss samples env.
- **Shadow/occlusion** rays — closest-hit just sets a "hit" flag,
  any-hit terminates early; miss means "not occluded".

Each ray type has its own CH/AH/miss trio in the SBT. You declare the
count once at pipeline creation (`owlRayTypeCreate` / OptiX pipeline
options).

### 2.3 Acceleration structures

- **BLAS** (Bottom-level) — built over primitives (triangles, AABBs).
  One per mesh is typical.
- **TLAS** (Top-level) — built over *instances* of BLASes, each with
  its own 3×4 transform. This is how you get instancing.

For a single-mesh scene, "group of one BLAS" is fine. The moment you
want two mesh instances with different transforms, you need a TLAS.

### 2.4 The traversable handle

The *entry point* to the acceleration structure is a 64-bit handle
(`OptixTraversableHandle`). You pass it to `optixTrace(...)`. In OWL
it is returned by `owlGroupGetTraversable(...)` and lives in
`launchParams.world` (see `gpu/src/deviceCode.h`).

### 2.5 SBT — the Shader Binding Table (this is the hard one)

**Purpose:** given "this ray hit primitive *p* on instance *i* with
ray type *r*", which CH program runs, and what per-geometry data does
it see?

**Shape:** a table indexed by

```
SBT_offset = instance_sbt_offset + geometry_index * ray_type_count + ray_type
```

Each record is:

```
[ 32-byte header ] [ user data (your SBT struct) ]
```

The header tells the driver which program to run. The user data is
whatever POD you declare (vertex pointer, index pointer, material
id, etc.).

**Why it matters:** every time your scene topology changes (new mesh,
new material binding), the SBT must be rebuilt. OWL rebuilds it
automatically; vanilla OptiX makes you do it yourself.

**Concretely in your scaffold:** `TriangleMeshSBT` in
`gpu/src/deviceCode.h` is the per-geometry SBT record. `RayGenData`
and `MissProgData` are the raygen and miss records.

### 2.6 Payload (PRD = Per-Ray Data)

The payload travels with the ray through `optixTrace`. It is
**tiny**: 8 × `uint32_t` (32 bytes) passed in registers. You cram
booleans, ids, and small floats in.

For bigger PRDs the canonical trick (which OWL does for you) is:

1. Allocate a `PRD` on the raygen stack.
2. Pack its pointer into two `uint32_t` halves.
3. Pass them as payload.
4. CH recovers the pointer, writes into it, returns.

After the trace returns, raygen reads from its own stack-allocated
PRD. You get arbitrary-sized PRD without leaving registers.

Keep your PRD as small as you can get away with; it's hot memory.

### 2.7 `__constant__ optixLaunchParams`

The canonical global:

```cpp
extern "C" __constant__ LaunchParams optixLaunchParams;
```

Every program reads it directly. It holds:

- Frame buffer / accumulation buffer pointers
- Camera (origin, axes, fovy)
- Traversable handle to the TLAS/BLAS
- Frame index (for RNG seeding)
- Pointers to every large scene buffer (materials, lights, env map, ...)

This is the main host→device *push* channel. The host rewrites it
before every launch via `owlParamsSet...`.

### 2.8 Self-test

1. Name the three programs you need for minimum path tracing.
2. What is an SBT record composed of? What happens when you forget to
   rebuild it after changing the scene?
3. Why is payload size constrained? How do people get around it?
4. Where does the traversable handle live and who sets it?
5. What's the difference between BLAS and TLAS?

---

## 3. OWL: the wrapper layer

OWL is a thin C library by Ingo Wald that makes OptiX usable in
**hours** instead of days. You still write OptiX device code; OWL
just hides the setup paperwork.

### 3.1 What OWL adds (value)

- **Typed variables.** You call `owlGeomTypeDeclareVariable(type,
  "vertex", OWL_BUFPTR, OFFSETOF(...))` and then
  `owlGeomSetBuffer(geom, "vertex", buf)`. OWL packs this into the
  SBT record automatically.
- **Automatic SBT build.** `owlBuildSBT(context)` does the boring
  index arithmetic from §2.5.
- **`embed_ptx(...)`** — a CMake macro that compiles your `.cu` with
  `nvcc -ptx` and embeds the PTX as a C symbol in the host binary.
  No runtime file I/O.
- **A viewer.** `OWLViewer` gives you a minimal GLFW window with
  mouse-drag camera — what your scaffold already uses.
- **Fewer foot-guns.** Sane defaults for pipeline options, stack size,
  traversal flags.

### 3.2 What OWL does NOT hide

- Device code is **still OptiX** — you use `optixTrace`,
  `optixGetTriangleBarycentrics`, `optixGetWorldRayOrigin`, etc.
- POD layout is still your responsibility.
- SBT *semantics* (ray types, record layout) still matter — OWL just
  automates the *writing* of records, not the designing of them.

### 3.3 OWL-specific idioms worth memorising

```cpp
// In raygen / CH / miss: pull the per-program SBT record
const RayGenData &rg = owl::getProgramData<RayGenData>();

// The PRD pointer trick, automated:
PerRayData prd{...};
owl::Ray ray(org, dir, tmin, tmax);
owl::traceRay(launchParams.world, ray, prd);
// prd is now populated by the CH program

// Embed PTX into the host binary at build time:
embed_ptx(OUTPUT_TARGET deviceCodePTX PTX_LINK_LIBRARIES owl::owl
          SOURCES deviceCode.cu)
```

The **geom → group → world** chain is:

```
owlTrianglesGeomCreate   -> OWLGeom  (one mesh; references buffers + geomType)
owlTrianglesGeomGroupCreate -> OWLGroup (BLAS over N OWLGeoms)
owlInstanceGroupCreate   -> OWLGroup (TLAS over N BLASes + transforms)
owlGroupGetTraversable   -> 64-bit handle for launchParams.world
```

### 3.4 Self-test

1. What does `embed_ptx` save you from doing at runtime?
2. If you want two spheres with different materials, do you need one
   BLAS or two? How about with different transforms?
3. Where does `owl::getProgramData<T>()` get its data from?
4. Why is OWL code *still OptiX* code on the device side?

---

## 4. POD design rules

Any type that crosses the host↔device boundary (SBT record,
launch-params field, element in a device buffer) must be a **plain
old data** type.

**Allowed:**

- scalars, fixed-size arrays
- structs of the above
- `__host__ __device__` inline functions on the struct
- `enum class` (underlying type = `int32_t`)
- tagged unions (`struct { Kind kind; union { ... } u; }`)
- device pointers (`T*`) — but you must manage their lifetimes

**Banned:**

- virtual functions (no vtable on device)
- `std::shared_ptr`, `std::unique_ptr`, `std::vector`, `std::string`
- `std::function`
- non-trivial constructors/destructors
- references (use pointers)
- exceptions

**Compile-time checks you should add next to every cross-boundary type:**

```cpp
static_assert(std::is_trivially_copyable_v<MaterialGPU>);
static_assert(std::is_standard_layout_v<MaterialGPU>);
static_assert(sizeof(MaterialGPU) % 4 == 0);
```

**Polymorphism replacement — the tagged union pattern you already see
in `gpu/src/deviceCode.h`:**

```cpp
enum class MaterialKind : int32_t { Lambertian, Metal, Dielectric };

struct MaterialGPU {
    MaterialKind kind;
    Vec3f        albedo;    // used by Lambertian, Metal
    float        roughness; // used by Metal
    float        ior;       // used by Dielectric
};

__device__ Vec3f evalBSDF(const MaterialGPU &m, const Vec3f &wi,
                          const Vec3f &wo, const Vec3f &n) {
    switch (m.kind) {
    case MaterialKind::Lambertian: return lambertEval(m, wi, wo, n);
    case MaterialKind::Metal:      return metalEval(m, wi, wo, n);
    case MaterialKind::Dielectric: return dielectricEval(m, wi, wo, n);
    }
    return Vec3f(0.f);
}
```

You can grow this for a dozen materials without ever introducing a
virtual. Past that, you graduate to **per-material `OWLGeomType`**
(separate CH program per material) or **callable programs** — both
are roadmap Step 8+ concerns, not day-1.

---

## 5. Debugging GPU code

This is the section nobody tells you about, and it's the one that
decides whether you ship or rage-quit.

### 5.1 The #1 tool: per-pixel debug buffer

Allocate a device buffer with one `Vec4f` per pixel. Write anything
you want to it from any program. On the host, read it back and dump
an EXR. This is your print statement.

```cpp
// deviceCode.cu
__device__ float4 *debugBuf = ...; // from launchParams
int idx = pixelY * W + pixelX;
debugBuf[idx] = make_float4(normal.x, normal.y, normal.z, t);
```

Now every visualisation is a 5-line code change: throughput,
sampled directions, PDF values, accumulated radiance, whatever.

### 5.2 Gated `printf`

CUDA supports `printf` from device code — but one printf per pixel
(1920×1080) melts your driver. Always gate it:

```cpp
if (pixelX == 960 && pixelY == 540 && launchParams.frameID == 0) {
    printf("hit t=%f normal=(%f,%f,%f)\n", t, n.x, n.y, n.z);
}
```

### 5.3 Progressive simplification

When pixels are wrong, strip the code to its simplest form:

1. Render only **ray direction** as colour. Looks like a gradient?
   Camera is right.
2. Render only **`has_hit`** as white/black. Correct silhouette?
   BVH and geometry are right.
3. Render only **normal** as RGB. Looks smooth/faceted as expected?
   Indices and vertex buffers are right.
4. Render only **albedo on hit**. Correct colours? Material indexing
   right?
5. Render **one-bounce diffuse**. Energy roughly right? BSDF eval
   right?

Each of these is a one-line change in CH. When step N is wrong, you
know the bug is between step N−1 and N.

### 5.4 `compute-sanitizer`

```
compute-sanitizer mypt.exe
compute-sanitizer --tool racecheck mypt.exe
```

Catches out-of-bounds writes, use-after-free, uninitialised memory,
race conditions in shared memory. Slow, but priceless. Run it once a
week.

### 5.5 Nsight Graphics

Optional for a first renderer. Useful once you care about
microseconds: lets you capture a frame, see SBT contents, step
through traversal.

### 5.6 The CPU oracle (repeat until you've internalised it)

For every new feature, write a script that:

1. Renders the same scene with the CPU renderer at N spp.
2. Renders it with the GPU renderer at N spp, same seed policy.
3. Computes MSE / per-channel diff / visual diff image.

Target: < 1 % mean error on a simple test scene after each step.
When it jumps to 10 %, the *last thing you changed* is the bug. You
don't need bisection; you need to just *look*.

---

## 6. Reading list in priority order

### Start immediately (read before Step 0)

- **OWL samples** — `cmd0-optix-v7+owl/cmd1-simpleTriangles`,
  `s02-firstRayTracer`, `s06-pathTracer` (in the OWL repo). One
  evening. These are *the* ground truth for how OWL code is shaped.
- **Ingo Wald, "Introduction to OptiX 7 with OWL"** — YouTube,
  40 min. The author narrating what OWL is and isn't. Watch twice.

### Keep open while porting (Step 0–5)

- **PBRT v4, chapters on GPU rendering** (free online at
  pbr-book.org) — the authoritative reference for translating a
  PBRT-shaped offline renderer to the GPU. Their design and yours
  rhyme closely.
- **Ray Tracing Gems I & II**, selected chapters:
  - Gems I, ch. 20 "Texture Level of Detail Strategies for Real-Time
    Ray Tracing" — only if you add textures.
  - Gems II, ch. 14–15 on "Ray-Tracing Small Primitives" and
    "Importance Sampling of Many Lights" — when you add NEE.
  - Gems II, ch. 3 "Improved Shader and Texture Compression".
- **OWL reference docs** (`owl/doc/`) — the source of truth for the
  API shape of `owlGeomTypeDeclareVariable` etc.

### Reference when something breaks

- **OptiX 7 Programming Guide** (NVIDIA) — don't read linearly. Use
  its index when a specific call (`optixTrace`, `optixReportIntersection`)
  surprises you.
- **CUDA Programming Guide** — only the sections on memory types,
  function qualifiers, constant memory, and textures. You can happily
  ignore 80 % of it.

### Later (Step 7+, optional polish)

- **Nsight Compute docs** — only when you start optimising.
- **Laine & Karras, "High-Performance Software Rasterization on
  GPUs"** (2011) — old but gold on divergence / warp efficiency.
- **Ingo Wald's GTC talks** — various years, great big-picture
  content on "how real engines organise all this".

---

## 7. Pre-flight self-assessment

Close this document. Answer out loud. If you can answer **10 of 12**,
you're ready to start Step 0 of the roadmap.

1. What is a warp, and what happens when threads in one diverge?
2. What is `__constant__` memory used for in an OptiX launch?
3. Name every program slot in the OptiX pipeline and say in one
   sentence what each is for.
4. What is the SBT, what does a record contain, and what must happen
   when the scene changes?
5. Why is `std::shared_ptr<Material>` illegal on the device, and what
   replaces it?
6. What's the difference between BLAS and TLAS, and when do you need
   the latter?
7. How do you debug a black framebuffer when `printf` is too slow to
   use per pixel?
8. What's the CPU renderer's role in the GPU development process?
9. What does OWL's `embed_ptx` save you from?
10. What does `getProgramData<T>()` return, and where did the `T`
    come from?
11. Why do tagged unions beat virtual functions on the GPU?
12. If the GPU image diverges from the CPU image by 40 %, what's your
    first debugging step?

When you can answer all twelve without looking, open
[`learning_roadmap.md`](./learning_roadmap.md) and start Step 0.
