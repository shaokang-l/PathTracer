# Path Tracer

An NEE + MIS path tracer, supports various BxDFs / Lights / Volumes and Monte Carlo effects. Supports both CPU and GPU (Optix) backend.

<img src="images/hdri.png" alt="image" style="zoom: 105%;" />

> Rough glass (alpha = 0.05) bunny with HDRI lighting



<img src="images/dielectric.png" alt="image" style="zoom: 67%;" />

> Rough glass bunny with Microfacet Conductor background 



<img src="https://s2.loli.net/2025/05/09/iXeMfBgGxCU1t6r.png" alt="output" style="zoom:77%;" />

>  Disney Principled Bunny with Microfacet Conductor background



<img src="images/sample.png" alt="image" style="zoom:110%;" />

> Left: Rough gold material, Right: Marschner Hair material



<img src="images/box_volume.png" alt="image" style="zoom:110%;" />

> Cornell box with homogeneous fog, rendered with volumetric integrator.



## Backends

The project has two renderer backends:

* `cpu/` is the feature-rich CPU path tracer with the original material,
  light, sampler, and volume systems.
* `gpu/` is an experimental OWL/OptiX backend used to port the renderer to
  CUDA/RT cores incrementally.

The GPU backend currently favors a clear architecture over premature kernel
specialization. OptiX closest-hit programs build a compact PRD, raygen owns the
path-tracing loop, and BSDF sampling is dispatched through GPU-friendly POD
materials instead of CPU virtual functions:

```text
radiance trace
  -> TriangleMesh closest-hit fills hitP / normal / materialId
  -> raygen fetches MaterialGPU from the global material buffer
  -> BSDF wrapper dispatches by MaterialGPU.kind
  -> optional direct-light sample shoots a shadow ray
```

The SBT currently stores geometry buffers plus `materialId`; per-frame data,
materials, and lights live in launch params / device buffers so they can be
updated without rebuilding the SBT. Per-material closest-hit programs and
wavefront scheduling are intentionally deferred until profiling shows that
material dispatch or raygen kernel size is a real bottleneck.

## Build Instructions:

This project uses CMake to build and is correctly built under M1 macOS environment. The C++ standard is set to C++ 20.

Once CMake is installed, use below commands to build with CMake.

This project uses OpenMP for parallelization, for ARM Mac users, please refer to this [post](https://stackoverflow.com/questions/71061894/how-to-install-openmp-on-mac-m1).

Add GL_SIMD to compile definition to enable SIMD (for vec and matrix class).

```
mkdir build
cd build
cmake ..
make .
```

* Unit tests are under `/tests` folder using `GTest` framework.

The GPU backend is optional and is disabled by default:

```bash
cmake -S . -B build -DPATHTRACER_BUILD_GPU=ON
cmake --build build --target mypt --config Release
```

When profiling the GPU backend, `mypt` also accepts `--frames N` so external
profilers can run a deterministic number of frames and exit.



### Integrator:

* The path tracer uses an importance sampling strategy, a mixed PDF of Material PDF and light sampling with NEE (next-event-estimation).

* Other integrator includes

  * Volumetric path tracer (only null-tracking for now. NEE WIP)
  * An analytical calculation for Polygonal diffuse only lighting. (Ref. James Arvo)
  * Russian Roulette version MIS, w./w.o. NEE
  
  

### Monte Carlo and Post-processing effects:

The path tracer supports DoF, motion blur. Image filtering and tone-mapping.



### Materials:

1. Marschner Hair
2. Phong
3. Dielectric (Microfacet BxDF + simple dispersion approximation)
   * Thin Dielectric 
   * This branch introduces a split-ray variant for noise reduction.
4. Conductor (Microfacet BRDF, VNDF)
5. Lambertian
6. Kajiya-Kay
7. Disney Principled BSDF (A mix of 2015 / 2012 impl.)



### Object IO

1. .obj
2. .fbx (WIP)



### Light

* Area light
* Sphere light
* HDRI (IBL)



### Sampler

1. Halton Sampler
2. Stratified Sampler
3. Sobol Sampler (WIP)
