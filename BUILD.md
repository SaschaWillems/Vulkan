# Building

The repository contains everything required to compile and build the examples on Windows, Linux and Android using a C++ compiler that supports C++11. All required dependencies are included.

## <img src="./images/windowslogo.png" alt="" height="32px"> Windows

[![Build status](https://ci.appveyor.com/api/projects/status/abylymfyil0mhpx8?svg=true)](https://ci.appveyor.com/project/SaschaWillems/vulkan)

Use the provided CMakeLists.txt with [CMake](https://cmake.org) to generate a build configuration for your favorite IDE or compiler, e.g.:
```
cmake -G "Visual Studio 14 2015 Win64"
```

## <img src="./images/linuxlogo.png" alt="" height="32px"> Linux

[![Build Status](https://travis-ci.org/SaschaWillems/Vulkan.svg?branch=master)](https://travis-ci.org/SaschaWillems/Vulkan)

Use the provided CMakeLists.txt with [CMake](https://cmake.org) to generate a build configuration for your favorite IDE or compiler.

Note that you need [assimp](https://github.com/assimp/assimp) in order to compile the examples for Linux. Either compile and install from the repository, or install libassimp-dev. The examples require at least version 3.2.

##### [Window system integration](https://www.khronos.org/registry/vulkan/specs/1.0-wsi_extensions/html/vkspec.html#wsi)
- **XCB**: Default WSI (if no cmake option is specified)
- **Wayland**: Use cmake option ```USE_WAYLAND_WSI``` (```-DUSE_WAYLAND_WSI=ON```)
- **DirectToDisplay**: Use cmake option ```USE_D2D_WSI``` (```-DUSE_D2D_WSI=ON```)

## <img src="./images/androidlogo.png" alt="" height="32px"> [Android](android/)

Building on Android is done using [Android Studio](https://developer.android.com/studio/) (Google's own and free Android IDE) and requires a device that supports Vulkan. Please see the [Android readme](./android/README.md) for details on how to build and run the samples.

## <img src="./images/applelogo.png" alt="" height="32px"> [iOS and macOS](xcode/)

Building for *iOS* and *macOS* is done using the [examples](xcode/examples.xcodeproj) *Xcode* project found in the [xcode](xcode) directory. These examples use the [**MoltenVK**](https://moltengl.com/moltenvk) Vulkan driver to provide Vulkan support on *iOS* and *macOS*, and require an *iOS* or *macOS* device that supports *Metal*. Please see the [MoltenVK Examples readme](xcode/README_MoltenVK_Examples.md) for more info on acquiring **MoltenVK** and building and deploying the examples on *iOS* and *macOS*.