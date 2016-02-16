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
/// @file gli/texture2d_array.hpp
/// @date 2011-04-06 / 2013-01-11
/// @author Christophe Riccio
///////////////////////////////////////////////////////////////////////////////////

#pragma once

#include "texture2d.hpp"

namespace gli
{
	/// texture2DArray
	class texture2DArray : public texture
	{
	public:
		typedef dim2_t dim_type;
		typedef vec3 texcoord_type;

	public:
		/// Create an empty texture 2D array
		texture2DArray();

		/// Create a texture2DArray and allocate a new storage
		explicit texture2DArray(
			format_type Format,
			dim_type const & Dimensions,
			size_type Layers,
			size_type Levels);

		/// Create a texture2DArray and allocate a new storage with a complete mipmap chain
		explicit texture2DArray(
			format_type Format,
			dim_type const & Dimensions,
			size_type Layers);

		/// Create a texture2DArray view with an existing storage
		explicit texture2DArray(
			texture const & Texture);

		/// Create a texture2DArray view with an existing storage
		explicit texture2DArray(
			texture const & Texture,
			format_type Format,
			size_type BaseLayer, size_type MaxLayer,
			size_type BaseFace, size_type MaxFace,
			size_type BaseLevel, size_type MaxLevel);

		/// Create a texture view, reference a subset of an exiting texture2DArray instance
		explicit texture2DArray(
			texture2DArray const & Texture,
			size_type BaseLayer, size_type MaxLayer,
			size_type BaseLevel, size_type MaxLevel);

		/// Create a view of the texture identified by Layer in the texture array
		texture2D operator[](size_type Layer) const;

		/// Return the dimensions of a texture instance: width and height 
		dim_type dimensions() const;
	};
}//namespace gli

#include "./core/texture2d_array.inl"
