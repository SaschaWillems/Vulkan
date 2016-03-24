# Vulkan examples on Android

## Vulkan on Android

Since Vulkan is not yet part of the Android OS (like OpenGL ES) the library and function pointers need to be dynamically loaded before using any of the Vulkan functions. See the **vulkanandroid.h** and **vulkanandroid.cpp** files in the base folder of the repositoy root for how this is done.

## Device support
- **To run these examples you need a device with an Android image that suports Vulkan**
- Builds currently only support arm-v7, x86 may follow at a later point
- Android TV leanback launcher is supported, examples will show up on the launcher
- Basic gamepad support is available too (zoom and rotate)

## Imporant note

I'm currently in the process of replacing the old (separate) Android examples, integrating Android support into the main line of examples. This is a work-in-progress, so the examples that are already converted may contain errors. Most notably they won't free the Vulkan resources for now.

## Building

### Requirements
- [Android NDK r11b](http://developer.android.com/ndk/downloads/index.html) - Somewhere in your sarch path
- Batch files for building are provided for windows only, with linux to be added at some point

### Building the Examples

Builds are started using the provided batch file for each example.

#### Build only

Call the corresponding .bat, call e.g. :

```
build-triangle.bat
```

This will build the apk and move it to the "bin" folder

#### Deploy

If you want to deploy to an attached Android device right after the build is done :

```
build-triangle.bat -deploy
```
