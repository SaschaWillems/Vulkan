## HLSL Shaders

This directory contains a fork of the shaders found in [data/shaders/glsl](https://github.com/SaschaWillems/Vulkan/tree/master/data/shaders/glsl), re-written in HLSL.
These can be compiled with [DXC](https://github.com/microsoft/DirectXShaderCompiler) using the `compile.py` script.

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