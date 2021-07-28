# Vulkan C++ examples and demos

A comprehensive collection of open source C++ examples for [Vulkan®](https://www.khronos.org/vulkan/), the new generation graphics and compute API from Khronos.

[![Donate](https://img.shields.io/badge/Donate-PayPal-green.svg)](https://www.paypal.com/cgi-bin/webscr?cmd=_s-xclick&hosted_button_id=BHXPMV6ZKPH9E)

## Table of Contents
+ [Official Khronos Vulkan Samples](#official-khronos-vulkan-samples)
+ [Cloning](#Cloning)
+ [Assets](#Assets)
+ [Building](#Building)
+ [Running](#Running)
+ [Shaders](#Shaders)
+ [Examples](#Examples)
    + [Basics](#Basics)
    + [glTF](#glTF)
    + [Advanced](#Advanced)
    + [Performance](#Performance)
    + [Physically Based Rendering](#physically-based-rendering)
    + [Deferred](#Deferred)
    + [Compute Shader](#compute-shader)
    + [Geometry Shader](#geometry-shader)
    + [Tessellation Shader](#tessellation-shader)
    + [Hardware accelerated ray tracing](#hardware-accelerated-ray-tracing)
    + [Headless](#Headless)
    + [User Interface](#user-interface)
    + [Effects](#Effects)
    + [Extensions](#Extensions)
    + [Misc](#Misc)
+ [Credits and Attributions](#credits-and-attributions)

## Official Khronos Vulkan Samples

Khronos recently made an official Vulkan Samples repository available to the public ([press release](https://www.khronos.org/blog/vulkan-releases-unified-samples-repository?utm_source=Khronos%20Blog&utm_medium=Twitter&utm_campaign=Vulkan%20Repository)).

You can find this repository at https://github.com/KhronosGroup/Vulkan-Samples

As I've been involved with getting the official repository up and running, I'll be mostly contributing to that repository from now, but may still add samples that don't fit there in here and I'll of course continue to maintain these samples.

## Cloning
This repository contains submodules for external dependencies, so when doing a fresh clone you need to clone recursively:

```
git clone --recursive https://github.com/SaschaWillems/Vulkan.git
```

Existing repositories can be updated manually:

```
git submodule init
git submodule update
```

## Assets
Many examples require assets from the asset pack that is not part of this repository due to file size. A python script is included to download the asset pack that. Run

    python download_assets.py

from the root of the repository after cloning or see [this](data/README.md) for manual download.

## Building

The repository contains everything required to compile and build the examples on <img src="./images/windowslogo.png" alt="" height="22px" valign="bottom"> Windows, <img src="./images/linuxlogo.png" alt="" height="24px" valign="bottom"> Linux, <img src="./images/androidlogo.png" alt="" height="24px" valign="bottom"> Android, <img src="./images/applelogo.png" alt="" valign="bottom" height="24px"> iOS and macOS (using MoltenVK) using a C++ compiler that supports C++11.

See [BUILD.md](BUILD.md) for details on how to build for the different platforms.

## Running

Once built, examples can be run from the bin directory. The list of available command line options can be brought up with `--help`:
```
 -v, --validation: Enable validation layers
 -br, --benchruntime: Set duration time for benchmark mode in seconds
 -vs, --vsync: Enable V-Sync
 -w, --width: Set window width
 -f, --fullscreen: Start in fullscreen mode
 --help: Show help
 -h, --height: Set window height
 -bt, --benchframetimes: Save frame times to benchmark results file
 -s, --shaders: Select shader type to use (glsl or hlsl)
 -b, --benchmark: Run example in benchmark mode
 -g, --gpu: Select GPU to run on
 -bf, --benchfilename: Set file name for benchmark results
 -gl, --listgpus: Display a list of available Vulkan devices
 -bw, --benchwarmup: Set warmup time for benchmark mode in seconds
```

Note that some examples require specific device features, and if you are on a multi-gpu system you might need to use the `-gl` and `-g` to select a gpu that supports them.

## Shaders

Vulkan consumes shaders in an intermediate representation called SPIR-V. This makes it possible to use different shader languages by compiling them to that bytecode format. The primary shader language used here is [GLSL](data/shaders/glsl) but thanks to an external contribution you'll also find [HLSL](data/shaders/hlsl) shader sources.

## Examples

### Basics

#### [First triangle](examples/triangle/)
Basic and verbose example for getting a colored triangle rendered to the screen using Vulkan. This is meant as a starting point for learning Vulkan from the ground up. A huge part of the code is boilerplate that is abstracted away in later examples.

#### [Pipelines](examples/pipelines/)

Using pipeline state objects (pso) that bake state information (rasterization states, culling modes, etc.) along with the shaders into a single object, making it easy for an implementation to optimize usage (compared to OpenGL's dynamic state machine). Also demonstrates the use of pipeline derivatives.

#### [Descriptor sets](examples/descriptorsets)

Descriptors are used to pass data to shader binding points. Sets up descriptor sets, layouts, pools, creates a single pipeline based on the set layout and renders multiple objects with different descriptor sets.

#### [Dynamic uniform buffers](examples/dynamicuniformbuffer/)

Dynamic uniform buffers are used for rendering multiple objects with multiple matrices stored in a single uniform buffer object. Individual matrices are dynamically addressed upon descriptor binding time, minimizing the number of required descriptor sets.

#### [Push constants](examples/pushconstants/)

Uses push constants, small blocks of uniform data stored within a command buffer, to pass data to a shader without the need for uniform buffers.

#### [Specialization constants](examples/specializationconstants/)

Uses SPIR-V specialization constants to create multiple pipelines with different lighting paths from a single "uber" shader.

#### [Texture mapping](examples/texture/)

Loads a 2D texture from disk (including all mip levels), uses staging to upload it into video memory and samples from it using combined image samplers.

#### [Texture arrays](examples/texturearray/)

Loads a 2D texture array containing multiple 2D texture slices (each with its own mip chain) and renders multiple meshes each sampling from a different layer of the texture. 2D texture arrays don't do any interpolation between the slices.

#### [Cube map textures](examples/texturecubemap/)

Loads a cube map texture from disk containing six different faces. All faces and mip levels are uploaded into video memory, and the cubemap is displayed on a skybox as a backdrop and on a 3D model as a reflection.

#### [Cube map arrays](examples/texturecubemaparray/)

Loads an array of cube map textures from a single file. All cube maps are uploaded into video memory with their faces and mip levels, and the selected cubemap is displayed on a skybox as a backdrop and on a 3D model as a reflection.

#### [3D textures](examples/texture3d/)

Generates a 3D texture on the cpu (using perlin noise), uploads it to the device and samples it to render an animation. 3D textures store volumetric data and interpolate in all three dimensions.

#### [Input attachments](examples/inputattachments)

Uses input attachments to read framebuffer contents from a previous sub pass at the same pixel position within a single render pass. This can be used for basic post processing or image composition ([blog entry](https://www.saschawillems.de/tutorials/vulkan/input_attachments_subpasses)).

#### [Sub passes](examples/subpasses/)

Advanced example that uses sub passes and input attachments to write and read back data from framebuffer attachments (same location only) in single render pass. This is used to implement deferred render composition with added forward transparency in a single pass.

#### [Offscreen rendering](examples/offscreen/)

Basic offscreen rendering in two passes. First pass renders the mirrored scene to a separate framebuffer with color and depth attachments, second pass samples from that color attachment for rendering a mirror surface.

#### [CPU particle system](examples/particlefire/)

Implements a simple CPU based particle system. Particle data is stored in host memory, updated on the CPU per-frame and synchronized with the device before it's rendered using pre-multiplied alpha.

#### [Stencil buffer](examples/stencilbuffer/)

Uses the stencil buffer and its compare functionality for rendering a 3D model with dynamic outlines.

### glTF

These samples show how implement different features of the [glTF 2.0 3D format](https://www.khronos.org/gltf/) 3D transmission file format in detail.

#### [glTF model loading and rendering](examples/gltfloading/)

Shows how to load a complete scene from a [glTF 2.0](https://github.com/KhronosGroup/glTF) file. The structure of the glTF 2.0 scene is converted into the data structures required to render the scene with Vulkan.

#### [glTF vertex skinning](examples/gltfskinning/)

Demonstrates how to do GPU vertex skinning from animation data stored in a [glTF 2.0](https://github.com/KhronosGroup/glTF) model. Along with reading all the data structures required for doing vertex skinning, the sample also shows how to upload animation data to the GPU and how to render it using shaders.

#### [glTF scene rendering](examples/gltfscenerendering/)

Renders a complete scene loaded from an [glTF 2.0](https://github.com/KhronosGroup/glTF) file. The sample is based on the glTF model loading sample, and adds data structures, functions and shaders required to render a more complex scene using Crytek's Sponza model with per-material pipelines and normal mapping.

### Advanced

#### [Multi sampling](examples/multisampling/)

Implements multisample anti-aliasing (MSAA) using a renderpass with multisampled attachments and resolve attachments that get resolved into the visible frame buffer.

#### [High dynamic range](examples/hdr/)

Implements a high dynamic range rendering pipeline using 16/32 bit floating point precision for all internal formats, textures and calculations, including a bloom pass, manual exposure and tone mapping.

#### [Shadow mapping](examples/shadowmapping/)

Rendering shadows for a directional light source. First pass stores depth values from the light's pov, second pass compares against these to check if a fragment is shadowed. Uses depth bias to avoid shadow artifacts and applies a PCF filter to smooth shadow edges.

#### [Cascaded shadow mapping](examples/shadowmappingcascade/)

Uses multiple shadow maps (stored as a layered texture) to increase shadow resolution for larger scenes. The camera frustum is split up into multiple cascades with corresponding layers in the shadow map. Layer selection for shadowing depth compare is then done by comparing fragment depth with the cascades' depths ranges.

#### [Omnidirectional shadow mapping](examples/shadowmappingomni/)

Uses a dynamic floating point cube map to implement shadowing for a point light source that casts shadows in all directions. The cube map is updated every frame and stores distance to the light source for each fragment used to determine if a fragment is shadowed.

#### [Run-time mip-map generation](examples/texturemipmapgen/)

Generating a complete mip-chain at runtime instead of loading it from a file, by blitting from one mip level, starting with the actual texture image, down to the next smaller size until the lower 1x1 pixel end of the mip chain.

#### [Capturing screenshots](examples/screenshot/)

Capturing and saving an image after a scene has been rendered using blits to copy the last swapchain image from optimal device to host local linear memory, so that it can be stored into a ppm image.

#### [Order Independent Transparency](examples/oit)

Implements order independent transparency based on linked lists. To achieve this, the sample uses storage buffers in combination with image load and store atomic operations in the fragment shader.

### Performance

#### [Multi threaded command buffer generation](examples/multithreading/)

Multi threaded parallel command buffer generation. Instead of prebuilding and reusing the same command buffers this sample uses multiple hardware threads to demonstrate parallel per-frame recreation of secondary command buffers that are executed and submitted in a primary buffer once all threads have finished.

#### [Instancing](examples/instancing/)

Uses the instancing feature for rendering many instances of the same mesh from a single vertex buffer with variable parameters and textures (indexing a layered texture). Instanced data is passed using a secondary vertex buffer.

#### [Indirect drawing](examples/indirectdraw/)

Rendering thousands of instanced objects with different geometry using one single indirect draw call instead of issuing separate draws. All draw commands to be executed are stored in a dedicated indirect draw buffer object (storing index count, offset, instance count, etc.) that is uploaded to the device and sourced by the indirect draw command for rendering.

#### [Occlusion queries](examples/occlusionquery/)

Using query pool objects to get number of passed samples for rendered primitives got determining on-screen visibility.

#### [Pipeline statistics](examples/pipelinestatistics/)

Using query pool objects to gather statistics from different stages of the pipeline like vertex, fragment shader and tessellation evaluation shader invocations depending on payload.

### Physically Based Rendering

Physical based rendering as a lighting technique that achieves a more realistic and dynamic look by applying approximations of bidirectional reflectance distribution functions based on measured real-world material parameters and environment lighting.

#### [PBR basics](examples/pbrbasic/)

Demonstrates a basic specular BRDF implementation with solid materials and fixed light sources on a grid of objects with varying material parameters, demonstrating how metallic reflectance and surface roughness affect the appearance of pbr lit objects.

#### [PBR image based lighting](examples/pbribl/)

Adds image based lighting from an hdr environment cubemap to the PBR equation, using the surrounding environment as the light source. This adds an even more realistic look the scene as the light contribution used by the materials is now controlled by the environment. Also shows how to generate the BRDF 2D-LUT and irradiance and filtered cube maps from the environment map.

#### [Textured PBR with IBL](examples/pbrtexture/)

Renders a model specially crafted for a metallic-roughness PBR workflow with textures defining material parameters for the PRB equation (albedo, metallic, roughness, baked ambient occlusion, normal maps) in an image based lighting environment.

### Deferred

These examples use a [deferred shading](https://en.wikipedia.org/wiki/Deferred_shading) setup.

#### [Deferred shading basics](examples/deferred/)

Uses multiple render targets to fill all attachments (albedo, normals, position, depth) required for a G-Buffer in a single pass. A deferred pass then uses these to calculate shading and lighting in screen space, so that calculations only have to be done for visible fragments independent of no. of lights.

#### [Deferred multi sampling](examples/deferredmultisampling/)

Adds multi sampling to a deferred renderer using manual resolve in the fragment shader.

#### [Deferred shading shadow mapping](examples/deferredshadows/)

Adds shadows from multiple spotlights to a deferred renderer using a layered depth attachment filled in one pass using multiple geometry shader invocations.

#### [Screen space ambient occlusion](examples/ssao/)

Adds ambient occlusion in screen space to a 3D scene. Depth values from a previous deferred pass are used to generate an ambient occlusion texture that is blurred before being applied to the scene in a final composition path.

### Compute Shader

#### [Image processing](examples/computeshader/)

Uses a compute shader along with a separate compute queue to apply different convolution kernels (and effects) on an input image in realtime.

#### [GPU particle system](examples/computeparticles/)

Attraction based 2D GPU particle system using compute shaders. Particle data is stored in a shader storage buffer and only modified on the GPU using memory barriers for synchronizing compute particle updates with graphics pipeline vertex access.

#### [N-body simulation](examples/computenbody/)

N-body simulation based particle system with multiple attractors and particle-to-particle interaction using two passes separating particle movement calculation and final integration. Shared compute shader memory is used to speed up compute calculations.

#### [Ray tracing](examples/computeraytracing/)

Simple GPU ray tracer with shadows and reflections using a compute shader. No scene geometry is rendered in the graphics pass.

#### [ Cloth simulation](examples/computecloth/)

Mass-spring based cloth system on the GPU using a compute shader to calculate and integrate spring forces, also implementing basic collision with a fixed scene object.

#### [Cull and LOD](examples/computecullandlod/)

Purely GPU based frustum visibility culling and level-of-detail system. A compute shader is used to modify draw commands stored in an indirect draw commands buffer to toggle model visibility and select its level-of-detail based on camera distance, no calculations have to be done on and synced with the CPU.

### Geometry Shader

#### [Normal debugging](examples/geometryshader/)

Visualizing per-vertex model normals (for debugging). First pass renders the plain model, second pass uses a geometry shader to generate colored lines based on per-vertex model normals,

#### [Viewport arrays](examples/viewportarray/)

Renders a scene to multiple viewports in one pass using a geometry shader to apply different matrices per viewport to simulate stereoscopic rendering (left/right). Requires a device with support for ```multiViewport```.

### Tessellation Shader

#### [Displacement mapping](examples/displacement/)

Uses a height map to dynamically generate and displace additional geometric detail for a low-poly mesh.

#### [Dynamic terrain tessellation](examples/terraintessellation/)

Renders a terrain using tessellation shaders for height displacement (based on a 16-bit height map), dynamic level-of-detail (based on triangle screen space size) and per-patch frustum culling.

#### [Model tessellation](examples/tessellation/)

Uses curved PN-triangles ([paper](http://alex.vlachos.com/graphics/CurvedPNTriangles.pdf)) for adding details to a low-polygon model.

### Hardware accelerated ray tracing

#### [Basic ray tracing](examples/raytracingbasic)

Basic example for doing hardware accelerated ray tracing using the ```VK_KHR_acceleration_structure``` and ```VK_KHR_ray_tracing_pipeline``` extensions. Shows how to setup acceleration structures, ray tracing pipelines and the shader binding table needed to do the actual ray tracing.

#### [Ray traced shadows](examples/raytracingshadows)

Adds ray traced shadows casting using the new ray tracing extensions to a more complex scene. Shows how to add multiple hit and miss shaders and how to modify existing shaders to add shadow calculations.

#### [Ray traced reflections](examples/raytracingreflections)

Renders a complex scene with reflective surfaces using the new ray tracing extensions. Shows how to do recursion inside of the ray tracing shaders for implementing real time reflections.

#### [Callable ray tracing shaders](examples/raytracingcallable)

Callable shaders can be dynamically invoked from within other ray tracing shaders to execute different shaders based on dynamic conditions. The example ray traces multiple geometries, with each calling a different callable shader from the closest hit shader.

#### [Ray query](examples/rayquery)

Ray queries add acceleration structure intersection functionality to non ray tracing shader stages. This allows for combining ray tracing with rasterization. This example makes uses ray queries to add ray casted shadows to a rasterized sample in the fragment shader.

### Headless

Examples that run one-time tasks and don't make use of visual output (no window system integration). These can be run in environments where no user interface is available ([blog entry](https://www.saschawillems.de/tutorials/vulkan/headless_examples)).

#### [Render](examples/renderheadless)

Renders a basic scene to a (non-visible) frame buffer attachment, reads it back to host memory and stores it to disk without any on-screen presentation, showing proper use of memory barriers required for device to host image synchronization.

#### [Compute](examples/computeheadless)

Only uses compute shader capabilities for running calculations on an input data set (passed via SSBO). A fibonacci row is calculated based on input data via the compute shader, stored back and displayed via command line.

### User Interface

#### [Text rendering](examples/textoverlay/)

Load and render a 2D text overlay created from the bitmap glyph data of a [stb font file](https://nothings.org/stb/font/). This data is uploaded as a texture and used for displaying text on top of a 3D scene in a second pass.

#### [Distance field fonts](examples/distancefieldfonts/)

Uses a texture that stores signed distance field information per character along with a special fragment shader calculating output based on that distance data. This results in crisp high quality font rendering independent of font size and scale.

#### [ImGui overlay](examples/imgui/)

Generates and renders a complex user interface with multiple windows, controls and user interaction on top of a 3D scene. The UI is generated using [Dear ImGUI](https://github.com/ocornut/imgui) and updated each frame.

### Effects

#### [Fullscreen radial blur](examples/radialblur/)

Demonstrates the basics of fullscreen shader effects. The scene is rendered into an offscreen framebuffer at lower resolution and rendered as a fullscreen quad atop the scene using a radial blur fragment shader.

#### [Bloom](examples/bloom/)

Advanced fullscreen effect example adding a bloom effect to a scene. Glowing scene parts are rendered to a low res offscreen framebuffer that is applied atop the scene using a two pass separated gaussian blur.

#### [Parallax mapping](examples/parallaxmapping/)

Implements multiple texture mapping methods to simulate depth based on texture information: Normal mapping, parallax mapping, steep parallax mapping and parallax occlusion mapping (best quality, worst performance).

#### [Spherical environment mapping](examples/sphericalenvmapping/)

Uses a spherical material capture texture array defining environment lighting and reflection information to fake complex lighting.

### Extensions

#### [Conservative rasterization (VK_EXT_conservative_rasterization)](examples/conservativeraster/)

Uses conservative rasterization to change the way fragments are generated by the gpu. The example enables overestimation to generate fragments for every pixel touched instead of only pixels that are fully covered ([blog post](https://www.saschawillems.de/tutorials/vulkan/conservative_rasterization)).

#### [Push descriptors (VK_KHR_push_descriptor)](examples/pushdescriptors/)

Uses push descriptors apply the push constants concept to descriptor sets. Instead of creating per-object descriptor sets for rendering multiple objects, this example passes descriptors at command buffer creation time.

#### [Inline uniform blocks (VK_EXT_inline_uniform_block)](examples/inlineuniformblocks/)

Makes use of inline uniform blocks to pass uniform data directly at descriptor set creation time and also demonstrates how to update data for those descriptors at runtime.

#### [Multiview rendering (VK_KHR_multiview)](examples/multiview/)

Renders a scene to to multiple views (layers) of a single framebuffer to simulate stereoscopic rendering in one pass. Broadcasting to the views is done in the vertex shader using ```gl_ViewIndex```.

#### [Conditional rendering (VK_EXT_conditional_rendering)](examples/conditionalrender)

Demonstrates the use of VK_EXT_conditional_rendering to conditionally dispatch render commands based on values from a dedicated buffer. This allows e.g. visibility toggles without having to rebuild command buffers ([blog post](https://www.saschawillems.de/tutorials/vulkan/conditional_rendering)).

#### [Debug markers (VK_EXT_debug_marker)](examples/debugmarker/)

<span style="color:red">This sample is deprecated</span>

An updated version using ```VK_EXT_debug_utils``` along with an in-depth tutorial is available in the [Official Khronos Vulkan Samples repository](https://github.com/KhronosGroup/Vulkan-Samples/blob/master/samples/extensions/debug_utils).

#### [Negative viewport height (VK_KHR_Maintenance1 or Vulkan 1.1)](examples/negativeviewportheight/)

Shows how to render a scene using a negative viewport height, making the Vulkan render setup more similar to other APIs like OpenGL. Also has several options for changing relevant pipeline state, and displaying meshes with OpenGL or Vulkan style coordinates. Details can be found in [this tutorial](https://www.saschawillems.de/tutorials/vulkan/flipping-viewport).

#### [Variable rate shading (VK_NV_shading_rate_image)](examples/variablerateshading/)

Uses a special image that contains variable shading rates to vary the number of fragment shader invocations across the framebuffer. This makes it possible to lower fragment shader invocations for less important/less noisy parts of the framebuffer.

#### [Descriptor indexing (VK_EXT_descriptor_indexing)](examples/descriptorindexing/)  

Demonstrates the use of VK_EXT_descriptor_indexing for creating descriptor sets with a variable size that can be dynamically indexed in a shader using `GL_EXT_nonuniform_qualifier` and `SPV_EXT_descriptor_indexing`.


### Misc

#### [Vulkan Gears](examples/gears/)

Vulkan interpretation of glxgears. Procedurally generates and animates multiple gears.

#### [Vulkan demo scene](examples/vulkanscene/)

Renders a Vulkan demo scene with logos and mascots. Not an actual example but more of a playground and showcase.

## Credits and Attributions
See [CREDITS.md](CREDITS.md) for additional credits and attributions.
