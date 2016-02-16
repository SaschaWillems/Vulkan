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
/// @file gli/core/fetch.hpp
/// @date 2008-12-19 / 2013-01-13
/// @author Christophe Riccio
///////////////////////////////////////////////////////////////////////////////////

#pragma once

#include "../gli.hpp"

namespace gli
{
	/// Fetch a texel from a texture
	/// The texture format must be uncompressed
	template <typename genType>
	genType texel_fetch(
		texture2D const & Texture,
		texture2D::dim_type const & Texcoord,
		texture2D::size_type const & Level);

	/// Write a texel to a texture
	/// The texture format must be uncompressed
	template <typename genType>
	void texel_write(
		texture2D & Texture,
		texture2D::dim_type const & Texcoord,
		texture2D::size_type const & Level,
		genType const & Color);

	/// Sample a pixel from a texture
	/// The texture format must be uncompressed
	template <typename genType>
	genType texture_lod(
		texture2D const & Texture,
		texture2D::texcoord_type const & Texcoord,
		texture2D::size_type const & Level);

}//namespace gli

#include "fetch.inl"
