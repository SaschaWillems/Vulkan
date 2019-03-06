# Header only C++ tiny glTF library(loader/saver).

`TinyGLTF` is a header only C++11 glTF 2.0 https://github.com/KhronosGroup/glTF library.

## Status

Work in process(`devel` branch). Very near to release, but need more tests and examples.

`TinyGLTF` uses Niels Lohmann's json library(https://github.com/nlohmann/json), so now it requires C++11 compiler.
If you are looking for old, C++03 version, please use `devel-picojson` branch.  

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
  * [x] Android + CrystaX(NDK drop-in replacement) GCC
  * [x] Web using Emscripten(LLVM)
* Moderate parsing time and memory consumption.
* glTF specification v2.0.0
  * [x] ASCII glTF
  * [x] Binary glTF(GLB)
  * [x] PBR material description
* Buffers
  * [x] Parse BASE64 encoded embedded buffer fata(DataURI).
  * [x] Load `.bin` file.
* Image(Using stb_image)
  * [x] Parse BASE64 encoded embedded image fata(DataURI).
  * [x] Load external image file.
  * [x] PNG(8bit only)
  * [x] JPEG(8bit only)
  * [x] BMP
  * [x] GIF

## Examples

* [glview](examples/glview) : Simple glTF geometry viewer.
* [validator](examples/validator) : Simple glTF validator with JSON schema.

## Projects using TinyGLTF

* Physical based rendering with Vulkan using glTF 2.0 models https://github.com/SaschaWillems/Vulkan-glTF-PBR
* GLTF loader plugin for OGRE 2.1. Support for PBR materials via HLMS/PBS https://github.com/Ybalrid/Ogre_glTF
* [TinyGltfImporter](http://doc.magnum.graphics/magnum/classMagnum_1_1Trade_1_1TinyGltfImporter.html) plugin for [Magnum](https://github.com/mosra/magnum), a lightweight and modular C++11/C++14 graphics middleware for games and data visualization.
* Your projects here! (Please send PR)

## TODOs

* [ ] Write C++ code generator from jSON schema for robust parsing.
* [x] Serialization
* [ ] Compression/decompression(Open3DGC, etc)
* [ ] Support `extensions` and `extras` property
* [ ] HDR image?
  * [ ] OpenEXR extension through TinyEXR.
* [ ] Write tests for `animation` and `skin` 

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
  
bool ret = loader.LoadASCIIFromFile(&model, &err, argv[1]);
//bool ret = loader.LoadBinaryFromFile(&model, &err, argv[1]); // for binary glTF(.glb) 
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

### Saving gltTF 2.0 model
* [ ] Buffers.
  * [x] To file
  * [x] Embedded
  * [ ] Draco compressed?
* [x] Images
  * [x] To file
  * [x] Embedded
* [ ] Binary(.glb)

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

## Third party licenses

* json.hpp : Licensed under the MIT License <http://opensource.org/licenses/MIT>. Copyright (c) 2013-2017 Niels Lohmann <http://nlohmann.me>.
* stb_image : Public domain.
* catch : Copyright (c) 2012 Two Blue Cubes Ltd. All rights reserved. Distributed under the Boost Software License, Version 1.0.
