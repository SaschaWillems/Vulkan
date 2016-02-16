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
/// @file gli/core/flip.inl
/// @date 2014-01-17 / 2014-01-17
/// @author Christophe Riccio
///////////////////////////////////////////////////////////////////////////////////

namespace gli{
namespace detail
{
	inline void flip(image ImageDst, image ImageSrc, std::size_t BlockSize)
	{
		std::size_t const LineSize = BlockSize * ImageDst.dimensions().x;

		for(std::size_t y = 0; y < ImageDst.dimensions().y; ++y)
		{
			std::size_t OffsetDst = LineSize * y;
			std::size_t OffsetSrc = ImageSrc.size() - (LineSize * (y + 1));

			memcpy(
				ImageDst.data<glm::byte>() + OffsetDst,
				ImageSrc.data<glm::byte>() + OffsetSrc,
				LineSize);
		}
	}

}//namespace detail

/*
template <>
inline image flip(image const & Image)
{

}
*/

template <>
inline texture2D flip(texture2D const & Texture)
{
	assert(!gli::is_compressed(Texture.format()));

	texture2D Flip(Texture.format(), Texture.dimensions(), Texture.levels());

	gli::size_t const BlockSize = block_size(Texture.format());

	for(std::size_t Level = 0; Level < Flip.levels(); ++Level)
		detail::flip(Flip[Level], Texture[Level], BlockSize);

	return Flip;
}

template <>
inline texture2DArray flip(texture2DArray const & Texture)
{
	assert(!gli::is_compressed(Texture.format()));

	texture2DArray Flip(Texture.format(), Texture.dimensions(), Texture.layers(), Texture.levels());

	gli::size_t const BlockSize = block_size(Texture.format());

	for(std::size_t Layer = 0; Layer < Flip.layers(); ++Layer)
	for(std::size_t Level = 0; Level < Flip.levels(); ++Level)
		detail::flip(Flip[Layer][Level], Texture[Layer][Level], BlockSize);

	return Flip;
}

}//namespace gli
