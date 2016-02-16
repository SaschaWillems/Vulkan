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
/// @file gli/view.hpp
/// @date 2013-02-07 / 2013-02-07
/// @author Christophe Riccio
///////////////////////////////////////////////////////////////////////////////////

#pragma once

#include "image.hpp"
#include "texture.hpp"
#include "texture1d.hpp"
#include "texture1d_array.hpp"
#include "texture2d.hpp"
#include "texture2d_array.hpp"
#include "texture3d.hpp"
#include "texture_cube.hpp"
#include "texture_cube_array.hpp"

namespace gli
{
	/// Create an image view of an existing image, sharing the same memory storage.
	image view(image const & Image);

	/// Create a texture view of an existing texture, sharing the same memory storage.
	texture view(texture const & Texture);

	/// Create a texture view of an existing texture, sharing the same memory storage.
	template <typename texType>
	texture view(texType const & Texture);

	/// Create a texture view of an existing texture, sharing the same memory storage but a different format.
	/// The format must be a compatible format, a format which block size match the original format. 
	template <typename texType>
	texture view(texType const & Texture, format Format);

	/// Create a texture view of an existing texture, sharing the same memory storage but giving access only to a subset of levels.
	texture view(
		texture1D const & Texture,
		texture1D::size_type BaseLevel, texture1D::size_type MaxLevel);

	/// Create a texture view of an existing texture, sharing the same memory storage but giving access only to a subset of levels and layers.
	texture view(
		texture1DArray const & Texture,
		texture1DArray::size_type BaseLayer, texture1DArray::size_type MaxLayer,
		texture1DArray::size_type BaseLevel, texture1DArray::size_type MaxLevel);

	/// Create a texture view of an existing texture, sharing the same memory storage but giving access only to a subset of levels.
	texture view(
		texture2D const & Texture,
		texture2D::size_type BaseLevel, texture2D::size_type MaxLevel);

	/// Create a texture view of an existing texture, sharing the same memory storage but giving access only to a subset of levels and layers.
	texture view(
		texture2DArray const & Texture,
		texture2DArray::size_type BaseLayer, texture2DArray::size_type MaxLayer,
		texture2DArray::size_type BaseLevel, texture2DArray::size_type MaxLevel);

	/// Create a texture view of an existing texture, sharing the same memory storage but giving access only to a subset of levels.
	texture view(
		texture3D const & Texture,
		texture3D::size_type BaseLevel, texture3D::size_type MaxLevel);

	/// Create a texture view of an existing texture, sharing the same memory storage but giving access only to a subset of levels and faces.
	texture view(
		textureCube const & Texture,
		textureCube::size_type BaseFace, textureCube::size_type MaxFace,
		textureCube::size_type BaseLevel, textureCube::size_type MaxLevel);

	/// Create a texture view of an existing texture, sharing the same memory storage but giving access only to a subset of layers, levels and faces.
	texture view(
		textureCubeArray const & Texture,
		textureCubeArray::size_type BaseLayer, textureCubeArray::size_type MaxLayer,
		textureCubeArray::size_type BaseFace, textureCubeArray::size_type MaxFace,
		textureCubeArray::size_type BaseLevel, textureCubeArray::size_type MaxLevel);
}//namespace gli

#include "./core/view.inl"
