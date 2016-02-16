///////////////////////////////////////////////////////////////////////////////////
/// OpenGL Image (gli.g-truc.net)
///
/// Copyright (c) 2008 - 2014 G-Truc Creation (www.g-truc.net)
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
/// @file gli/levels.hpp
/// @date 2014-12-12 / 2014-12-12
/// @author Christophe Riccio
///
/// @defgroup core_image Image 
/// @ingroup core
///////////////////////////////////////////////////////////////////////////////////

#pragma once

#include "type.hpp"

namespace gli
{
	/// Compute the number of mipmaps levels necessary to create a mipmap complete texture
	/// 
	/// @param Dimensions Dimensions of the texture base level mipmap
	/// @tparam dimType Vector type used to express the dimentions of a texture of any kind.
	/// @code
	/// #include <gli/gli.hpp>
	/// #include <gli/levels.hpp>
	/// ...
	/// gli::size2_t Dimensions(32, 10);
	/// gli::texture2D Texture(gli::levels(Dimensions));
	/// @endcode
	template <template <typename, glm::precision> class dimType>
	size_t levels(dimType<size_t, glm::defaultp> const & Dimensions);

	/// Compute the number of mipmaps levels necessary to create a mipmap complete texture
	/// 
	/// @param Dimensions Dimensions of the texture base level mipmap
	/// @tparam dimType Vector type used to express the dimentions of a texture of any kind.
	/// @code
	/// #include <gli/gli.hpp>
	/// #include <gli/levels.hpp>
	/// ...
	/// gli::texture2D Texture(32);
	/// @endcode
	size_t levels(size_t Dimension);
}//namespace gli

#include "./core/levels.inl"
