# Vulkan examples and demos

<img src="./images/vulkanlogoscene.png" alt="Vulkan demo scene" height="256px">

Future place for examples and demos for Khronos' new 3D and compute API [Vulkan(tm)](https://www.khronos.org/vulkan)


**Sources will be available when Vulkan is released to the public. Current ETA is early 2016**.

See the [Vulkan Working Group Update](https://www.khronos.org/vulkan) for details.

## Vulkan from my point-of-view

I recently [did a write-up](http://www.saschawillems.de/?p=1886) with my personal view on Vulkan for a hobby developer. It goes into detail on some of the most important things to consider when deciding on how to switch over from Vulkan and also clears up some things that several press articles got wrong.

## Building
The repository contains a CMakeLists.txt to be used with [CMake](https://cmake.org).

Use it to generate a platform-specific build configuration for building all examples. It should work with different compilers on Windows and Linux (C++11 required).

All required headers and libs are included in the repository, building the examples should work out of the box.

## Examples

### Triangle
----
<img src="./screenshots/basic_triangle.png" height="96px" align="right">

Most basic example. Renders a colored triangle using an indexed vertex buffer, only one pipeline with very simple shaders. Uses a single uniform buffer for the matrices.

This example won't make use of helper functions or initializers (like the other examples) and is much more of an explicit example then the others included in this repository. It contains lot's of boiler plate that you'd usually encapsulate in helper functions and classes.

### Texture mapping
----
<img src="./screenshots/basic_texture.png" height="96px" align="right">

Loads a single texture and displays it on a simple quad. Demonstrates loading a texture to host visible memory (linear tiling) and transforming it into an optimal (tiling) format for the GPU, including upload of available mip map levels.

### Cubemap
----
<img src="./screenshots/texture_cubemap.png" height="96px" align="right">

Building on the basic texture loading example a cubemap is loaded into host visible memory and then transformed into an optimal format for the GPU.
The demo uses two different pipelines (and shader sets) to display the cubemap as a skybox (background) and as a source for reflections.

### Texture array
----
<img src="./screenshots/texture_array.png" height="96px" align="right">

Texture arrays allow storing of multiple images in different layers without any interpolation between the layers.
This example demonstrates the use of a 2D texture array with instanced rendering. Each instance samples from a different layer of the texture array.

### Distance field fonts
----
<img src="./screenshots/font_distancefield.png" height="96px" align="right">

Instead of just sampling a bitmap font texture, a texture with per-character signed distance fields is used to generate high quality glyphs in the fragment shader. This results in a much higher quality than common bitmap fonts, even if heavily zoomed.

Distance field font textures can be generated with tools like [Hiero](https://github.com/libgdx/libgdx/wiki/Hiero).

### Pipelines
----
<img src="./screenshots/basic_pipelines.png" height="96px" align="right">

Pipelines replace the huge (and cumbersome) state machine of OpenGL. This example creates different pipelines with different states and shader setups.

### Gears
----
<img src="./screenshots/basic_gears.png" height="96px" align="right">

Vulkan interpretation of glxgears. Procedurally generates separate meshes for each gear, with every mesh having it's own uniform buffer object for animation. Also demonstrates how to use different descriptor sets.

### Mesh loading and rendering
----
<img src="./screenshots/basic_mesh.png" height="96px" align="right">

Uses [assimp](https://github.com/assimp/assimp) to load and a mesh from a common 3D format including a color map. The mesh data is then converted to a fixed vertex layout matching the pipeline (and shader) layout descriptions.

### Mesh instancing
----
<img src="./screenshots/instancing.png" height="96px" align="right">

Shows the use of instancing for rendering the same mesh with differing uniforms with one single draw command. This saves performance if the same mesh has to be rendered multiple times.

### Push constants
----
<img src="./screenshots/push_constants.png" height="96px" align="right">

Demonstrates the use of push constants for updating small blocks of shader data with high speed (and without having to use a uniform buffer). Displays several light sources with position updates through a push constant block in a separate command buffer.

### Offscreen rendering
----
<img src="./screenshots/basic_offscreen.png" height="96px" align="right">

Uses a separate framebuffer (that is not part of the swap chain) and a texture target for offscreen rendering. The texture is then used as a mirror.

### Radial blur
----
<img src="./screenshots/radial_blur.png" height="96px" align="right">

Demonstrates basic usage of fullscreen shader effects. The scene is rendered offscreen first, gets blitted to a texture target and for the final draw this texture is blended on top of the 3D scene with a radial blur shader applied.

### Bloom
----
<img src="./screenshots/bloom.png" height="96px" align="right">

Implements a bloom effect to simulate glowing parts of a 3D mesh. A two pass gaussian blur (horizontal and then vertical) is used to generate a blurred low res version of the scene only containing the glowing parts of th the 3D mesh. This then gets blended onto the scene to add the blur effect.

### Deferred shading
----
<img src="./screenshots/deferred_shading.png" height="96px" align="right">

Demonstrates the use of multiple render targets to fill a G-Buffer for deferred shading.

Deferred shading collects all values (color, normal, position) into different render targets in one pass thanks to multiple render targets, and then does all shading and lighting calculations based on these in scree space, thus allowing for much more light sources than traditional forward renderers.

### Omnidirectional shadow mapping
----
<img src="./screenshots/shadow_omnidirectional.png" height="96px" align="right">

Uses a dynamic 32 bit floating point cube map for a point light source that casts shadows in all directions (unlike projective shadow mapping).
The cube map faces contain thee distances from the light sources, which are then used in the scene rendering pass to determine if the fragment is shadowed or not.

### Spherical environment mapping
----
<img src="./screenshots/spherical_env_mapping.png" height="96px" align="right">

Uses a matcap texture (spherical reflection map) to fake complex lighting. It's based on [this article](https://github.com/spite/spherical-environment-mapping).

### Parallax mapping
----
<img src="./screenshots/parallax_mapping.jpg" height="96px" align="right">

Like normal mapping, parallax mapping simulates geometry on a flat surface. In addition to normal mapping a heightmap is used to offset texture coordinates depending on the viewing angle giving the illusion of added depth.

### (Tessellation shader) PN-Triangles
----
<img src="./screenshots/tess_pntriangles.jpg" height="96px" align="right">

Generating curved PN-Triangles on the GPU using tessellation shaders to add details to low-polygon meshes, based on [this paper](http://alex.vlachos.com/graphics/CurvedPNTriangles.pdf).

### (Tessellation shader) Displacement mapping
----
<img src="./screenshots/tess_displacement.jpg" height="96px" align="right">

Uses tessellation shaders to generate and displace geometry based on a displacement map (heightmap).

### (Compute shader) Particle system
----
<img src="./screenshots/compute_particles.png" height="96px" align="right">

Attraction based particle system. A shader storage buffer is used to store particle data and updated by a compute shader. The buffer is then used by the graphics pipeline for rendering.

### (Compute shader) Image processing
----
<img src="./screenshots/compute_imageprocessing.png" height="96px" align="right">

Demonstrates the use of a separate compute queue (and command buffer) to apply different convolution kernels on an input image.

### (Geometry shader) Normal debugging
----
<img src="./screenshots/geom_normals.png" height="96px" align="right">

Renders the vertex normals of a complex mesh with the use of a geometry shader. The mesh is rendered solid first and the a geometry shader that generates lines from the face normals is used in the second pass.

### Vulkan demo scene
----
<img src="./screenshots/vulkan_scene.png" height="96px" align="right">

More of a playground than an actual example. Renders multiple meshes with different shaders (and pipelines) including a background.

## Additional documentation
- [Vulkan example base class](./documentation/examplebaseclass.md)

## Dependencies
*Note*: Included in the repository
- [OpenGL Mathematics (GLM)](https://github.com/g-truc/glm)
- [OpenGL Image (GLI)](https://github.com/g-truc/gli)
- [Open Asset Import Library](https://github.com/assimp/assimp)
- Vulkan Headers (not yet available)

## Attributions
- Cubemap used in cubemap example by [Emil Persson(aka Humus)](http://www.humus.name/)
- Armored knight model by [Gabriel Piacenti](http://opengameart.org/users/piacenti)
- Voyager model by [NASA](http://nasa3d.arc.nasa.gov/models)

## External resources
- [Official list of Vulkan resources](https://www.khronos.org/vulkan/resources)
- [SPIR-V specifications](https://www.khronos.org/registry/spir-v/specs/1.0/SPIRV.html)
