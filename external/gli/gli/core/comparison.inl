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
/// @file gli/core/comparison.inl
/// @date 2013-02-04 / 2013-02-04
/// @author Christophe Riccio
///////////////////////////////////////////////////////////////////////////////////

#include <cstring>

namespace gli{
namespace detail
{
	inline bool equalData(texture const & TextureA, texture const & TextureB)
	{
		if(TextureA.data() == TextureB.data())
			return true;

		void const* PointerA = TextureA.data();
		void const* PointerB = TextureB.data();
		if(std::memcmp(PointerA, PointerB, TextureA.size()) == 0)
			return true;

		return false;
	}
}//namespace detail

	inline bool operator==(image const & ImageA, image const & ImageB)
	{
		if(!glm::all(glm::equal(ImageA.dimensions(), ImageB.dimensions())))
			return false;
		if(ImageA.size() != ImageB.size())
			return false;

		for(std::size_t i = 0; i < ImageA.size<glm::byte>(); ++i)
			if(*(ImageA.data<glm::byte>() + i) != *(ImageB.data<glm::byte>() + i))
				return false;

		return true;
	}

	inline bool operator!=(image const & ImageA, image const & ImageB)
	{
		if(!glm::all(glm::equal(ImageA.dimensions(), ImageB.dimensions())))
			return true;
		if(ImageA.size() != ImageB.size())
			return true;

		for(std::size_t i = 0; i < ImageA.size<glm::byte>(); ++i)
			if(*(ImageA.data<glm::byte>() + i) != *(ImageB.data<glm::byte>() + i))
				return true;

		return false;
	}

	inline bool equal(texture const & TextureA, texture const & TextureB)
	{
		if(TextureA.empty() && TextureB.empty())
			return true;
		if(TextureA.empty() != TextureB.empty())
			return false;
		if(TextureA.target() != TextureB.target())
			return false;
		if(TextureA.layers() != TextureB.layers())
			return false;
		if(TextureA.faces() != TextureB.faces())
			return false;
		if(TextureA.levels() != TextureB.levels())
			return false;
		if(TextureA.format() != TextureB.format())
			return false;
		if(TextureA.size() != TextureB.size())
			return false;

		return detail::equalData(TextureA, TextureB);
	}

	inline bool notEqual(texture const & TextureA, texture const & TextureB)
	{
		if(TextureA.empty() && TextureB.empty())
			return false;
		if(TextureA.empty() != TextureB.empty())
			return true;
		if(TextureA.target() != TextureB.target())
			return true;
		if(TextureA.layers() != TextureB.layers())
			return true;
		if(TextureA.faces() != TextureB.faces())
			return true;
		if(TextureA.levels() != TextureB.levels())
			return true;
		if(TextureA.format() != TextureB.format())
			return true;
		if(TextureA.size() != TextureB.size())
			return true;

		return !detail::equalData(TextureA, TextureB);
	}

	inline bool operator==(gli::texture const & A, gli::texture const & B)
	{
		return gli::equal(A, B);
	}

	inline bool operator!=(gli::texture const & A, gli::texture const & B)
	{
		return gli::notEqual(A, B);
	}
}//namespace gli
