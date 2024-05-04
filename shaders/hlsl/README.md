## HLSL Shaders

This directory contains shaders using the [HLSL shading language](https://docs.vulkan.org/guide/latest/hlsl.html). These can be compiled with [DXC](https://github.com/microsoft/DirectXShaderCompiler) using e.g. the included `compile.py` script.

### Known issues

- Specialization constants can't be used to specify array size.
- `gl_PointCoord` not supported. HLSL has no equivalent. We changed the shaders to calulate the PointCoord manually in the shader. (`computenbody`, `computeparticles`, `particlesystem` examples).
- HLSL doesn't have inverse operation (`deferred`, `hdr`, `instancing`, `skeletalanimation` & `texturecubemap` examples), these should be done on the CPU
- HLSL interface for sparse residency textures is different from GLSL interface. After translating from HLSL to GLSL the shaders behave slightly different. Most important parts do behave identically though.