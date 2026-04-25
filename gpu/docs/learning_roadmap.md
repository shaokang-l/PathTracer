# GPU Path Tracing — Concrete Learning Roadmap

> An ordered, step-by-step plan for porting your CPU path tracer to
> OWL/OptiX. Each step teaches **one** new GPU concept and ends with
> a pixel-level acceptance test against your CPU oracle.
>
> Sibling doc: [`prerequisites.md`](./prerequisites.md) — read first.
>
> Reference code in-repo:
> - CPU oracle: `cpu/src/`, `cpu/include/`
> - GPU scaffold: `gpu/src/{Scene.h, Materials.h, Renderer.h,
>   deviceCode.h, deviceCode.cu}`

---

## Table of contents

- [The golden rule: parity harness on day 1](#the-golden-rule-parity-harness-on-day-1)
- [How each step is structured](#how-each-step-is-structured)
- [Step 0 — Annotate your scaffold](#step-0--annotate-your-scaffold)
- [Step 1 — Material indirection via device buffer](#step-1--material-indirection-via-device-buffer)
- [Step 2 — `evalBSDF` and `pdfBSDF` parity](#step-2--evalbsdf-and-pdfbsdf-parity)
- [Step 3 — Shadow rays and a second ray type](#step-3--shadow-rays-and-a-second-ray-type)
- [Step 4 — Albedo texture via `cudaTextureObject_t`](#step-4--albedo-texture-via-cudatextureobject_t)
- [Step 5 — NEE integrator on the GPU](#step-5--nee-integrator-on-the-gpu)
- [Step 6 — MIS integrator on the GPU](#step-6--mis-integrator-on-the-gpu)
- [Step 7 — Offline mode with tile scheduling](#step-7--offline-mode-with-tile-scheduling)
- [Step 8 — Microfacet (conductor + dielectric)](#step-8--microfacet-conductor--dielectric)
- [Step 9 — HDRI env-map with CDF importance sampling](#step-9--hdri-env-map-with-cdf-importance-sampling)
- [Step 10 — Instancing via TLAS](#step-10--instancing-via-tlas)
- [Beyond: Disney, volumes, ReSTIR](#beyond-disney-volumes-restir)
- [Realistic timeline](#realistic-timeline)
- [What NOT to do](#what-not-to-do)

---

## The golden rule: parity harness on day 1

**Before you write any OptiX code**, build a script that:

1. Loads the same scene description into both renderers.
2. Renders it to two EXR/PNG files.
3. Computes a per-pixel RGB diff and a scalar MSE.
4. Prints a pass/fail based on a threshold.

Your scaffold already uses a tiny shared format (see `gpu/src/Scene.h`).
Bring your CPU entry point (`cpu/src/main.cpp`) to the point where it
consumes the same description. Day 1 goal: both renderers produce a
flat grey image (no shading yet) and the diff is zero.

Once this harness exists, *every step below ends with running it*.

Put the harness under `tools/parity/` at the repo root so it's shared
(or under `gpu/tools/parity/` if you want it purely GPU-side —
doesn't matter, just pick one place).

---

## How each step is structured

Every step has the same template:

- **Goal.** One sentence.
- **Concepts learned.** The GPU prerequisites this step *teaches*
  (cross-ref to `prerequisites.md`).
- **Prereqs.** What must already be done in the roadmap.
- **Implementation hints.** *Not* spoilers — the shape of the change,
  not the code.
- **Acceptance test.** A concrete, measurable criterion. You're done
  when the diff is below the threshold.
- **Time estimate.** For a student who read the prerequisites once.

Time estimates assume you're debugging the first occurrence of each
concept. The second time you do a similar step it's ~1/3 the time.

---

## Step 0 — Annotate your scaffold

**Goal.** Read every line of `gpu/src/deviceCode.cu` and
`gpu/src/deviceCode.h` and write a comment next to each one saying
*what OptiX/OWL concept it uses*. Don't change any code.

**Concepts learned.** All of §1–§3 in prerequisites, now grounded in
your actual repo.

**Prereqs.** §1–§3 of prerequisites read once.

**Implementation hints.**
- For each `owl*` function call, identify which of OWL's four areas
  it belongs to: *context setup*, *geometry/SBT setup*, *launch*, or
  *frame I/O*.
- For each `optix*` device call, mark which *program type* it's legal
  in (raygen-only, CH-only, etc.).
- For each field in `LaunchParams`, say which memory tier it lives in
  after the launch and who wrote it.

**Acceptance test.** Close the docs. Explain each line to a rubber
duck without looking. If you stumble on something, go back to the
prereq doc and re-read that specific sub-section.

**Time estimate.** 3–5 hours, spread over two sittings.

---

## Step 1 — Material indirection via device buffer

**Goal.** Move `MaterialGPU` out of `TriangleMeshSBT` and into its own
device buffer. Each mesh's SBT record stores only a `int32_t
materialId`. The CH program looks materials up via
`launchParams.materials[id]`.

**Concepts learned.**
- Separating per-mesh data from per-material data.
- Owning and uploading a device buffer from the host.
- How changing SBT data *does not* need a pipeline rebuild (only an
  SBT rebuild — OWL handles it).

**Prereqs.** Step 0.

**Implementation hints.**
- Host side: add a `std::vector<MaterialGPU>` to `Scene` (in
  `gpu/src/Scene.h`). Upload it as an `OWLBuffer`. Store the pointer
  in `LaunchParams`.
- `TriangleMeshSBT` gets a new `int32_t materialId` field. Remove the
  inline `MaterialGPU`.
- CH program fetches the id from the SBT record, then reads the
  material via `launchParams.materials[id]`.
- Update `Renderer::setScene` so materials can be swapped without
  re-uploading vertex/index data.

**Acceptance test.** Render a 2-sphere scene where both spheres share
the same material (diffuse red). Then *flip the material buffer*
between frames so both turn blue. Both transitions happen in under a
frame. Parity diff vs CPU: zero for the static frames.

**Time estimate.** 3–4 hours.

---

## Step 2 — `evalBSDF` and `pdfBSDF` parity

**Goal.** Implement `evalBSDF`, `sampleBSDF`, `pdfBSDF` on the GPU for
your first two materials (Lambertian + Metal). Match the CPU impl
bit-for-bit on unit directions.

**Concepts learned.**
- Writing `__host__ __device__` math helpers.
- How the "virtual" interface in `cpu/include/material/materialBase.hpp`
  becomes a tagged-union switch.
- Numerical determinism: `fmaf`, `__sinf`, `sinf` — pick one and be
  consistent.

**Prereqs.** Step 1.

**Implementation hints.**
- Keep the function signatures parallel to the CPU side so `grep` can
  line them up.
- Put all three in `gpu/src/Materials.h` as `__device__` inline
  functions that switch on `mat.kind`.
- Write a **unit test** (gtest on the CPU side) that:
  - Fills a grid of (wi, wo, normal) triples.
  - Evaluates the CPU BSDF and the GPU BSDF (via a tiny CUDA kernel
    that calls your `__device__` function and copies results back).
  - Asserts they agree within 1e-5.
- `pdfBSDF` must integrate to 1 over the hemisphere. Add a Monte Carlo
  check: draw 10k samples, accumulate `1/pdf`, expect `2π`.

**Acceptance test.** Unit tests pass. Render a Cornell box with
diffuse walls + one metallic sphere at 64 spp. Parity diff vs CPU:
MSE < 1e-3.

**Time estimate.** 6–8 hours.

---

## Step 3 — Shadow rays and a second ray type

**Goal.** Add a "shadow" ray type. Trace NEE-style visibility rays to
an analytic point light and multiply the Lambertian term by visibility.

**Concepts learned.**
- Multiple ray types in a single pipeline.
- Any-hit program for early termination.
- The `OPTIX_RAY_FLAG_TERMINATE_ON_FIRST_HIT` flag.
- Why shadow rays have a *different* PRD shape than radiance rays.

**Prereqs.** Step 2.

**Implementation hints.**
- Declare two ray types in the OWL context setup. OWL uses the
  ray-type count in the SBT stride.
- Shadow CH is **empty** or nonexistent; you rely on miss to mean
  "light is visible".
- Shadow miss sets `prd.visible = true`. Radiance miss is the env
  sampling one you already have.
- Set `TERMINATE_ON_FIRST_HIT` and optionally
  `DISABLE_CLOSEST_HIT_SHADER` for the shadow ray.
- `tmax` for the shadow ray = distance to the light minus epsilon.

**Acceptance test.** Point light inside Cornell box; hard shadows on
the floor. Compare to CPU — silhouette pixel-perfect, interior MSE
< 1e-3.

**Time estimate.** 4–6 hours. (Expect one full hour lost to a
self-intersection `tmin` bug. Everyone pays this tax.)

---

## Step 4 — Albedo texture via `cudaTextureObject_t`

**Goal.** Replace the constant `albedo` in Lambertian with a
texture-driven albedo. The CH program samples a 2D texture using hit
UVs.

**Concepts learned.**
- CUDA texture objects (hardware-accelerated bilinear filtering).
- UV interpolation from triangle barycentrics.
- Host-side image loading (`stb_image`) and upload via cudaMallocArray
  + cudaMemcpy2DToArray.
- Storing a `cudaTextureObject_t` inside a POD.

**Prereqs.** Step 2.

**Implementation hints.**
- Your `MaterialGPU` grows a `cudaTextureObject_t albedoTex` field
  and a `bool hasAlbedoTex`.
- Sample: `float4 c = tex2D<float4>(mat.albedoTex, u, v);`
- Build a small host-side `TextureCache` keyed by filename so the
  same PNG doesn't get uploaded twice.
- OWL does **not** manage textures for you; this is raw CUDA.

**Acceptance test.** Render a textured quad (checkerboard or a
photograph). Parity diff vs CPU: MSE < 2e-3 (slightly looser to allow
for bilinear vs your CPU sampler).

**Time estimate.** 6–8 hours. More if you also want mipmaps.

---

## Step 5 — NEE integrator on the GPU

**Goal.** Port your CPU `maxdepth_nee` integrator. Each bounce draws a
sample from a light, computes visibility + BSDF + geometry term,
adds to accumulated radiance. No MIS yet.

**Concepts learned.**
- Iterative (non-recursive) path tracing on the GPU.
- Reusing the RNG state across bounces.
- Why NEE explodes with bad sampling choices (and how to tell).

**Prereqs.** Steps 3, 4.

**Implementation hints.**
- The **whole** integrator lives in raygen. No recursion; use a
  `for (bounce = 0; bounce < maxDepth; ++bounce)` loop.
- `PerRayData` grows to hold: incoming direction, hit position,
  normal, material id, UV — the stuff CH writes for raygen to
  process.
- Keep the CPU integrator structure one-for-one. Comments like
  `// === CPU: L += β * Ld * V ===` help enormously when debugging.
- Russian roulette termination: easy on GPU, keep deterministic seed
  policy.

**Acceptance test.** Cornell box with an area light, 512 spp. Parity
diff vs CPU NEE: MSE < 1e-2 (Monte Carlo noise makes perfect parity
impossible; you're comparing *expectation*, not variance).

**Time estimate.** 8–12 hours.

---

## Step 6 — MIS integrator on the GPU

**Goal.** Port the (currently stubbed on the CPU side — see
`cpu/include/integrator/maxdepth_mis.hpp`) MIS integrator. At each
bounce, sample both BSDF and light, weight by power heuristic, add.

**Concepts learned.**
- Why MIS helps variance-wise — visible in the noise pattern of the
  converged image.
- Balancing payload size against multi-tech integrators.
- When the **CPU version is broken or absent**, the GPU version is
  your *only* reference — great lesson in writing the algorithm from
  the textbook rather than copying.

**Prereqs.** Step 5.

**Implementation hints.**
- Implement MIS on the CPU first (finish the stub) so you have your
  oracle.
- The power-2 heuristic is `w = f² / (f² + g²)`.
- Watch for the `pdf == 0` branches on both the light and BSDF sides.

**Acceptance test.** Glossy sphere under area light, 256 spp. Noise
should be visibly *lower* than NEE-only at the same spp count.
Parity diff vs CPU MIS: MSE < 1e-2.

**Time estimate.** 6–10 hours (including finishing the CPU MIS stub).

---

## Step 7 — Offline mode with tile scheduling

**Goal.** Add an "offline" mode to the GPU renderer that renders N spp
*without* the viewer, writes an EXR, and exits. This is what your
parity harness actually drives.

**Concepts learned.**
- Multiple launches accumulating into the same buffer.
- Tile scheduling when a single launch would exceed GPU watchdog
  timeouts (Windows TDR — a real trap).
- Reading back a buffer to the host and writing EXR/PNG.

**Prereqs.** Step 6.

**Implementation hints.**
- Add a `Renderer::renderOffline(spp, filepath)` that launches in a
  loop of `sppPerLaunch = 4` (or 1) to avoid TDR.
- `cudaDeviceSynchronize` between launches, or at least before the
  readback.
- Use `tinyexr` or `stb_image_write` for PNG (you already have the
  latter on the CPU side).

**Acceptance test.** `mypt.exe --offline cornell.json --spp 1024 -o
out.exr` runs without window, writes a valid EXR, completes in
reasonable time. Parity harness can now be automated in CI.

**Time estimate.** 3–5 hours.

---

## Step 8 — Microfacet (conductor + dielectric)

**Goal.** Add GGX conductor + dielectric to the `MaterialKind` enum.
Implement `eval/sample/pdf` matching your CPU microfacet code.

**Concepts learned.**
- Growing the tagged union without blowing up register pressure.
- Dielectric's Fresnel + refraction branch — a classic divergence
  source. Measure it.
- When to graduate to **per-material `OWLGeomType`** (separate CH
  programs). You probably *don't* need to yet, but you can benchmark
  the switch version vs a two-geomType version.

**Prereqs.** Step 6 (MIS matters more for glossy).

**Implementation hints.**
- Keep helper functions `ggx_D`, `ggx_G`, `fresnel_schlick` in a
  shared `gpu/src/BsdfUtils.h` — both CPU-test and device code
  include it.
- Unit tests from Step 2 extend trivially: add rows for the new
  materials.
- Dielectric: sample-side must handle the total-internal-reflection
  case without branching the calling code into chaos.

**Acceptance test.** Your favourite CPU test scene with a glass
sphere + copper sphere. Parity MSE < 2e-2 (slightly looser — GGX is
numerically fussy).

**Time estimate.** 10–14 hours.

---

## Step 9 — HDRI env-map with CDF importance sampling

**Goal.** Replace the constant miss background with an HDR
environment map, importance-sampled by a 2D CDF.

**Concepts learned.**
- Building and uploading a 2D CDF on the host, querying it on the
  device.
- Why env-map sampling is "free" for most integrators once you have
  NEE + MIS infrastructure.
- `cudaTextureObject_t` with **float** format (`cudaChannelFormatKindFloat`).

**Prereqs.** Steps 4, 6.

**Implementation hints.**
- Build the luminance CDF on the CPU once, upload as two buffers
  (marginal + conditional). Query with a binary search `__device__`
  helper.
- The env-map sample *also* needs a visibility check — it's a light
  like any other; reuse Step 3 shadow rays.
- Add the env-map PDF to MIS.

**Acceptance test.** A glossy dragon under an outdoor HDRI. Diff vs
CPU: MSE < 3e-2. Visually: the bright sun in the env map should cast
a believable shadow on the ground.

**Time estimate.** 8–12 hours.

---

## Step 10 — Instancing via TLAS

**Goal.** Render 100 instances of the same mesh with different
transforms, sharing one BLAS.

**Concepts learned.**
- The BLAS / TLAS split (finally!).
- Per-instance SBT offset (for per-instance materials without
  duplicating meshes).
- How instancing collapses memory usage and build time.

**Prereqs.** Any mesh-loading step (you already have one).

**Implementation hints.**
- Build one BLAS from the mesh. Build a TLAS with N instances, each
  with its own `float[12]` transform.
- Per-instance material is done via the instance's SBT offset —
  OWL exposes this as part of the instance setup.
- Hit program reads its own material via
  `optixGetInstanceId()` + a material-by-instance buffer.

**Acceptance test.** 100 copies of the Stanford bunny on a grid, each
a different colour. Runs at > 30 fps at 1080p on your GPU. Parity vs
a CPU version of the same scene: MSE < 2e-2.

**Time estimate.** 6–8 hours.

---

## Beyond: Disney, volumes, ReSTIR

Past Step 10, you're no longer learning the OWL/OptiX stack — you're
learning *advanced rendering*, the same material as your CPU code
supports but implemented with the patterns you've now internalised.

Rough difficulty ordering:

1. **Disney BSDF port.** ~2 weeks. Straight extension of Step 8. The
   interesting GPU concept is the `union`-of-layered-BRDFs memory
   layout and keeping registers under budget.
2. **Homogeneous volumes.** ~1 week. Teaches distance-sampling
   integrators and how to re-enter the integrator loop cleanly.
3. **ReSTIR DI.** ~2–3 weeks. Teaches *temporal reuse* and
   *spatial reuse* patterns — the real GPU-specific thing. You'll
   finally write a CUDA kernel that isn't an OptiX program.
4. **Wavefront / sorted wavefront path tracing.** ~1 month. The
   production move: replace megakernel pathtracing with per-stage
   kernels and stream compaction. This is the design Jakob et al.
   and modern game engines use.

Each of these is a separate project; don't tackle them until Step 10
works smoothly.

---

## Realistic timeline

Assuming you're a motivated student with ~10 h/week:

| Phase             | Steps  | Weeks | Outcome                                         |
| ----------------- | ------ | ----- | ----------------------------------------------- |
| Foundation        | 0–2    | 2     | First BSDF working with parity                  |
| Shading           | 3–4    | 2     | Shadow rays + textures, a recognisable scene    |
| Full integrator   | 5–6    | 3     | NEE + MIS matching CPU, convincingly            |
| Production shape  | 7      | 1     | Offline renders, CI-able parity                 |
| Advanced shading  | 8–9    | 3     | Glossy + env-map: it *looks* like a renderer    |
| Instancing        | 10     | 1     | Real scene complexity                           |
| **Total to "I have a GPU path tracer"** | 0–10 | **≈ 12 weeks** | |

This is a serious project. It is also ~2× faster than learning OptiX
from scratch without OWL.

---

## What NOT to do

- **Don't skip the parity harness.** Every person who does ends up
  debugging by eye and burns a week finding a bug that would have
  shown up as a 15 % MSE spike.
- **Don't port more than one concept per step.** The integrator is
  complicated enough without simultaneously debugging SBT layout.
- **Don't introduce `SceneTranslator` or other bridging layers
  yet.** Keep CPU and GPU branches parallel and minimally shared,
  as you chose. A translator can come *after* Step 10 if you find
  yourself duplicating scene-walking logic.
- **Don't optimise before Step 10.** `switch` material dispatch,
  megakernel integrator, no stream compaction. If an experienced
  renderer dev tells you "that's slow", say "I know, I'm learning
  first".
- **Don't ignore `compute-sanitizer` smells.** A single
  "invalid global read" can mask as a weird colour shift that
  looks like a BSDF bug for a week.
- **Don't lose the habit of annotating.** Step 0's habit — one
  comment per unfamiliar API call — is the habit that keeps the
  code legible as it grows to 10k lines.

Good luck. When something goes wrong, your CPU renderer is the
oracle, `compute-sanitizer` is the x-ray, the parity diff image is
the stethoscope. That's all the tools you need.
