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
/// @file gli/copy.hpp
/// @date 2013-02-01 / 2013-02-03
/// @author Christophe Riccio
///////////////////////////////////////////////////////////////////////////////////

#pragma once

#include "image.hpp"
#include "texture1d.hpp"
#include "texture1d_array.hpp"
#include "texture2d.hpp"
#include "texture2d_array.hpp"
#include "texture3d.hpp"
#include "texture_cube.hpp"
#include "texture_cube_array.hpp"

namespace gli
{
	/// Copy an image and create a new image with a new storage allocation.
	image copy(image const & Image);

	/// Copy a texture and create a new texture with a new storage allocation.
	texture copy(texture const & Texture);

	/// Copy a texture and create a new texture with a new storage allocation.
	template <typename texType>
	texture copy(texType const & Texture);

	/// Copy a texture and create a new texture with a new storage allocation but a different format.
	/// The format must be a compatible format, a format which block size match the original format. 
	template <typename texType>
	texture copy(texType const & Texture, format Format);

	/// Copy a subset of a texture and create a new texture with a new storage allocation.
	texture copy(
		texture1D const & Texture,
		texture1D::size_type BaseLevel, texture1D::size_type MaxLevel);

	/// Copy a subset of a texture and create a new texture with a new storage allocation.
	texture copy(
		texture1DArray const & Texture,
		texture1DArray::size_type BaseLayer, texture1DArray::size_type MaxLayer,
		texture1DArray::size_type BaseLevel, texture1DArray::size_type MaxLevel);

	/// Copy a subset of a texture and create a new texture with a new storage allocation.
	texture copy(
		texture2D const & Texture,
		texture2D::size_type BaseLevel, texture2D::size_type MaxLevel);

	/// Copy a subset of a texture and create a new texture with a new storage allocation.
	texture copy(
		texture2DArray const & Texture,
		texture2DArray::size_type BaseLayer, texture2DArray::size_type MaxLayer,
		texture2DArray::size_type BaseLevel, texture2DArray::size_type MaxLevel);

	/// Copy a subset of a texture and create a new texture with a new storage allocation.
	texture copy(
		texture3D const & Texture,
		texture3D::size_type BaseLevel, texture3D::size_type MaxLevel);

	/// Copy a subset of a texture and create a new texture with a new storage allocation.
	texture copy(
		textureCube const & Texture,
		textureCube::size_type BaseFace, textureCube::size_type MaxFace,
		textureCube::size_type BaseLevel, textureCube::size_type MaxLevel);

	/// Copy a subset of a texture and create a new texture with a new storage allocation.
	texture copy(
		textureCubeArray const & Texture,
		textureCubeArray::size_type BaseLayer, textureCubeArray::size_type MaxLayer,
		textureCubeArray::size_type BaseFace, textureCubeArray::size_type MaxFace,
		textureCubeArray::size_type BaseLevel, textureCubeArray::size_type MaxLevel);
}//namespace gli

#include "./core/copy.inl"
