# Vulkan examples on Android

## <img src="./../images/androidlogo.png" alt="" height="32px"> Vulkan on Android

Since Vulkan is not yet part of the Android OS (like OpenGL ES) the library and function pointers need to be dynamically loaded before using any of the Vulkan functions. See the **vulkanandroid.h** and **vulkanandroid.cpp** files in the base folder of the repositoy root for how this is done.

## Device support
- **To run these examples you need a device with an Android image that suports Vulkan**
- Builds currently only support arm-v7, x86 may follow at a later point
- Android TV leanback launcher is supported, so the examples will show up on the launcher
- Basic gamepad support is available too (zoom and rotate)
- Basic touch control support (zoom, move, rotate, look)

## Building

### Requirements
- [Android NDK r11b](http://developer.android.com/ndk/downloads/index.html) (or newer) - Somewhere in your search path
- Examples are built against API level 23 (requires the SDK Platform installed)
- Python 3.x

### Building the Examples

### Complete set

**Please note that building (and deploying) all examples may take a while**

#### Build only

```
build-all.py
```

This will build all apks and puts them into the **bin** folder.

#### Build and deploy

```
install-all.py
```

This will build all apks and deploys them to the currently attached android device.

### Single examples

These are for building and/or deploying a single example.

#### Build only

Call build(.bat) with the name of the example to build, e.g. :

```
build.py pbrtexture
```

This will build the apk for the triangle example and puts it into the **bin** folder.

#### Build and deploy

```
build.py pbrtexture -deploy
```

This will build the apk for the triangle example and deploys it to the currently attached android device.

#### Validation layers

```
build.py pbrtexture -validation (-deploy)
```

Builds the apk, adds the validation layer libraries and enables validation via a compiler define.

**Note**: You need to manually build the validation layers and put them in the [proper folder](layers/). If the libaries are not present they won't be included with the apk and running the app will fail.

## Removing

A single file for removing all installed examples is provided in case you installed all of them and don't want to remove them by hand (which is especially tedious on Android TV).


```
uninstall-all.py
```

This will remove any installed Android example from this repository from the attached device.
