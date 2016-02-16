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
/// @file gli/core/texture_cube.inl
/// @date 2011-04-06 / 2012-12-12
/// @author Christophe Riccio
///////////////////////////////////////////////////////////////////////////////////

namespace gli
{
	inline textureCube::textureCube()
	{}

	inline textureCube::textureCube
	(
		format_type Format,
		dim_type const & Dimensions
	)
		: texture(gli::TARGET_CUBE, Format, texture::dim_type(Dimensions, 1), 1, 6, gli::levels(Dimensions))
	{}

	inline textureCube::textureCube
	(
		format_type Format,
		dim_type const & Dimensions,
		size_type Levels
	)
		: texture(gli::TARGET_CUBE, Format, texture::dim_type(Dimensions, 1), 1, 6, Levels)
	{}

	inline textureCube::textureCube(texture const & Texture)
		: texture(Texture, gli::TARGET_CUBE, Texture.format())
	{}

	inline textureCube::textureCube
	(
		texture const & Texture,
		format_type Format,
		size_type BaseLayer, size_type MaxLayer,
		size_type BaseFace, size_type MaxFace,
		size_type BaseLevel, size_type MaxLevel
	)
		: texture(
			Texture, gli::TARGET_CUBE,
			Format,
			BaseLayer, MaxLayer,
			BaseFace, MaxFace,
			BaseLevel, MaxLevel)
	{}

	inline textureCube::textureCube
	(
		textureCube const & Texture,
		size_type BaseFace, size_type MaxFace,
		size_type BaseLevel, size_type MaxLevel
	)
		: texture(
			Texture, gli::TARGET_CUBE,
			Texture.format(),
			Texture.base_layer(), Texture.max_layer(),
			Texture.base_face() + BaseFace, Texture.base_face() + MaxFace,
			Texture.base_level() + BaseLevel, Texture.base_level() + MaxLevel)
	{}

	inline texture2D textureCube::operator[](size_type Face) const
	{
		assert(Face < this->faces());

		return texture2D(
			*this, this->format(),
			this->base_layer(), this->max_layer(),
			this->base_face() + Face, this->base_face() + Face,
			this->base_level(), this->max_level());
	}

	inline textureCube::dim_type textureCube::dimensions() const
	{
		assert(!this->empty());

		return textureCube::dim_type(
			this->Storage->block_count(this->base_level()) * block_dimensions(this->format()));

		//return textureCube::dim_type(this->Storage.dimensions(this->base_level()));
	}
}//namespace gli
