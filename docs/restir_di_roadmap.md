# ReSTIR DI Implementation Roadmap

This document is a practical roadmap for adding ReSTIR Direct Illumination
(ReSTIR DI) to this path tracer. The goal is to keep each step testable against
the current CPU/GPU validation loop before moving to the next algorithmic layer.

## Current Baseline

The project now has enough shared infrastructure to start ReSTIR DI work:

- CPU and GPU can load the same Mitsuba XML subset through a shared scene
  description.
- CPU and GPU render settings are parsed through `pt::RenderSettings`.
- Cornell box XML validation exists under `assets/validation/`.
- CPU/GPU convergence smoke scripts exist under `tools/`.
- CPU and GPU both support debug output modes through `--debug-view`:
  `beauty`, `normal`, `albedo`, `visibility`, `material-id`, `light-id`.

Before starting ReSTIR DI changes, keep this baseline green. If a later step
breaks `beauty`, `normal`, or `visibility`, fix that before continuing.

## Target Algorithm

ReSTIR DI replaces naive direct-light sampling with reservoir-based light
candidate sampling. At a high level:

1. Generate one or more direct-light candidates for each pixel.
2. Evaluate each candidate's contribution and target density.
3. Use reservoir importance sampling (RIS) to keep one representative candidate.
4. Reuse reservoirs temporally from the previous frame.
5. Reuse reservoirs spatially from neighboring pixels.
6. Evaluate the final chosen sample with visibility and bias-correction weights.

The first milestone should be "RIS without reuse". Temporal and spatial reuse
should only be added after the no-reuse version matches ordinary direct
lighting in simple scenes.

## Data Model

Start with small POD structs that can live on the GPU and have CPU mirrors for
debugging or validation.

Suggested reservoir state:

```cpp
struct RestirLightSample {
  int lightId;
  float2 uv;
  float pdf;
  float target;
  float3 position;
  float3 normal;
  float3 emission;
};

struct RestirReservoir {
  RestirLightSample y;
  float wSum;
  float W;
  uint32_t M;
};
```

Meaning:

- `y`: the selected candidate.
- `wSum`: sum of RIS candidate weights.
- `W`: final reservoir weight used during shading.
- `M`: number of candidates represented by the reservoir.

Keep the first version explicit and redundant. Do not over-compress the sample
state until the debug views are trustworthy.

## Phase 1: Direct Lighting Debug Buffers

Purpose: make direct-light terms observable before adding reservoirs.

Already done:

- `normal`
- `albedo`
- `visibility`
- `material-id`
- `light-id`

Recommended next debug views:

- `light-pdf`: visualize area or solid-angle PDF used by the sampled light.
- `light-distance`: visualize distance to the sampled light.
- `direct-radiance`: visualize direct-light contribution before BSDF sampling.
- `bsdf-f`: visualize BSDF value for the selected direct-light direction.
- `cosine`: visualize `NoL`, `NoI`, or geometry term.

Acceptance criteria:

- Debug output is deterministic at fixed camera and resolution.
- CPU/GPU `normal`, `visibility`, and `light-id` agree on simple XML scenes.
- Debug modes do not use temporal accumulation, denoising, or random jitter.

## Phase 2: Per-Pixel RIS, No Reuse

Purpose: replace one-sample direct lighting with local reservoir sampling, but
without temporal or spatial reuse.

Implementation steps:

1. Add a GPU-only reservoir buffer with one `RestirReservoir` per pixel.
2. Add a candidate generation function:
   - choose a light,
   - sample a point on that light,
   - compute light PDF,
   - compute candidate target value.
3. Add a reservoir update function:
   - candidate weight `w = target / sourcePdf`,
   - update `wSum`,
   - probabilistically replace selected sample.
4. Finalize the reservoir:
   - `W = wSum / max(target(y) * M, epsilon)` for the basic formulation.
5. Shade using the selected sample and visibility.

For the first version, use one candidate per pixel and no reuse. This should be
behaviorally close to the current direct-light path. Then raise candidates per
pixel to 4, 8, or 16 and verify noise decreases.

Acceptance criteria:

- With `initialCandidates=1`, output is close to current direct lighting.
- With more candidates, direct lighting becomes less noisy without biasing
  simple Cornell box results.
- Debug views can show `reservoir-weight`, `reservoir-m`, and selected
  `light-id`.

## Phase 3: Integrate Into Current GPU Path Tracer

Purpose: make ReSTIR DI a selectable direct-light backend inside the existing
path tracer.

Suggested CLI flags:

- `--direct-light nee`
- `--direct-light restir`
- `--restir-initial-candidates N`
- `--restir-temporal 0|1`
- `--restir-spatial-passes N`
- `--restir-spatial-neighbors N`

Keep the default as the current NEE path until ReSTIR is stable.

Implementation notes:

- Only replace the direct-light estimate at non-delta BSDF hits.
- Keep BSDF continuation unchanged.
- Disable denoiser while debugging ReSTIR buffers.
- Keep `beauty` and ReSTIR debug views separate from tone-mapping details.

Acceptance criteria:

- Existing GPU beauty path is unchanged when `--direct-light nee`.
- `--direct-light restir --restir-temporal 0 --restir-spatial-passes 0` renders
  correctly in Cornell box XML.
- CPU high-spp NEE can still be used as a reference for simple scenes.

## Phase 4: Temporal Reuse

Purpose: reuse the previous frame's reservoir through camera reprojection.

Required state:

- current reservoir buffer,
- previous reservoir buffer,
- previous camera matrices or enough camera data for reprojection,
- depth or primary hit position buffer,
- normal buffer for rejection.

Implementation steps:

1. Store primary hit position/depth and normal for each pixel.
2. Add previous-frame reservoir buffer.
3. Reproject current hit point to previous frame.
4. Reject temporal reuse when:
   - previous pixel is outside the framebuffer,
   - depth differs too much,
   - normal differs too much,
   - material/light context is incompatible.
5. Combine current and temporal reservoirs with reservoir merge logic.

Acceptance criteria:

- Static camera reduces noise over frames.
- Camera motion does not leave obvious ghost trails in simple scenes.
- Temporal reuse can be disabled and the output returns to Phase 2 behavior.

## Phase 5: Spatial Reuse

Purpose: reuse neighboring reservoirs within the current frame.

Implementation steps:

1. Ping-pong reservoir buffers for spatial passes.
2. Randomly sample neighbor pixels within a radius.
3. Reject neighbors with incompatible geometry:
   - large depth difference,
   - large normal difference,
   - invalid reservoir.
4. Merge accepted reservoirs.
5. Evaluate visibility for the final selected sample.

Suggested starting settings:

- `spatialPasses = 1`
- `spatialNeighbors = 4`
- small radius, such as 16 pixels

Acceptance criteria:

- One spatial pass reduces noise without obvious light leaking.
- Increasing spatial passes has predictable quality/performance tradeoff.
- Debug view for `reservoir-m` shows larger represented sample counts.

## Phase 6: Bias Handling

Purpose: make the estimator behavior clear and controllable.

Start with the biased ReSTIR DI variant because it is easier to implement and
debug. Once it is stable, add options for more correct weighting.

Things to document in code:

- Which target function is used.
- Which PDF is used for candidate generation.
- Whether visibility is included in the target or evaluated only at the end.
- Whether temporal/spatial reused samples are visibility-validated.
- Which Jacobian or reconnection terms are ignored.

Acceptance criteria:

- The selected estimator is named clearly in CLI or code comments.
- Known bias risks are documented.
- Simple scenes remain close to CPU high-spp reference.

## Phase 7: Multi-Light and Mesh-Light Scaling

Purpose: make ReSTIR DI useful beyond the Cornell box.

Implementation steps:

1. Build a robust light table from scene data.
2. Support many emissive triangles or mesh lights.
3. Add light selection distributions:
   - uniform light selection,
   - power-weighted light selection,
   - optional alias table.
4. Validate scenes with many small lights.

Acceptance criteria:

- ReSTIR DI clearly outperforms one-sample NEE in many-light scenes.
- `light-id` debug view remains meaningful.
- Light PDFs are correct and visible through debug modes.

## Validation Plan

Use three tiers of validation.

Tier 1: debug invariants

- `normal` matches CPU/GPU.
- `visibility` behaves as expected.
- selected `light-id` is stable in simple scenes.

Tier 2: no-reuse correctness

- ReSTIR with one candidate is close to current direct lighting.
- More candidates reduce variance.
- Results converge toward CPU high-spp reference on Cornell box XML.

Tier 3: reuse behavior

- Temporal reuse reduces static-camera noise.
- Spatial reuse reduces per-frame noise.
- Motion/reprojection rejection avoids obvious ghosting.

## Recommended Implementation Order

1. Add GPU reservoir structs and buffers.
2. Add `--direct-light restir` and keep it disabled by default.
3. Implement per-pixel RIS without reuse.
4. Add debug views:
   - `reservoir-weight`
   - `reservoir-m`
   - `reservoir-target`
   - `restir-light-id`
5. Validate against current NEE in Cornell box.
6. Add temporal reuse.
7. Add spatial reuse.
8. Add many-light scenes and light selection distributions.

Do not start temporal reuse until the no-reuse reservoir path is boringly
predictable. Most ReSTIR bugs become much harder to diagnose after history and
neighbor reuse are introduced.

