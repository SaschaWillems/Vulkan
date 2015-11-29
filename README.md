# Vulkan examples and demos

<img src="./images/vulkanlogoscene.png" alt="Vulkan demo scene" height="256px">

**Future place for examples and demos for Khronos' new 3D and compute API [Vulkan(tm)](https://www.khronos.org/vulkan)**

## Vulkan from my point-of-view

I recently [did a write-up](http://www.saschawillems.de/?p=1886) with my personal view on Vulkan for a hobby developer. It goes into detail on some of the most important things to consider when deciding on how to switch over from Vulkan and also clears up some things that several press articles got wrong.

## Vulkan example base class
All examples are derived from a base class that encapsulates common used Vulkan functionality and all the setup stuff that's not necessary to repeat for each example. It also contains functions to load shaders and an easy wrapper to enable debugging via the validation layers.

If you want to create an example based on this base class, simply derive :

```cpp
#include "vulkanexamplebase.h"
...
class MyVulkanExample : public VulkanExampleBase
{
  ...
  VulkanExample()
  {
    width = 1024;
    height = 1024;
    zoom = -15;
    rotation = glm::vec3(-45.0, 22.5, 0.0);
    title = "My new Vulkan Example";
  }
}
```
##### Validation layers
The example base class offers a constructor overload for enabling a default set of Vulkan validation layers (for debugging purposes). If you want to use this functionality, simple use the construtor override :
```cpp
VulkanExample() : VulkanExampleBase(true)
{
  ...
}
```



*todo* : Document helper classes like vulkandebug

## Examples
*todo* : In progress

### Triangle
<img src="./screenshots/basic_triangle.png" height="128px">

Most basic example. Renders a colored triangle using an indexed vertex buffer, only one pipeline with very simple shaders. Uses a single uniform buffer for the matrices.

### Texture
<img src="./screenshots/basic_texture.png" height="128px">

Loads a single texture and displays it on a simple quad.

### Pipelines
<img src="./screenshots/basic_pipelines.png" height="128px">

Pipelines replace the huge (and cumbersome) state machine of OpenGL. This example creates different pipelines with different states and shader setups.

### Gears
<img src="./screenshots/basic_gears.png" height="128px">

Vulkan interpretation of glxgears. Procedurally generates separate meshes for each gear, with every mesh having it's own uniform buffer object for animation. Also demonstrates how to use different descriptor sets.

### Mesh rendering
<img src="./screenshots/basic_mesh.png" height="128px">

Uses [assimp](https://github.com/assimp/assimp) to load and a mesh from a common 3D format. The mesh data is then converted to a fixed vertex layout matching a basic set of shaders.

### Mesh instancing
<img src="./screenshots/instancing.png" height="128px">

Renders hundreds of meshes using instances with uniforms for e.g. coloring each mesh separately.

### Offscreen rendering
<img src="./screenshots/basic_offscreen.png" height="128px">

Uses a separate framebuffer (that is not part of the swap chain) for rendering a 3D scene off screen and blits it into a texture target displayed on a quad. The blit does scaling and (if required) also format conversions.

### Radial blur
<img src="./screenshots/radial_blur.png" height="128px">

Demonstrates basic usage of fullscreen shader effects. The scene is rendered offscreen first, gets blitted to texture target and for the final draw this texture is blended on top of the 3D scene with a radial blur shader applied.

### Spherical environment mapping
<img src="./screenshots/spherical_env_mapping.png" height="128px">

Uses a matcap texture (spherical reflection map) to fake complex lighting. It's based on [this article](https://github.com/spite/spherical-environment-mapping).

### Parallax mapping
<img src="./screenshots/parallax_mapping.jpg" height="128px">

Like normal mapping, parallax mapping simulates geometry on a flat surface. In addition to normal mapping a heightmap is used to offset texture coordinates depending on the viewing angle giving the illusion of added depth.

### (Tessellation shader) PN-Triangles
<img src="./screenshots/tess_pntriangles.jpg" height="128px">

Generating curved PN-Triangles on the GPU using tessellation shaders to add details to low-polygon meshes, based on [this paper](http://alex.vlachos.com/graphics/CurvedPNTriangles.pdf).

### (Tessellation shader) Displacement mapping
<img src="./screenshots/tess_displacement.jpg" height="128px">

Uses tessellation shaders to generate and displace geometry based on a displacement map (heightmap).

### (Geometry shader) Normal debugging
<img src="./screenshots/geom_normals.png" height="128px">

Renders the vertex normals of a complex mesh with the use of a geometry shader. The mesh is rendered solid first and the a geometry shader that generates lines from the face normals is used in the second pass.

### Vulkan demo scene
<img src="./screenshots/vulkan_scene.png" height="128px">

More of a playground than an actual example. Renders multiple meshes with different shaders (and pipelines) including a background.


## Dependencies
*Note*: Not all demos require all of these
- [OpenGL Mathematics (GLM)](https://github.com/g-truc/glm)
- [OpenGL Image (GLI)](https://github.com/g-truc/gli)
- [Open Asset Import Library](https://github.com/assimp/assimp)
- Vulkan Headers (not yet available)

## External resources
*TODO : In progress*
- [Official list of Vulkan resources](https://www.khronos.org/vulkan/resources)
