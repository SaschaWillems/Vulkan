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
/// @file gli/core/image.inl
/// @date 2011-10-06 / 2013-01-12
/// @author Christophe Riccio
///////////////////////////////////////////////////////////////////////////////////

namespace gli{
namespace detail
{
	inline size_t texelLinearAdressing
	(
		dim1_t const & Dimensions,
		dim1_t const & TexelCoord
	)
	{
		assert(glm::all(glm::lessThan(TexelCoord, Dimensions)));

		return TexelCoord.x;
	}

	inline size_t texelLinearAdressing
	(
		dim2_t const & Dimensions,
		dim2_t const & TexelCoord
	)
	{
		assert(TexelCoord.x < Dimensions.x);
		assert(TexelCoord.y < Dimensions.y);

		return TexelCoord.x + Dimensions.x * TexelCoord.y;
	}

	inline size_t texelLinearAdressing
	(
		dim3_t const & Dimensions,
		dim3_t const & TexelCoord
	)
	{
		assert(TexelCoord.x < Dimensions.x);
		assert(TexelCoord.y < Dimensions.y);
		assert(TexelCoord.z < Dimensions.z);

		return TexelCoord.x + Dimensions.x * (TexelCoord.y + Dimensions.y * TexelCoord.z);
	}

	inline size_t texelMortonAdressing
	(
		dim1_t const & Dimensions,
		dim1_t const & TexelCoord
	)
	{
		assert(TexelCoord.x < Dimensions.x);

		return TexelCoord.x;
	}

	inline size_t texelMortonAdressing
	(
		dim2_t const & Dimensions,
		dim2_t const & TexelCoord
	)
	{
		assert(TexelCoord.x < Dimensions.x && TexelCoord.x < std::numeric_limits<std::uint32_t>::max());
		assert(TexelCoord.y < Dimensions.y && TexelCoord.y < std::numeric_limits<std::uint32_t>::max());

		glm::u32vec2 const Input(TexelCoord);

		return glm::bitfieldInterleave(Input.x, Input.y);
	}

	inline size_t texelMortonAdressing
	(
		dim3_t const & Dimensions,
		dim3_t const & TexelCoord
	)
	{
		assert(TexelCoord.x < Dimensions.x);
		assert(TexelCoord.y < Dimensions.y);
		assert(TexelCoord.z < Dimensions.z);

		glm::u32vec3 const Input(TexelCoord);

		return glm::bitfieldInterleave(Input.x, Input.y, Input.z);
	}
}//namespace detail

	inline image::image()
		: Format(static_cast<gli::format>(FORMAT_INVALID))
		, BaseLevel(0)
		, Data(nullptr)
		, Size(0)
	{}

	inline image::image
	(
		format_type Format,
		dim_type const & Dimensions
	)
		: Storage(std::make_shared<storage>(Format, Dimensions, 1, 1, 1))
		, Format(Format)
		, BaseLevel(0)
		, Data(Storage->data())
		, Size(compute_size(0))
	{}

	inline image::image
	(
		std::shared_ptr<storage> Storage,
		format_type Format,
		size_type BaseLayer,
		size_type BaseFace,
		size_type BaseLevel
	)
		: Storage(Storage)
		, Format(Format)
		, BaseLevel(BaseLevel)
		, Data(compute_data(BaseLayer, BaseFace, BaseLevel))
		, Size(compute_size(BaseLevel))
	{}

	inline image::image
	(
		image const & Image,
		format_type Format
	)
		: Storage(Image.Storage)
		, Format(Format)
		, BaseLevel(Image.BaseLevel)
		, Data(Image.Data)
		, Size(Image.Size)
	{
		assert(block_size(Format) == block_size(Image.format()));
	}

	inline bool image::empty() const
	{
		if(this->Storage.get() == nullptr)
			return true;

		return this->Storage->empty();
	}

	inline image::size_type image::size() const
	{
		assert(!this->empty());

		return this->Size;
	}

	template <typename genType>
	inline image::size_type image::size() const
	{
		assert(sizeof(genType) <= this->Storage->block_size());

		return this->size() / sizeof(genType);
	}

	inline image::format_type image::format() const
	{
		return this->Format;
	}

	inline image::dim_type image::dimensions() const
	{
		assert(!this->empty());

		return this->Storage->dimensions(this->BaseLevel);

		//return this->Storage->block_count(this->BaseLevel) * block_dimensions(this->format());
	}

	inline void * image::data()
	{
		assert(!this->empty());

		return this->Data;
	}

	inline void const * image::data() const
	{
		assert(!this->empty());
		
		return this->Data;
	}

	template <typename genType>
	inline genType * image::data()
	{
		assert(!this->empty());
		assert(this->Storage->block_size() >= sizeof(genType));

		return reinterpret_cast<genType *>(this->data());
	}

	template <typename genType>
	inline genType const * image::data() const
	{
		assert(!this->empty());
		assert(this->Storage->block_size() >= sizeof(genType));

		return reinterpret_cast<genType const *>(this->data());
	}

	inline void image::clear()
	{
		assert(!this->empty());

		memset(this->data<glm::byte>(), 0, this->size<glm::byte>());
	}

	template <typename genType>
	inline void image::clear(genType const & Texel)
	{
		assert(!this->empty());
		assert(this->Storage->block_size() == sizeof(genType));

		for(size_type TexelIndex = 0; TexelIndex < this->size<genType>(); ++TexelIndex)
			*(this->data<genType>() + TexelIndex) = Texel;
	}

	inline image::data_type * image::compute_data(size_type BaseLayer, size_type BaseFace, size_type BaseLevel)
	{
		size_type const Offset = this->Storage->offset(BaseLayer, BaseFace, BaseLevel);

		return this->Storage->data() + Offset;
	}

	inline image::size_type image::compute_size(size_type Level) const
	{
		assert(!this->empty());

		return this->Storage->level_size(Level);
	}

	template <typename genType>
	genType image::load(dim_type const & TexelCoord)
	{
		assert(!this->empty());
		assert(!is_compressed(this->format()));
		assert(this->Storage->block_size() == sizeof(genType));
		assert(glm::all(glm::lessThan(TexelCoord, this->dimensions())));

		return *(this->data<genType>() + detail::texelLinearAdressing(this->dimensions(), TexelCoord));
	}

	template <typename genType>
	void image::store(dim_type const & TexelCoord, genType const & Data)
	{
		assert(!this->empty());
		assert(!is_compressed(this->format()));
		assert(this->Storage->block_size() == sizeof(genType));
		assert(glm::all(glm::lessThan(TexelCoord, this->dimensions())));

		*(this->data<genType>() + detail::texelLinearAdressing(this->dimensions(), TexelCoord)) = Data;
	}
}//namespace gli
