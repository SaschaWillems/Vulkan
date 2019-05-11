# Vulkan C++ examples and demos

A comprehensive collection of open source C++ examples for [Vulkan®](https://www.khronos.org/vulkan/), the new graphics and compute API from Khronos.

[![Donate](https://img.shields.io/badge/Donate-PayPal-green.svg)](https://www.paypal.com/cgi-bin/webscr?cmd=_s-xclick&hosted_button_id=BHXPMV6ZKPH9E)

## Table of Contents
+ [Cloning](#Cloning)
+ [Assets](#Assets)
+ [Building](#Building)
+ [Examples](#Examples)
    + [Basics](#Basics)
    + [Advanced](#Advanced)
    + [Performance](#Performance)
    + [Physically Based Rendering](#PBR)
    + [Deferred](#Deferred)
    + [Compute Shader](#ComputeShader)
    + [Geometry Shader](#GeometryShader)
    + [Tessellation Shader](#TessellationShader)
    + [Headless](#Headless)
    + [User Interface](#UserInterface)
    + [Effects](#Effects)
    + [Extensions](#Extensions)
    + [Misc](#Misc)
+ [Credits and Attributions](#CreditsAttributions)


## <a name="Cloning"></a> Cloning
This repository contains submodules for external dependencies, so when doing a fresh clone you need to clone recursively:

```
git clone --recursive https://github.com/SaschaWillems/Vulkan.git
``` 

Existing repositories can be updated manually:

```
git submodule init
git submodule update
```

## <a name="Assets"></a> Assets
Many examples require assets from the asset pack that is not part of this repository due to file size. A python script is included to download the asset pack that. Run

    python download_assets.py

from the root of the repository after cloning or see [this](data/README.md) for manual download.

## <a name="Building"></a> Building

The repository contains everything required to compile and build the examples on <img src="./images/windowslogo.png" alt="" height="22px" valign="bottom"> Windows, <img src="./images/linuxlogo.png" alt="" height="24px" valign="bottom"> Linux, <img src="./images/androidlogo.png" alt="" height="24px" valign="bottom"> Android, <img src="./images/applelogo.png" alt="" valign="bottom" height="24px"> iOS and macOS (using MoltenVK) using a C++ compiler that supports C++11.

See [BUILD.md](BUILD.md) for details on how to build for the different platforms.

## <a name="Examples"></a> Examples

### <a name="Basics"></a> Basics

#### [01 - Triangle](examples/triangle/)
Basic and verbose example for getting a colored triangle rendered to the screen using Vulkan. This is meant as a starting point for learning Vulkan from the ground up. A huge part of the code is boilerplate that is abstracted away in later examples.

#### [02 - Pipelines](examples/pipelines/)

Using pipeline state objects (pso) that bake state information (rasterization states, culling modes, etc.) along with the shaders into a single object, making it easy for an implementation to optimize usage (compared to OpenGL's dynamic state machine). Also demonstrates the use of pipeline derivatives.

#### [03 - Descriptor sets](examples/descriptorsets)

Descriptors are used to pass data to shader binding points. Sets up descriptor sets, layouts, pools, creates a single pipeline based on the set layout and renders multiple objects with different descriptor sets.

#### [04 - Dynamic uniform buffers](examples/dynamicuniformbuffer/)

Dynamic uniform buffers are used for rendering multiple objects with multiple matrices stored in a single uniform buffer object. Individual matrices are dynamically addressed upon descriptor binding time, minimizing the number of required descriptor sets.

#### [05 - Push constants](examples/pushconstants/)

Uses push constants, small blocks of uniform data stored within a command buffer, to pass data to a shader without the need for uniform buffers.

#### [06 - Specialization constants](examples/specializationconstants/)

Uses SPIR-V specialization constants to create multiple pipelines with different lighting paths from a single "uber" shader.

#### [07 - Texture mapping](examples/texture/)

Loads a 2D texture from disk (including all mip levels), uses staging to upload it into video memory and samples from it using combined image samplers.

#### [08 - Cube map textures](examples/texturecubemap/)

Loads a cube map texture from disk containing six different faces. All faces and mip levels are uploaded into video memory and the cubemap is sampled once as a skybox (for the background) and as a source for reflections (for a 3D model).

#### [09 - Texture arrays](examples/texturearray/)

Loads a 2D texture array containing multiple 2D texture slices (each with it's own mip chain) and renders multiple meshes each sampling from a different layer of the texture. 2D texture arrays don't do any interpolation between the slices.

#### [10 - 3D textures](examples/texture3d/)

Generates a 3D texture on the cpu (using perlin noise), uploads it to the device and samples it to render an animation. 3D textures store volumetric data and interpolate in all three dimensions.

#### [11 - Model rendering](examples/mesh/)

Loads a 3D model and texture maps from a common file format (using [assimp](https://github.com/assimp/assimp)), uploads the vertex and index buffer data to video memory, sets up a matching vertex layout and renders the 3D model.

#### [12 - Input attachments](examples/inputattachments)

Uses input attachments to read framebuffer contents from a previous sub pass at the same pixel position within a single render pass. This can be used for basic post processing or image composition ([blog entry](https://www.saschawillems.de/tutorials/vulkan/input_attachments_subpasses)).

#### [13 - Sub passes](examples/subpasses/)

Advanced example that sses sub passes and input attachments to write and read back data from framebuffer attachments (same location only) in single render pass. This is used to implement deferred render composition with added forward transparency in a single pass. 

#### [14 - Offscreen rendering](examples/offscreen/)

Basic offscreen rendering in two passes. First pass renders the mirrored scene to a separate framebuffer with color and depth attachments, second pass samples from that color attachment for rendering a mirror surface.

#### [15 - CPU particle system](examples/particlefire/)

Implements a simple CPU based particle system. Particle data is stored in host memory, updated on the CPU per-frame and synchronized with the device before it's rendered using pre-multiplied alpha.

#### [16 - Stencil buffer](examples/stencilbuffer/)

Uses the stencil buffer and it's compare functionality for rendering a 3D model with dynamic outlines.

### <a name="Advanced"></a> Advanced

#### [01 - Scene rendering](examples/scenerendering/)

Combines multiple techniques to render a complex scene consisting of multiple meshes, textures and materials. Meshes are stored and rendered from a single buffer using vertex offsets. Material parameters are passed via push constants, and separate per-model and scene descriptor sets are used to pass data to the shaders.

#### [02 - Multi sampling](examples/multisampling/)

Implements multisample anti-aliasing (MSAA) using a renderpass with multisampled attachments and resolve attachments that get resolved into the visible frame buffer.

#### [03 - High dynamic range](examples/hdr/)

Implements a high dynamic range rendering pipeline using 16/32 bit floating point precision for all internal formats, textures and calculations, including a bloom pass, manual exposure and tone mapping. 

#### [04 - Shadow mapping](examples/shadowmapping/)

Rendering shadows for a directional light source. First pass stores depth values from the light's pov, second pass compares against these to check if a fragment is shadowed. Uses depth bias to avoid shadow artifacts and applies a PCF filter to smooth shadow edges.

#### [05 - Cascaded shadow mapping](examples/shadowmappingcascade/)

Uses multiple shadow maps (stored as a layered texture) to increase shadow resolution for larger scenes. The camera frustum is split up into multiple cascades with corresponding layers in the shadow map. Layer selection for shadowing depth compare is then done by comparing fragment depth with the cascades' depths ranges.

#### [06 - Omnidirectional shadow mapping](examples/shadowmappingomni/)

Uses a dynamic floating point cube map to implement shadowing for a point light source that casts shadows in all directions. The cube map is updated every frame and stores distance to the light source for each fragment used to determine if a fragment is shadowed.

#### [07 - Run-time mip-map generation](examples/texturemipmapgen/)

Generating a complete mip-chain at runtime instead of loading it from a file, by blitting from one mip level, starting with the actual texture image, down to the next smaller size until the lower 1x1 pixel end of the mip chain.

#### [08 - Skeletal animation](examples/skeletalanimation/)

Loads and renders an animated skinned 3D model. Skinning is done on the GPU by passing per-vertex bone weights and translation matrices. 

#### [09 - Capturing screenshots](examples/screenshot/)

Capturing and saving an image after a scene has been rendered using blits to copy the last swapchain image from optimal device to host local linear memory, so that it can be stored into a ppm image.

### <a name="Performance"></a> Performance

#### [01 - Multi threaded command buffer generation](examples/multithreading/)

Multi threaded parallel command buffer generation. Instead of prebuilding and reusing the same command buffers this sample uses multiple hardware threads to demonstrate parallel per-frame recreation of secondary command buffers that are executed and submitted in a primary buffer once all threads have finished.

#### [02 - Instancing](examples/instancing/)

Uses the instancing feature for rendering many instances of the same mesh from a single vertex buffer with variable parameters and textures (indexing a layered texture). Instanced data is passed using a secondary vertex buffer.

#### [03 - Indirect drawing](examples/indirectdraw/)

Rendering thousands of instanced objects with different geometry using one single indirect draw call instead of issuing separate draws. All draw commands to be executed are stored in a dedicated indirect draw buffer object (storing index count, offset, instance count, etc.) that is uploaded to the device and sourced by the indirect draw command for rendering.

#### [04 - Occlusion queries](examples/occlusionquery/)

Using query pool objects to get number of passed samples for rendered primitives got determining on-screen visibility.

#### [05 - Pipeline statistics](examples/pipelinestatistics/)

Using query pool objects to gather statistics from different stages of the pipeline like vertex, fragment shader and tessellation evaluation shader invocations depending on payload.

### <a name="PBR"></a> Physically Based Rendering

Physical based rendering as a lighting technique that achieves a more realistic and dynamic look by applying approximations of bidirectional reflectance distribution functions based on measured real-world material parameters and environment lighting.

#### [01 - PBR basics](examples/pbrbasic/)

Demonstrates a basic specular BRDF implementation with solid materials and fixed light sources on a grid of objects with varying material parameters, demonstrating how metallic reflectance and surface roughness affect the appearance of pbr lit objects.

#### [02 - PBR image based lighting](examples/pbribl/)

Adds image based lighting from an hdr environment cubemap to the PBR equation, using the surrounding environment as the light source. This adds an even more realistic look the scene as the light contribution used by the materials is now controlled by the environment. Also shows how to generate the BRDF 2D-LUT and irradiance and filtered cube maps from the environment map.

#### [03 - Textured PBR with IBL](examples/pbrtexture/)

Renders a model specially crafted for a metallic-roughness PBR workflow with textures defining material parameters for the PRB equation (albedo, metallic, roughness, baked ambient occlusion, normal maps) in an image based lighting environment.

### <a name="Deferred"></a> Deferred

These examples use a [deferred shading](https://en.wikipedia.org/wiki/Deferred_shading) setup.

#### [01 - Deferred shading basics](examples/deferred/)

Uses multiple render targets to fill all attachments (albedo, normals, position, depth) required for a G-Buffer in a single pass. A deferred pass then uses these to calculate shading and lighting in screen space, so that calculations only have to be done for visible fragments independent of no. of lights.

#### [02 - Deferred multi sampling](examples/deferredmultisampling/)

Adds multi sampling to a deferred renderer using manual resolve in the fragment shader.

#### [03 - Deferred shading shadow mapping](examples/deferredshadows/)

Adds shadows from multiple spotlights to a deferred renderer using a layered depth attachment filled in one pass using multiple geometry shader invocations.

#### [04 - Screen space ambient occlusion](examples/ssao/)

Adds ambient occlusion in screen space to a 3D scene. Depth values from a previous deferred pass are used to generate an ambient occlusion texture that is blurred before being applied to the scene in a final composition path.

### <a name="ComputeShader"></a> Compute Shader

#### [01 - Image processing](examples/computeshader/)

Uses a compute shader along with a separate compute queue to apply different convolution kernels (and effects) on an input image in realtime.

#### [02 - GPU particle system](examples/computeparticles/)

Attraction based 2D GPU particle system using compute shaders. Particle data is stored in a shader storage buffer and only modified on the GPU using memory barriers for synchronizing compute particle updates with graphics pipeline vertex access.

#### [03 - N-body simulation](examples/computenbody/)

N-body simulation based particle system with multiple attractors and particle-to-particle interaction using two passes separating particle movement calculation and final integration. Shared compute shader memory is used to speed up compute calculations.

#### [04 - Ray tracing](examples/computeraytracing/)

Simple GPU ray tracer with shadows and reflections using a compute shader. No scene geometry is rendered in the graphics pass.

#### [05 - Cloth simulation](examples/computecloth/)

Mass-spring based cloth system on the GPU using a compute shader to calculate and integrate spring forces, also implementing basic collision with a fixed scene object.

#### [06 - Cull and LOD](examples/computecullandlod/)

Purely GPU based frustum visibility culling and level-of-detail system. A compute shader is used to modify draw commands stored in an indirect draw commands buffer to toggle model visibility and select it's level-of-detail based on camera distance, no calculations have to be done on and synced with the CPU.

### <a name="GeometryShader"></a> Geometry Shader

#### [01 - Normal debugging](examples/geometryshader/)

Visualizing per-vertex model normals (for debugging). First pass renders the plain model, second pass uses a geometry shader to generate colored lines based on per-vertex model normals,

#### [02 - Viewport arrays](examples/viewportarray/)

Renders a scene to multiple viewports in one pass using a geometry shader to apply different matrices per viewport to simulate stereoscopic rendering (left/right). Requires a device with support for ```multiViewport```.

### <a name="TessellationShader"></a> Tessellation Shader

#### [01 - Displacement mapping](examples/tessellation/)

Uses a height map to dynamically generate and displace additional geometric detail for a low-poly mesh.

#### [02 - Dynamic terrain tessellation](examples/terraintessellation/)

Renders a terrain using tessellation shaders for height displacement (based on a 16-bit height map), dynamic level-of-detail (based on triangle screen space size) and per-patch frustum culling.

#### [03 - Model tessellation](examples/tessellation/)

Uses curved PN-triangles ([paper](http://alex.vlachos.com/graphics/CurvedPNTriangles.pdf)) for adding details to a low-polygon model.

### <a name="Headless"></a> Headless 

Examples that run one-time tasks and don't make use of visual output (no window system integration). These can be run in environments where no user interface is available ([blog entry](https://www.saschawillems.de/tutorials/vulkan/headless_examples)).

#### [01 - Render](examples/renderheadless)

Renders a basic scene to a (non-visible) frame buffer attachment, reads it back to host memory and stores it to disk without any on-screen presentation, showing proper use of memory barriers required for device to host image synchronization.

#### [02 - Compute](examples/computeheadless)

Only uses compute shader capabilities for running calculations on an input data set (passed via SSBO). A fibonacci row is calculated based on input data via the compute shader, stored back and displayed via command line.

### <a name="UserInterface"></a> User Interface

#### [01 - Text rendering](examples/textoverlay/)

Load and render a 2D text overlay created from the bitmap glyph data of a [stb font file](https://nothings.org/stb/font/). This data is uploaded as a texture and used for displaying text on top of a 3D scene in a second pass.

#### [02 - Distance field fonts](examples/distancefieldfonts/)

Uses a texture that stores signed distance field information per character along with a special fragment shader calculating output based on that distance data. This results in crisp high quality font rendering independent of font size and scale.

#### [03 - ImGui overlay](examples/imgui/)

Generates and renders a complex user interface with multiple windows, controls and user interaction on top of a 3D scene. The UI is generated using [Dear ImGUI](https://github.com/ocornut/imgui) and updated each frame.

### <a name="Effects"></a> Effects

#### [01 - Fullscreen radial blur](examples/radialblur/)

Demonstrates the basics of fullscreen shader effects. The scene is rendered into an offscreen framebuffer at lower resolution and rendered as a fullscreen quad atop the scene using a radial blur fragment shader.

#### [02 - Bloom](examples/bloom/)

Advanced fullscreen effect example adding a bloom effect to a scene. Glowing scene parts are rendered to a low res offscreen framebuffer that is applied atop the scene using a two pass separated gaussian blur.

#### [03 - Parallax mapping](examples/parallaxmapping/)

Implements multiple texture mapping methods to simulate depth based on texture information: Normal mapping, parallax mapping, steep parallax mapping and parallax occlusion mapping (best quality, worst performance).

#### [04 - Spherical environment mapping](examples/sphericalenvmapping/)

Uses a spherical material capture texture array defining environment lighting and reflection information to fake complex lighting. 

### <a name="Extensions"></a> Extensions

#### [01 - Conservative rasterization (VK_EXT_conservative_rasterization)](examples/conservativeraster/)

Uses conservative rasterization to change the way fragments are generated by the gpu. The example enables overestimation to generate fragments for every pixel touched instead of only pixels that are fully covered ([blog post](https://www.saschawillems.de/tutorials/vulkan/conservative_rasterization)).

#### [02 - Push descriptors (VK_KHR_push_descriptor)](examples/pushdescriptors/)

Uses push descriptors apply the push constants concept to descriptor sets. Instead of creating per-object descriptor sets for rendering multiple objects, this example passes descriptors at command buffer creation time.

#### [03 - Inline uniform blocks (VK_EXT_inline_uniform_block)](examples/inlineuniformblocks/)

Makes use of inline uniform blocks to pass uniform data directly at descriptor set creation time and also demonstrates how to update data for those descriptors at runtime.

#### [04 - Multiview rendering (VK_KHR_multiview)](examples/multiview/)

Renders a scene to to multiple views (layers) of a single framebuffer to simulate stereoscopic rendering in one pass. Broadcasting to the views is done in the vertex shader using ```gl_ViewIndex```.

#### [05 - Conditional rendering (VK_EXT_conditional_rendering)](examples/conditionalrender)

Demonstrates the use of VK_EXT_conditional_rendering to conditionally dispatch render commands based on values from a dedicated buffer. This allows e.g. visibility toggles without having to rebuild command buffers ([blog post](https://www.saschawillems.de/tutorials/vulkan/conditional_rendering)).

#### [06 - Debug markers (VK_EXT_debug_marker)](examples/debugmarker/)

Uses the VK_EXT_debug_marker extension to set debug markers, regions and to name Vulkan objects for advanced debugging in graphics debuggers like [RenderDoc](https://www.renderdoc.org). Details can be found in [this tutorial](https://www.saschawillems.de/tutorials/vulkan/vk_ext_debug_marker).

#### [07 - Negative viewport height (VK_KHR_Maintenance1 or Vulkan 1.1)](examples/negativeviewportheight/)

Shows how to render a scene using a negative viewport height, making the Vulkan render setup more similar to other APIs like OpenGL. Also has several options for changing relevant pipeline state, and displaying meshes with OpenGL or Vulkan style coordinates. Details can be found in [this tutorial](https://www.saschawillems.de/tutorials/vulkan/flipping-viewport).

#### [08 - Basic ray tracing with VK_NV_ray_tracing](examples/nv_ray_tracing_basic)

Basic example for doing ray tracing using the new Nvidia RTX extensions. Shows how to setup acceleration structures, ray tracing pipelines and the shaders needed to do the actual ray tracing.

#### [09 - Ray traced shadows with VK_NV_ray_tracing](examples/nv_ray_tracing_shadows)

Adds ray traced shadows casting using the new Nvidia RTX extensions to a more complex scene. Shows how to add multiple hit and miss shaders and how to modify existing shaders to add shadow calculations.

#### [10 - Ray traced reflections with VK_NV_ray_tracing](examples/nv_ray_tracing_reflections)

Renders a complex scene with reflective surfaces using using the new Nvidia RTX extensions. Shows how to do recursion inside of the ray tracing shaders for implementing real time reflections.

### <a name="Misc"></a> Misc

#### [01 - Vulkan Gears](examples/gears/)

Vulkan interpretation of glxgears. Procedurally generates and animates multiple gears.

#### [02 - Vulkan demo scene](examples/vulkanscene/)

Renders a Vulkan demo scene with logos and mascots. Not an actual example but more of a playground and showcase.

## <a name="CreditsAttributions"></a> Credits and Attributions
See [CREDITS.md](CREDITS.md) for additional credits and attributions.
