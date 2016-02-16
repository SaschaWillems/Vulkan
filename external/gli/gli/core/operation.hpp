///////////////////////////////////////////////////////////////////////////////////
/// OpenGL Image (gli.g-truc.net)
///
/// Copyright (c) 2008 - 2013 G-Truc Creation (www.g-truc.net)
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
/// @file gli/core/operation.hpp
/// @date 2008-12-19 / 2013-01-12
/// @author Christophe Riccio
///////////////////////////////////////////////////////////////////////////////////

#pragma once

#include "texture2d.hpp"

namespace gli
{
/*
	texture2D duplicate(texture2D const & Texture);
	texture2D flip(texture2D const & Texture);
	texture2D mirror(texture2D const & Texture);
	texture2D swizzle(
		texture2D const & Texture, 
		glm::uvec4 const & Channel);
	texture2D crop(
		texture2D const & Texture, 
		texture2D::dimensions_type const & Position,
		texture2D::dimensions_type const & Size);

	image2D crop(
		image2D const & Image, 
		image2D::dimensions_type const & Position,
		image2D::dimensions_type const & Size);

	image2D copy(
		image2D const & SrcImage, 
		image2D::dimensions_type const & SrcPosition,
		image2D::dimensions_type const & SrcSize,
		image2D & DstImage, 
		image2D::dimensions_type const & DstPosition);
*/
	//image operator+(image const & MipmapA, image const & MipmapB);
	//image operator-(image const & MipmapA, image const & MipmapB);
	//image operator*(image const & MipmapA, image const & MipmapB);
	//image operator/(image const & MipmapA, image const & MipmapB);

	//namespace wip
	//{
	//	template <typename GENTYPE, template <typename> class SURFACE>
	//	GENTYPE fetch(SURFACE<GENTYPE> const & Image)
	//	{
	//		return GENTYPE();
	//	}

	//	template
	//	<
	//		typename GENTYPE, 
	//		template 
	//		<
	//			typename
	//		>
	//		class SURFACE,
	//		template 
	//		<
	//			typename, 
	//			template 
	//			<
	//				typename
	//			>
	//			class
	//		> 
	//		class IMAGE
	//	>
	//	GENTYPE fetch(IMAGE<GENTYPE, SURFACE> const & Image)
	//	{
	//		return GENTYPE();
	//	}
	//}//namespace wip

}//namespace gli

#include "operation.inl"
