///////////////////////////////////////////////////////////////////////////////////
/// OpenGL Image (gli.g-truc.net)
///
/// Copyright (c) 2008 - 2015 G-Truc Creation (www.g-truc.net)
/// Permission is hereby granted, free of charge, to any person obtaining a copy
/// of this software and associated documentation files (the "Software"), to deal
/// in the Software without restriction, including without limitation the rights
/// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
/// copies of the Software, and to permit persons to whom the Software is
/// furnished to do so, subject to the following conditions:
///
/// The above copyright notice and this permission notice shall be included in
/// all copies or substantial portions of the Software.
///
/// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
/// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
/// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
/// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
/// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
/// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
/// THE SOFTWARE.
///
/// @ref core
/// @file gli/gli.hpp
/// @date 2008-12-19 / 2015-08-08
/// @author Christophe Riccio
///////////////////////////////////////////////////////////////////////////////////

/*! @mainpage OpenGL Image
 *
 */

#pragma once

#define GLI_VERSION					70
#define GLI_VERSION_MAJOR			0
#define GLI_VERSION_MINOR			7
#define GLI_VERSION_PATCH			0
#define GLI_VERSION_REVISION		0

#include "format.hpp"
#include "target.hpp"
#include "levels.hpp"

#include "image.hpp"
#include "texture.hpp"
#include "texture1d.hpp"
#include "texture1d_array.hpp"
#include "texture2d.hpp"
#include "texture2d_array.hpp"
#include "texture3d.hpp"
#include "texture_cube.hpp"
#include "texture_cube_array.hpp"

#include "copy.hpp"
#include "view.hpp"
#include "comparison.hpp"

#include "load.hpp"
#include "save.hpp"

#include "gl.hpp"
#include "dx.hpp"

#include "./core/flip.hpp"
#include "./core/fetch.hpp"



