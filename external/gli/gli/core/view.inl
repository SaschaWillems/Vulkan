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
/// @file gli/core/view.inl
/// @date 2013-02-07 / 2013-02-07
/// @author Christophe Riccio
///////////////////////////////////////////////////////////////////////////////////

namespace gli
{
	inline image view(image const & Image)
	{
		return Image;
	}

	inline texture view(texture const & Texture)
	{
		return Texture;
	}

	template <typename texType>
	inline texture view(texType const & Texture)
	{
		return Texture;
	}

	template <typename texType>
	inline texture view(texType const & Texture, format Format)
	{
		if(block_size(Texture.format()) != block_size(Format))
			return texture();

		return texture(Texture, Texture.target(), Format);
	}

	inline texture view
	(
		texture1D const & Texture,
		texture1D::size_type BaseLevel, texture1D::size_type MaxLevel
	)
	{
		assert(BaseLevel <= MaxLevel);
		assert(BaseLevel < Texture.levels());
		assert(MaxLevel < Texture.levels());

		return texture(
			Texture, TARGET_1D, Texture.format(),
			Texture.base_layer(), Texture.max_layer(),
			Texture.base_face(), Texture.max_face(),
			Texture.base_level() + BaseLevel, Texture.base_level() + MaxLevel);
	}

	inline texture view
	(
		texture1DArray const & Texture,
		texture1DArray::size_type BaseLayer, texture1DArray::size_type MaxLayer,
		texture1DArray::size_type BaseLevel, texture1DArray::size_type MaxLevel
	)
	{
		assert(BaseLevel <= MaxLevel);
		assert(BaseLevel < Texture.levels());
		assert(MaxLevel < Texture.levels());
		assert(BaseLayer <= MaxLayer);
		assert(BaseLayer < Texture.layers());
		assert(MaxLayer < Texture.layers());

		return texture(
			Texture, TARGET_1D_ARRAY, Texture.format(),
			Texture.base_layer() + BaseLayer, Texture.base_layer() + MaxLayer,
			Texture.base_face(), Texture.max_face(),
			Texture.base_level() + BaseLevel, Texture.base_level() + MaxLevel);
	}

	inline texture view
	(
		texture2D const & Texture,
		texture2D::size_type BaseLevel, texture2D::size_type MaxLevel
	)
	{
		assert(BaseLevel <= MaxLevel);
		assert(BaseLevel < Texture.levels());
		assert(MaxLevel < Texture.levels());

		return texture(
			Texture, TARGET_2D, Texture.format(),
			Texture.base_layer(), Texture.max_layer(),
			Texture.base_face(), Texture.max_face(),
			Texture.base_level() + BaseLevel, Texture.base_level() + MaxLevel);
	}

	inline texture view
	(
		texture2DArray const & Texture,
		texture2DArray::size_type BaseLayer, texture2DArray::size_type MaxLayer,
		texture2DArray::size_type BaseLevel, texture2DArray::size_type MaxLevel
	)
	{
		assert(BaseLevel <= MaxLevel);
		assert(BaseLevel < Texture.levels());
		assert(MaxLevel < Texture.levels());
		assert(BaseLayer <= MaxLayer);
		assert(BaseLayer < Texture.layers());
		assert(MaxLayer < Texture.layers());

		return texture(
			Texture, TARGET_2D_ARRAY, Texture.format(),
			Texture.base_layer() + BaseLayer, Texture.base_layer() + MaxLayer,
			Texture.base_face(), Texture.max_face(),
			Texture.base_level() + BaseLevel, Texture.base_level() + MaxLevel);
	}

	inline texture view
	(
		texture3D const & Texture,
		texture3D::size_type BaseLevel, texture3D::size_type MaxLevel
	)
	{
		assert(BaseLevel <= MaxLevel);
		assert(BaseLevel < Texture.levels());
		assert(MaxLevel < Texture.levels());

		return texture(
			Texture, TARGET_3D, Texture.format(),
			Texture.base_layer(), Texture.max_layer(),
			Texture.base_face(), Texture.max_face(),
			Texture.base_level() + BaseLevel, Texture.base_level() + MaxLevel);
	}

	inline texture view
	(
		textureCube const & Texture,
		textureCube::size_type BaseFace, textureCube::size_type MaxFace,
		textureCube::size_type BaseLevel, textureCube::size_type MaxLevel
	)
	{
		assert(BaseLevel <= MaxLevel);
		assert(BaseLevel < Texture.levels());
		assert(MaxLevel < Texture.levels());
		assert(BaseFace <= MaxFace);
		assert(BaseFace < Texture.faces());
		assert(MaxFace < Texture.faces());

		return texture(
			Texture, TARGET_CUBE, Texture.format(),
			Texture.base_layer(), Texture.max_layer(),
			Texture.base_face(), Texture.base_face() + MaxFace,
			Texture.base_level() + BaseLevel, Texture.base_level() + MaxLevel);
	}

	inline texture view
	(
		textureCubeArray const & Texture,
		textureCubeArray::size_type BaseLayer, textureCubeArray::size_type MaxLayer,
		textureCubeArray::size_type BaseFace, textureCubeArray::size_type MaxFace,
		textureCubeArray::size_type BaseLevel, textureCubeArray::size_type MaxLevel
	)
	{
		assert(BaseLevel <= MaxLevel);
		assert(BaseLevel < Texture.levels());
		assert(MaxLevel < Texture.levels());
		assert(BaseFace <= MaxFace);
		assert(BaseFace < Texture.faces());
		assert(MaxFace < Texture.faces());
		assert(BaseLayer <= MaxLayer);
		assert(BaseLayer < Texture.layers());
		assert(MaxLayer < Texture.layers());

		return texture(
			Texture, TARGET_CUBE_ARRAY, Texture.format(),
			Texture.base_layer() + BaseLayer, Texture.base_layer() + MaxLayer,
			Texture.base_face() + BaseFace, Texture.base_face() + MaxFace,
			Texture.base_level() + BaseLevel, Texture.base_level() + MaxLevel);
	}
}//namespace gli
