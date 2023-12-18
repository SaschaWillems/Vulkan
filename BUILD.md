# Building

The repository contains everything required to compile and build the examples on Windows, Linux, Android and MacOS using a C++ compiler that supports at least C++11. All required dependencies are included. The project uses [CMake](https://cmake.org/) as the build system.

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

### <img src="./images/applelogo.png" alt="" height="32px"> [iOS and macOS](xcode/)

Building for *iOS* and *macOS* is done using the [examples](xcode/examples.xcodeproj) *Xcode* project found in the [xcode](xcode) directory. These examples use the [**MoltenVK**](https://moltengl.com/moltenvk) Vulkan driver to provide Vulkan support on *iOS* and *macOS*, and require an *iOS* or *macOS* device that supports *Metal*. Please see the [MoltenVK Examples readme](xcode/README_MoltenVK_Examples.md) for more info on acquiring **MoltenVK** and building and deploying the examples on *iOS* and *macOS*.

###### MacOS
Install Libomp with:
-brew install libomp
find the path
-brew --prefix libomp  
use the path from the above command to populate the path in the -DOpenMP_C_FLAGS, -DOpenMP_omp_LIBRARY & -DOpenMP_CXX_FOUND statement below

Download Vulkan SDK and install it note the path as this will need to be configure in Xcode
curl -O https://sdk.lunarg.com/sdk/download/latest/mac/vulkan_sdk.dmg  

Open vulkan_sdk.dmg and install Vulkan SDK
Navigate to the Vulkan SDK folder and run 'python install_vulkan.py'

Use the provided CMakeLists.txt with [CMake](https://cmake.org) to generate a build configuration for your favorite IDE or compiler, e.g.:
```
Example of cmake with libraries defined
cmake -G "Xcode" -DOpenMP_C_FLAGS=/usr/local/opt/libomp -DOpenMP_omp_LIBRARY=/usr/local/opt/libomp -DOpenMP_CXX_FOUND=/usr/local/opt/libomp
```



