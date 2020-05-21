## HLSL Shaders

This directory contains a fork of the shaders found in [data/shaders](https://github.com/SaschaWillems/Vulkan/tree/master/data/shaders), re-written in HLSL.
These can be compiled with [DXC](https://github.com/microsoft/DirectXShaderCompiler) using the `compile.py` script.

### Status

Tested on Ubuntu 18.04 + GeForce RTX 2080 Ti


Shaders written to mirror the GLSL versions at `eddd724`. There have been changes made to the GLSL shaders since then, which will need to be updated.


| Name                       | GLSL @`eddd724e7` | HLSL @`eddd724e7` | GLSL @`10a1ecaf7` | HLSL @`10a1ecaf7`
|----------------------------|-------------------|-------------------|-------------------|-------------------
| bloom                      | &#9745;           | &#9745;           | &#9745;           | &#9745;
| computecloth               | &#9745;           | &#9745;           | &#9745;           | &#9745;
| computecullandlod          | &#9745;           | &#9745;           | &#9745;           | &#9745;
| computeheadless            | &#9745;           | &#9745;           | &#9745;           | &#9745;
| computenbody               | &#9745;           | &#9745;           | &#9745;           | &#9745;
| computeparticles           | &#9745;           | &#10060;          | &#9745;           | &#10060;
| computeraytracing          | &#9745;           | &#9745;           | &#9745;           | &#9745;
| computeshader              | &#9745;           | &#9745;           | &#9745;           | &#9745;
| conditionalrender          | &#9745;           | &#9745;           | &#9745;           | &#9745;
| conservativeraster         | &#9745;           | &#9745;           | &#9745;           | &#9745;
| debugmarker                | &#9745;           | &#9745;           | &#9745;           | &#9745;
| deferred                   | &#9745;           | &#9745;           | &#9745;           | &#9745;
| deferredmultisampling      | &#9745;           | &#9745;           | &#9745;           | &#9745;
| deferredshadows            | &#9745;           | &#9745;           | &#9745;           | &#9745;
| descriptorsets             | &#9745;           | &#9745;           | &#9745;           | &#9745;
| displacement               | &#9745;           | &#9745;           | &#9745;           | &#9745;
| distancefieldfonts         | &#9745;           | &#9745;           | &#9745;           | &#9745;
| dynamicuniformbuffer       | &#9745;           | &#9745;           | &#9745;           | &#9745;
| gears                      | &#9745;           | &#9745;           | &#9745;           | &#9745;
| geometryshader             | &#9745;           | &#9745;           | &#9745;           | &#9745;
| gltfscene                  | -                 | -                 | &#9745;           | &#10060;
| hdr                        | &#9745;           | &#10060;          | &#9745;           | &#10060;
| imgui                      | &#9745;           | &#9745;           | &#9745;           | &#9745;
| indirectdraw               | &#9745;           | &#9745;           | &#9745;           | &#9745;
| inlineuniformblocks        | &#9745;           | &#9745;           | &#9745;           | &#9745;
| inputattachments           | &#9745;           | &#9745;           | &#9745;           | &#9745;
| instancing                 | &#9745;           | &#9745;           | &#9745;           | &#9745;
| mesh                       | &#9745;           | &#9745;           | &#10060;          | &#9745;
| multisampling              | &#9745;           | &#9745;           | &#9745;           | &#10060;
| multithreading             | &#9745;           | &#9745;           | &#9745;           | &#9745;
| multiview                  | &#9745;           | &#9745;           | &#9745;           | &#9745;
| negativeviewportheight     | &#9745;           | &#9745;           | &#9745;           | &#9745;
| nv_ray_tracing_basic       | &#9745;           | &#9745;           | &#9745;           | &#9745;
| nv_ray_tracing_reflections | &#9745;           | &#9745;           | &#9745;           | &#9745;
| nv_ray_tracing_shadows     | &#9745;           | &#9745;           | &#9745;           | &#9745;
| occlusionquery             | &#9745;           | &#9745;           | &#9745;           | &#9745;
| offscreen                  | &#9745;           | &#9745;           | &#9745;           | &#10060;
| parallaxmapping            | &#9745;           | &#9745;           | &#9745;           | &#9745;
| particlefire               | &#9745;           | &#9745;           | &#9745;           | &#9745;
| pbrbasic                   | &#9745;           | &#9745;           | &#9745;           | &#9745;
| pbribl                     | &#9745;           | &#9745;           | &#9745;           | &#9745;
| pbrtexture                 | &#9745;           | &#9745;           | &#9745;           | &#9745;
| pipelines                  | &#9745;           | &#9745;           | &#9745;           | &#9745;
| pipelinestatistics         | &#9745;           | &#9745;           | &#9745;           | &#9745;
| pushconstants              | &#9745;           | &#9745;           | &#9745;           | &#9745;
| pushdescriptors            | &#9745;           | &#9745;           | &#9745;           | &#9745;
| radialblur                 | &#9745;           | &#9745;           | &#9745;           | &#9745;
| renderheadless             | &#9745;           | &#9745;           | &#9745;           | &#9745;
| scenerendering             | &#9745;           | &#9745;           | &#9745;           | &#9745;
| screenshot                 | &#9745;           | &#9745;           | &#9745;           | &#9745;
| shadowmapping              | &#9745;           | &#9745;           | &#9745;           | &#9745;
| shadowmappingcascade       | &#9745;           | &#9745;           | &#9745;           | &#9745;
| shadowmappingomni          | &#9745;           | &#9745;           | &#9745;           | &#9745;
| skeletalanimation          | &#9745;           | &#9745;           | &#9745;           | &#10060;
| specializationconstants    | &#9745;           | &#9745;           | &#9745;           | &#9745;
| sphericalenvmapping        | &#9745;           | &#9745;           | &#9745;           | &#9745;
| ssao                       | &#9745;           | &#9745;           | &#9745;           | &#9745;
| stencilbuffer              | &#9745;           | &#9745;           | &#9745;           | &#9745;
| subpasses                  | &#9745;           | &#9745;           | &#9745;           | &#9745;
| terraintessellation        | &#9745;           | &#9745;           | &#9745;           | &#9745;
| tessellation               | &#9745;           | &#9745;           | &#9745;           | &#9745;
| textoverlay                | &#9745;           | &#9745;           | &#9745;           | &#9745;
| texture                    | &#9745;           | &#9745;           | &#9745;           | &#9745;
| texture3d                  | &#9745;           | &#9745;           | &#9745;           | &#9745;
| texturearray               | &#9745;           | &#9745;           | &#9745;           | &#9745;
| texturecubemap             | &#9745;           | &#10060;          | &#9745;           | &#10060;
| texturemipmapgen           | &#9745;           | &#9745;           | &#9745;           | &#9745;
| texturesparseresidency     | &#9745;           | &#9745;           | &#9745;           | &#9745;
| triangle                   | &#9745;           | &#9745;           | &#9745;           | &#9745;
| viewportarray              | &#9745;           | &#9745;           | &#9745;           | &#9745;
| vulkanscene                | &#9745;           | &#9745;           | &#9745;           | &#9745;

### Known issues

- specialization constants can't be used to specify array size.
- `gl_PointCoord` not supported. HLSL has no equivalent. We changed the shaders to calulate the PointCoord manually in the shader. (`computenbody`, `computeparticles`, `particlefire` examples).
- HLSL doesn't have inverse operation (`deferred`, `hdr`, `instancing`, `skeletalanimation` & `texturecubemap` examples).
- `modf` causes compilation to fail without errors or warnings. (`modf` not used by any examples, easily confused with fmod)
- In `specializationconstants` example, shader compilation fails with error:
    ```
    --- Error msg: fatal error: failed to optimize SPIR-V: Id 10 is defined more than once
    ```
  When multiple constant ids are defined and have different types. We work around this problem by making all constant ids the same type, then use `asfloat`, `asint` or `asuint` to get the original value in the shader.
- `gl_RayTmaxNV` not supported. (`nv_ray_tracing_*` examples)
- HLSL interface for sparse residency textures is different from GLSL interface. After translating from HLSL to GLSL the shaders behave slightly different. Most important parts do behave identically though.