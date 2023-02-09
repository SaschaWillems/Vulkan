# Header only C++ tiny glTF library(loader/saver).

`TinyGLTF` is a header only C++11 glTF 2.0 https://github.com/KhronosGroup/glTF library.

`TinyGLTF` uses Niels Lohmann's json library(https://github.com/nlohmann/json), so now it requires C++11 compiler.
If you are looking for old, C++03 version, please use `devel-picojson` branch.

## Status

 - v2.4.0 Experimental RapidJSON support. Experimental C++14 support(C++14 may give better performance)
 - v2.3.0 Modified Material representation according to glTF 2.0 schema(and introduced TextureInfo class)
 - v2.2.0 release(Support loading 16bit PNG. Sparse accessor support)
 - v2.1.0 release(Draco support)
 - v2.0.0 release(22 Aug, 2018)!

## Builds

[![Build Status](https://travis-ci.org/syoyo/tinygltf.svg?branch=devel)](https://travis-ci.org/syoyo/tinygltf)

[![Build status](https://ci.appveyor.com/api/projects/status/warngenu9wjjhlm8?svg=true)](https://ci.appveyor.com/project/syoyo/tinygltf)

## Features

* Written in portable C++. C++-11 with STL dependency only.
  * [x] macOS + clang(LLVM)
  * [x] iOS + clang
  * [x] Linux + gcc/clang
  * [x] Windows + MinGW
  * [x] Windows + Visual Studio 2015 Update 3 or later.
    * Visual Studio 2013 is not supported since they have limited C++11 support and failed to compile `json.hpp`.
  * [x] Android NDK
  * [x] Android + CrystaX(NDK drop-in replacement) GCC
  * [x] Web using Emscripten(LLVM)
* Moderate parsing time and memory consumption.
* glTF specification v2.0.0
  * [x] ASCII glTF
    * [x] Load
    * [x] Save
  * [x] Binary glTF(GLB)
    * [x] Load
    * [x] Save(.bin embedded .glb)
* Buffers
  * [x] Parse BASE64 encoded embedded buffer data(DataURI).
  * [x] Load `.bin` file.
* Image(Using stb_image)
  * [x] Parse BASE64 encoded embedded image data(DataURI).
  * [x] Load external image file.
  * [x] Load PNG(8bit and 16bit)
  * [x] Load JPEG(8bit only)
  * [x] Load BMP
  * [x] Load GIF
  * [x] Custom Image decoder callback(e.g. for decoding OpenEXR image)
* Morph traget
  * [x] Sparse accessor
* Load glTF from memory
* Custom callback handler
  * [x] Image load
  * [x] Image save
* Extensions
  * [x] Draco mesh decoding
  * [ ] Draco mesh encoding

## Note on extension property

In extension(`ExtensionMap`), JSON number value is parsed as int or float(number) and stored as `tinygltf::Value` object. If you want a floating point value from `tinygltf::Value`, use `GetNumberAsDouble()` method.

`IsNumber()` returns true if the underlying value is an int value or a floating point value.

## Examples

* [glview](examples/glview) : Simple glTF geometry viewer.
* [validator](examples/validator) : Simple glTF validator with JSON schema.
* [basic](examples/basic) : Basic glTF viewer with texturing support.

## Projects using TinyGLTF

* px_render Single header C++ Libraries for Thread Scheduling, Rendering, and so on... https://github.com/pplux/px
* Physical based rendering with Vulkan using glTF 2.0 models https://github.com/SaschaWillems/Vulkan-glTF-PBR
* GLTF loader plugin for OGRE 2.1. Support for PBR materials via HLMS/PBS https://github.com/Ybalrid/Ogre_glTF
* [TinyGltfImporter](http://doc.magnum.graphics/magnum/classMagnum_1_1Trade_1_1TinyGltfImporter.html) plugin for [Magnum](https://github.com/mosra/magnum), a lightweight and modular C++11/C++14 graphics middleware for games and data visualization.
* [Diligent Engine](https://github.com/DiligentGraphics/DiligentEngine) - A modern cross-platform low-level graphics library and rendering framework
* Lighthouse 2: a rendering framework for real-time ray tracing / path tracing experiments. https://github.com/jbikker/lighthouse2
* [QuickLook GLTF](https://github.com/toshiks/glTF-quicklook) - quicklook plugin for macos. Also SceneKit wrapper for tinygltf.
* [GlslViewer](https://github.com/patriciogonzalezvivo/glslViewer) - live GLSL coding for MacOS and Linux
* [Vulkan-Samples](https://github.com/KhronosGroup/Vulkan-Samples) - The Vulkan Samples is collection of resources to help you develop optimized Vulkan applications.
* Your projects here! (Please send PR)

## TODOs

* [ ] Write C++ code generator which emits C++ code from JSON schema for robust parsing.
* [ ] Mesh Compression/decompression(Open3DGC, etc)
  * [x] Load Draco compressed mesh
  * [ ] Save Draco compressed mesh
  * [ ] Open3DGC?
* [x] Support `extensions` and `extras` property
* [ ] HDR image?
  * [ ] OpenEXR extension through TinyEXR.
* [ ] 16bit PNG support in Serialization
* [ ] Write example and tests for `animation` and `skin`

## Licenses

TinyGLTF is licensed under MIT license.

TinyGLTF uses the following third party libraries.

* json.hpp : Copyright (c) 2013-2017 Niels Lohmann. MIT license.
* base64 : Copyright (C) 2004-2008 René Nyffenegger
* stb_image.h : v2.08 - public domain image loader - [Github link](https://github.com/nothings/stb/blob/master/stb_image.h)
* stb_image_write.h : v1.09 - public domain image writer - [Github link](https://github.com/nothings/stb/blob/master/stb_image_write.h)


## Build and example

Copy `stb_image.h`, `stb_image_write.h`, `json.hpp` and `tiny_gltf.h` to your project.

### Loading glTF 2.0 model

```c++
// Define these only in *one* .cc file.
#define TINYGLTF_IMPLEMENTATION
#define STB_IMAGE_IMPLEMENTATION
#define STB_IMAGE_WRITE_IMPLEMENTATION
// #define TINYGLTF_NOEXCEPTION // optional. disable exception handling.
#include "tiny_gltf.h"

using namespace tinygltf;

Model model;
TinyGLTF loader;
std::string err;
std::string warn;

bool ret = loader.LoadASCIIFromFile(&model, &err, &warn, argv[1]);
//bool ret = loader.LoadBinaryFromFile(&model, &err, &warn, argv[1]); // for binary glTF(.glb)

if (!warn.empty()) {
  printf("Warn: %s\n", warn.c_str());
}

if (!err.empty()) {
  printf("Err: %s\n", err.c_str());
}

if (!ret) {
  printf("Failed to parse glTF\n");
  return -1;
}
```

## Compile options

* `TINYGLTF_NOEXCEPTION` : Disable C++ exception in JSON parsing. You can use `-fno-exceptions` or by defining the symbol `JSON_NOEXCEPTION` and `TINYGLTF_NOEXCEPTION`  to fully remove C++ exception codes when compiling TinyGLTF.
* `TINYGLTF_NO_STB_IMAGE` : Do not load images with stb_image. Instead use `TinyGLTF::SetImageLoader(LoadimageDataFunction LoadImageData, void *user_data)` to set a callback for loading images.
* `TINYGLTF_NO_STB_IMAGE_WRITE` : Do not write images with stb_image_write. Instead use `TinyGLTF::SetImageWriter(WriteimageDataFunction WriteImageData, void *user_data)` to set a callback for writing images.
* `TINYGLTF_NO_EXTERNAL_IMAGE` : Do not try to load external image file. This option would be helpful if you do not want to load image files during glTF parsing.
* `TINYGLTF_ANDROID_LOAD_FROM_ASSETS`: Load all files from packaged app assets instead of the regular file system. **Note:** You must pass a valid asset manager from your android app to `tinygltf::asset_manager` beforehand.
* `TINYGLTF_ENABLE_DRACO`: Enable Draco compression. User must provide include path and link correspnding libraries in your project file.
* `TINYGLTF_NO_INCLUDE_JSON `: Disable including `json.hpp` from within `tiny_gltf.h` because it has been already included before or you want to include it using custom path before including `tiny_gltf.h`.
* `TINYGLTF_NO_INCLUDE_STB_IMAGE `: Disable including `stb_image.h` from within `tiny_gltf.h` because it has been already included before or you want to include it using custom path before including `tiny_gltf.h`.
* `TINYGLTF_NO_INCLUDE_STB_IMAGE_WRITE `: Disable including `stb_image_write.h` from within `tiny_gltf.h` because it has been already included before or you want to include it using custom path before including `tiny_gltf.h`.
* `TINYGLTF_USE_RAPIDJSON` : Use RapidJSON as a JSON parser/serializer. RapidJSON files are not included in TinyGLTF repo. Please set an include path to RapidJSON if you enable this featrure.
* `TINYGLTF_USE_CPP14` : Use C++14 feature(requires C++14 compiler). This may give better performance than C++11.


### Saving gltTF 2.0 model

* Buffers.
  * [x] To file
  * [x] Embedded
  * [ ] Draco compressed?
* [x] Images
  * [x] To file
  * [x] Embedded
* Binary(.glb)
  * [x] .bin embedded single .glb
  * [ ] External .bin

## Running tests.

### glTF parsing test

#### Setup

Python 2.6 or 2.7 required.
Git clone https://github.com/KhronosGroup/glTF-Sample-Models to your local dir.

#### Run parsing test

After building `loader_example`, edit `test_runner.py`, then,

```bash
$ python test_runner.py
```

### Unit tests

```bash
$ cd tests
$ make
$ ./tester
$ ./tester_noexcept
```

### Fuzzing tests

See `tests/fuzzer` for details.

After running fuzzer on Ryzen9 3950X a week, at least `LoadASCIIFromString` looks safe except for out-of-memory error in Fuzzer.
We may be better to introduce bounded memory size checking when parsing glTF data.

## Third party licenses

* json.hpp : Licensed under the MIT License <http://opensource.org/licenses/MIT>. Copyright (c) 2013-2017 Niels Lohmann <http://nlohmann.me>.
* stb_image : Public domain.
* catch : Copyright (c) 2012 Two Blue Cubes Ltd. All rights reserved. Distributed under the Boost Software License, Version 1.0.
* RapidJSON : Copyright (C) 2015 THL A29 Limited, a Tencent company, and Milo Yip. All rights reserved. http://rapidjson.org/
* dlib(uridecode, uriencode) : Copyright (C) 2003  Davis E. King Boost Software License 1.0. http://dlib.net/dlib/server/server_http.cpp.html
