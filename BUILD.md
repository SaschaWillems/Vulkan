# Building

The repository contains everything required to compile and build the examples on Windows, Linux, Android and MacOS using a C++ compiler that supports at least C++14. All required dependencies are included. The project uses [CMake](https://cmake.org/) as the build system.

## General CMake options

### Asset path setup

Asset (and shader) paths used by the samples can be adjusted using CMake options. By default, paths are absolute and are based on the top level of the current CMake source tree. The following arguments can be used to adjust this:

- ```RESOURCE_INSTALL_DIR```: Set an absolute path for assets and shaders to which they are installed and from which they are loaded
- ```USE_RELATIVE_ASSET_PATH```: Use a fixed relative (to the binary) path for loading assets and shaders

## Platform specific build instructions

### <img src="./images/windowslogo.png" alt="" height="32px"> Windows
Use the provided CMakeLists.txt with [CMake](https://cmake.org) to generate a build configuration for your favorite IDE or compiler, e.g.:

```
cmake -G "Visual Studio 16 2019" -A x64
```

### <img src="./images/linuxlogo.png" alt="" height="32px"> Linux

Use the provided CMakeLists.txt with [CMake](https://cmake.org) to generate a build configuration for your favorite IDE or compiler.

##### [Window system integration](https://www.khronos.org/registry/vulkan/specs/1.0-wsi_extensions/html/vkspec.html#wsi)
- **XCB**: Default WSI (if no cmake option is specified)
- **Wayland**: Use cmake option ```USE_WAYLAND_WSI``` (```-DUSE_WAYLAND_WSI=ON```)
- **DirectFB**: Use cmake option ```USE_DIRECTFB_WSI``` (```-DUSE_DIRECTFB_WSI=ON```)
- **DirectToDisplay**: Use cmake option ```USE_D2D_WSI``` (```-DUSE_D2D_WSI=ON```)

### <img src="./images/androidlogo.png" alt="" height="32px"> [Android](android/)

Building on Android is done using the [Gradle Build Tool](https://gradle.org/):

If you want to build it through command line, set Android SDK/NDK by environment variable `ANDROID_SDK_ROOT`/`ANDROID_NDK_HOME`.

On Linux execute:

```
cd android
./gradlew assembleDebug
```
This will download gradle locally, build all samples and output the apks to ```android/examples/bin```.

On Windows execute ```gradlew.bat assembleDebug```.

If you want to build and install on a connected device or emulator image, run ```gradle installDebug``` instead.

If you want to build it through [Android Studio](https://developer.android.com/studio), open project folder ```android``` in Android Studio.

### <img src="./images/applelogo.png" alt="" height="32px"> macOS and iOS

**Note:** Running these examples on macOS and iOS requires [**MoltenVK**](https://github.com/KhronosGroup/MoltenVK) and a device that supports the *Metal* api.

#### macOS
Download the most recent Vulkan SDK using:
```curl -O https://sdk.lunarg.com/sdk/download/latest/mac/vulkan_sdk.dmg```

Open **vulkan_sdk.dmg** and install the Vulkan SDK with *System Global Installation* selected.

Install **libomp** from [homebrew](https://brew.sh) using:
```brew install libomp```

Define the **libomp** path prefix using:
```export LIBOMP_PREFIX=$(brew --prefix libomp)```

Use [CMake](https://cmake.org) to generate a build configuration for Xcode or your preferred build method (e.g. Unix Makefiles or Ninja).

Example of cmake generating for Xcode with **libomp** library path defined:
```cmake -G "Xcode" -DOpenMP_omp_LIBRARY=$LIBOMP_PREFIX/lib/libomp.dylib .```

#### iOS
Navigate to the [apple](apple/) folder and follow the instructions in [README\_MoltenVK_Examples.md](apple/README_MoltenVK_Examples.md)
