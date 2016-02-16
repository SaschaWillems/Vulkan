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
/// @file gli/texture3d.hpp
/// @date 2010-01-09 / 2013-01-11
/// @author Christophe Riccio
///////////////////////////////////////////////////////////////////////////////////

#pragma once

#include "texture.hpp"
#include "image.hpp"

namespace gli
{
	/// texture3D
	class texture3D : public texture
	{
	public:
		typedef dim3_t dim_type;
		typedef vec3 texcoord_type;

	public:
		/// Create an empty texture 3D
		texture3D();

		/// Create a texture3D and allocate a new storage
		explicit texture3D(
			format_type Format,
			dim_type const & Dimensions,
			size_type Levels);

		/// Create a texture3D and allocate a new storage with a complete mipmap chain
		explicit texture3D(
			format_type Format,
			dim_type const & Dimensions);

		/// Create a texture3D view with an existing storage
		explicit texture3D(
			texture const & Texture);

		/// Create a texture3D view with an existing storage
		explicit texture3D(
			texture const & Texture,
			format_type Format,
			size_type BaseLayer, size_type MaxLayer,
			size_type BaseFace, size_type MaxFace,
			size_type BaseLevel, size_type MaxLevel);

		/// Create a texture3D view, reference a subset of an existing texture3D instance
		explicit texture3D(
			texture3D const & Texture,
			size_type BaseLevel, size_type MaxLevel);

		/// Create a view of the image identified by Level in the mipmap chain of the texture
		image operator[](size_type Level) const;

		/// Return the dimensions of a texture instance: width, height and depth 
		dim_type dimensions() const;
	};
}//namespace gli

#include "./core/texture3d.inl"
