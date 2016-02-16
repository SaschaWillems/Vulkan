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
/// @file gli/core/texture2d_array.inl
/// @date 2013-01-12 / 2013-01-12
/// @author Christophe Riccio
///////////////////////////////////////////////////////////////////////////////////

#include "../levels.hpp"

namespace gli
{
	inline texture2DArray::texture2DArray()
	{}

	inline texture2DArray::texture2DArray
	(
		format_type Format,
		dim_type const & Dimensions,
		size_type Layers
	)
		: texture(gli::TARGET_2D_ARRAY, Format, texture::dim_type(Dimensions, 1), Layers, 1, gli::levels(Dimensions))
	{}

	inline texture2DArray::texture2DArray
	(
		format_type Format,
		dim_type const & Dimensions,
		size_type Layers,
		size_type Levels
	)
		: texture(gli::TARGET_2D_ARRAY, Format, texture::dim_type(Dimensions, 1), Layers, 1, Levels)
	{}

	inline texture2DArray::texture2DArray(texture const & Texture)
		: texture(Texture, gli::TARGET_2D_ARRAY, Texture.format())
	{}

	inline texture2DArray::texture2DArray
	(
		texture const & Texture,
		format_type Format,
		size_type BaseLayer, size_type MaxLayer,
		size_type BaseFace, size_type MaxFace,
		size_type BaseLevel, size_type MaxLevel
	)
		: texture(
			Texture, gli::TARGET_2D_ARRAY,
			Format,
			BaseLayer, MaxLayer,
			BaseFace, MaxFace,
			BaseLevel, MaxLevel)
	{}

	inline texture2DArray::texture2DArray
	(
		texture2DArray const & Texture,
		size_type BaseLayer, size_type MaxLayer,
		size_type BaseLevel, size_type MaxLevel
	)
		: texture(
			Texture, gli::TARGET_2D_ARRAY,
			Texture.format(),
			Texture.base_layer() + BaseLayer, Texture.base_layer() + MaxLayer,
			Texture.base_face(), Texture.max_face(),
			Texture.base_level() + BaseLevel, Texture.base_level() + MaxLevel)
	{}

	inline texture2D texture2DArray::operator[](size_type Layer) const
	{
		assert(Layer < this->layers());

		return texture2D(
			*this, this->format(),
			this->base_layer() + Layer, this->base_layer() + Layer,
			this->base_face(), this->max_face(),
			this->base_level(), this->max_level());
	}

	inline texture2DArray::dim_type texture2DArray::dimensions() const
	{
		assert(!this->empty());

		return texture2DArray::dim_type(
			this->Storage->block_count(this->base_level()) * block_dimensions(this->format()));

		//return texture2DArray::dim_type(this->Storage.dimensions(this->base_level()));
	}
}//namespace gli
