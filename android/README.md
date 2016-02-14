# Vulkan examples and demos - Android

## Building

The examples have been built using Visual Studio 2015 and require the Android NDK for building.

## Vulkan on Android

Since Vulkan is not yet part of the Android OS (like OpenGL ES) the library and function pointers need to be dynamically loaded before using any of the Vulkan functions. See the **vulkanandroid.h** and **vulkanandroid.cpp** files in the base folder of the repositoy root for how this is done.

**To run these examples you need a device with an Android image that suports Vulkan and has the libvulkan.so preinstalled!**

## Examples

### Triangle
----

### Texture mapping
----

### Mesh loading and rendering
----

### Compute shader particle system
----
