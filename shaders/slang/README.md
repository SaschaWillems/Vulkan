# Slang shaders

This folder contains shaders using the [Slang shading language](https://github.com/shader-slang/slang/releases).

These require a different SPIR-V environment than glsl/hlsl. When selecting slang shaders, the base requirement for all samples is raised to at least Vulkan 1.1 with the SPIRV 1.4 extension.

If you want to compile these shaders to SPIR-V, please use the latest release from [here](https://github.com/shader-slang/slang/releases) to get the latest bug fixes and features required for some of the samples. Minimum version for all shader to properly compile is `2025.16.1`.