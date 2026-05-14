# Feature Roadmap / TODO

## CODE LEVEL
* ✅ Refactor code for dependency management
* ✅ Add std::variant to unify ConstantTexture and vec3 during construction

## TEST LEVEL
* ✅ Verify obj loader correctness
* ✅ Add a shared Mitsuba XML validation scene for CPU/GPU alignment
* ✅ Add CPU/GPU XML alignment smoke script
* ✅ Add CPU/GPU high-spp convergence smoke script with MAE/RMSE thresholds
* Keep expanding validation scenes beyond Cornell box
* Add image diff / false-color diagnostic output for failed convergence checks

## COMMON / SCENE IO
* ✅ Add shared `pt::RenderSettings` for aligned CPU/GPU CLI parameters
* ✅ Add shared `pt::SceneDesc` for backend-agnostic scene loading
* ✅ Move Mitsuba XML subset parsing into common code
* ✅ Lower shared scene descriptions into CPU and GPU backend scenes
* Extend XML support for more Mitsuba BSDFs, transforms, textures, and emitters

## GPU BACKEND - OWL / OptiX
* ✅ Add optional GPU backend behind `PATHTRACER_BUILD_GPU`
* ✅ Set up OWL context, module, raygen, miss, closest-hit, launch params, and SBT
* ✅ Split shared GPU PODs into focused headers (`material.h`, `light.h`, `geometryData.h`, `launchParams.h`, `rayTypes.h`)
* ✅ Store per-geometry `materialId` in the SBT and keep `MaterialGPU` in a global launch-param buffer
* ✅ Use a thin radiance closest-hit shader that fills compact PRD hit data
* ✅ Move shading to raygen-side BSDF device functions with `MaterialGPU.kind` dispatch
* ✅ Add Lambertian GPU BxDF
* ✅ Add Mirror GPU BxDF
* ✅ Add radiance / shadow ray types
* ✅ Add binary shadow visibility tracing
* ✅ Add flat GPU light buffer and first quad-light sampler
* ✅ Add initial direct-light sampling in raygen
* ✅ Add fixed-frame benchmark mode for Nsight profiling
* ✅ Add CUDA-event frame timing / primary-ray throughput logging
* ✅ Add headless PNG output path for validation renders
* ✅ Add configurable gamma / tone-map output transform
* ✅ Add optional OptiX denoiser post-process
* ✅ Add GPU debug output modes (`normal`, `albedo`, `visibility`, `material-id`, `light-id`)
* Implement smooth Dielectric GPU BxDF
* Add direct-light MIS with BSDF pdf and power heuristic
* Generalize GPU light sampling beyond one quad-light path
* Add texture / normal-map support on GPU
* Add per-primitive material ids or mesh splitting for multi-material meshes
* Profile current raygen kernel with Nsight Compute and track register pressure / occupancy
* Consider per-material closest-hit shaders only if material dispatch becomes a measured bottleneck
* Consider wavefront path tracing after material, light, and visibility paths are stable

## CPU BACKEND
* ✅ Add Mitsuba XML scene loading through shared scene description parser
* ✅ Add CLI render settings aligned with the GPU backend
* ✅ Add CPU debug output modes (`normal`, `albedo`, `visibility`, `material-id`, `light-id`)
* Keep CPU NEE integrator as a validation reference for simple surface scenes
* Improve CPU material debug albedo extraction beyond the current BSDF probe fallback

## RESTIR DI
* ✅ Add ReSTIR DI implementation roadmap in `docs/restir_di_roadmap.md`
* ✅ Add GPU reservoir/sample POD structs and per-pixel reservoir buffers
* ✅ Add `--direct-light nee|restir` backend selection
* ✅ Implement per-pixel RIS without temporal or spatial reuse
* ✅ Validate no-reuse ReSTIR against current NEE on Cornell box XML
* Add ReSTIR debug views (`reservoir-weight`, `reservoir-m`, `reservoir-target`, `restir-light-id`)
* Add temporal reuse after no-reuse RIS is stable
* Add spatial reuse after temporal rejection/debugging is stable

## FEATURE LEVEL - Light
* ✅ Environment Light
   * Importance sampling to HDRI

## FEATURE LEVEL - Material
* ✅ Interface extension
* ✅ Rough dielectric
* ✅ Adding detailed dielectric interface
   * Per channel refract (i.e. dispersion)
* ✅ Disney Principled BSDF 2012 / 2015 Ver.
* Add Kajiya-Kay Material
* Layered BSDF model
   * Some BSSRDF Maybe

## FEATURE LEVEL - Medium
* ✅ One Global Homogeneous Medium with Absorption
* Homogeneous Medium with Single Scattering
* Homogeneous Medium with Multiple Scattering
* Heterogeneous Medium with BDPT

## FEATURE LEVEL - Interface
* ✅ No longer need a separated lightlist explicitly, light discovery will be done in the scene

## FEATURE LEVEL - Rendering Method
* NEE + MIS+Power Heuristic
* BDPT

## FEATURE LEVEL - Sampler
* ✅ Halton Sampler
* ✅ Stratified Sampling
* ✅ Scrambled Halton Sampler
* Sobol sampler