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
/// @file gli/core/texture_cube_array.inl
/// @date 2013-01-10 / 2013-01-13
/// @author Christophe Riccio
///////////////////////////////////////////////////////////////////////////////////

namespace gli
{
	inline textureCubeArray::textureCubeArray()
	{}

	inline textureCubeArray::textureCubeArray
	(
		format_type Format,
		dim_type const & Dimensions,
		size_type Layers
	)
		: texture(gli::TARGET_CUBE_ARRAY, Format, texture::dim_type(Dimensions, 1), Layers, 6, gli::levels(Dimensions))
	{}

	inline textureCubeArray::textureCubeArray
	(
		format_type Format,
		dim_type const & Dimensions,
		size_type Layers,
		size_type Levels
	)
		: texture(gli::TARGET_CUBE_ARRAY, Format, texture::dim_type(Dimensions, 1), Layers, 6, Levels)
	{}

	inline textureCubeArray::textureCubeArray(texture const & Texture)
		: texture(Texture, gli::TARGET_CUBE_ARRAY, Texture.format())
	{}

	inline textureCubeArray::textureCubeArray
	(
		texture const & Texture,
		format_type Format,
		size_type BaseLayer, size_type MaxLayer,
		size_type BaseFace, size_type MaxFace,
		size_type BaseLevel, size_type MaxLevel
	)
		: texture(
			Texture, gli::TARGET_CUBE_ARRAY,
			Format,
			BaseLayer, MaxLayer,
			BaseFace, MaxFace,
			BaseLevel, MaxLevel)
	{}

	inline textureCubeArray::textureCubeArray
	(
		textureCubeArray const & Texture,
		size_type BaseLayer, size_type MaxLayer,
		size_type BaseFace, size_type MaxFace,
		size_type BaseLevel, size_type MaxLevel
	)
		: texture(
			Texture, gli::TARGET_CUBE_ARRAY, Texture.format(),
			Texture.base_layer() + BaseLayer, Texture.base_layer() + MaxLayer,
			Texture.base_face() + BaseFace, Texture.base_face() + MaxFace,
			Texture.base_level() + BaseLevel, Texture.base_level() + MaxLevel)
	{}

	inline textureCube textureCubeArray::operator[](size_type Layer) const
	{
		assert(Layer < this->layers());

		return textureCube(
			*this, this->format(),
			this->base_layer() + Layer, this->base_layer() + Layer,
			this->base_face(), this->max_face(),
			this->base_level(), this->max_level());
	}

	inline textureCubeArray::dim_type textureCubeArray::dimensions() const
	{
		assert(!this->empty());

		return textureCubeArray::dim_type(
			this->Storage->block_count(this->base_level()) * block_dimensions(this->format()));

		//return textureCubeArray::dim_type(this->Storage.dimensions(this->base_level()));
	}
}//namespace gli
