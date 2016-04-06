# Vulkan examples on Android

## Vulkan on Android

Since Vulkan is not yet part of the Android OS (like OpenGL ES) the library and function pointers need to be dynamically loaded before using any of the Vulkan functions. See the **vulkanandroid.h** and **vulkanandroid.cpp** files in the base folder of the repositoy root for how this is done.

## Device support
- **To run these examples you need a device with an Android image that suports Vulkan**
- Builds currently only support arm-v7, x86 may follow at a later point
- Android TV leanback launcher is supported, so the examples will show up on the launcher
- Basic gamepad support is available too (zoom and rotate)

## Imporant note

The Android part of the Vulkan example base is not yet finished, and while the examples run fine they might encounter some problems as most of the resources are not yet correctly released when closing the app.

## Building

### Requirements
- [Android NDK r11b](http://developer.android.com/ndk/downloads/index.html) - Somewhere in your sarch path
- Batch files for building are provided for windows only, with linux to be added at some point

### Building the Examples

### Complete set

**Please note that building (and deploying) all examples may take a while**

#### Build only

```
build-all
```

This will build all apks and puts them into the **bin** folder.

#### Build and deploy

```
install-all
```

This will build all apks and deploys them to the currently attached android device. 

### Single examples

These are for building and/or deploying a single example.

#### Build only

Call build(.bat) with the name of the example to build, e.g. :

```
build triangle
```

This will build the apk for the triangle example and puts it into the **bin** folder.

#### Build and deploy

```
build triangle -deploy
```

This will build the apk for the triangle example and deploys it to the currently attached android device. 

## Removing

A batch file for removing all installed examples is provided in case you installed all of them and don't want to remove them by hand (which is especially tedious on Android TV).


```
uninstall-all
```
 
This will remove any installed Android example from this repository from the attached device.
