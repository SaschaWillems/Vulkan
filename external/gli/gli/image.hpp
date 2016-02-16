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
/// @file gli/image.hpp
/// @date 2011-10-06 / 2013-01-12
/// @author Christophe Riccio
///
/// @defgroup core_image Image 
/// @ingroup core
///////////////////////////////////////////////////////////////////////////////////

#pragma once

#include "./core/storage.hpp"

namespace gli
{
	/// Image
	class image
	{
	private:
		friend class texture1D;
		friend class texture2D;
		friend class texture3D;

	public:
		typedef size_t size_type;
		typedef gli::format format_type;
		typedef storage::dim_type dim_type;
		typedef storage::data_type data_type;

		/// Create an empty image instance
		image();

		/// Create an image object and allocate an image storoge for it.
		explicit image(
			format_type Format,
			dim_type const & Dimensions);

		/// Create an image object by sharing an existing image storage from another image instance.
		/// This image object is effectively an image view where format can be reinterpreted
		/// with a different compatible image format.
		/// For formats to be compatible, the block size of source and destination must match.
		explicit image(
			image const & Image,
			format_type Format);

		/// Return whether the image instance is empty, no storage or description have been assigned to the instance.
		bool empty() const;

		/// Return the image instance format.
		format_type format() const;

		/// Return the dimensions of an image instance: width, height and depth.
		dim_type dimensions() const;

		/// Return the memory size of an image instance storage in bytes.
		size_type size() const;

		/// Return the number of blocks contained in an image instance storage.
		/// genType size must match the block size conresponding to the image format. 
		template <typename genType>
		size_type size() const;

		/// Return a pointer to the beginning of the image instance data.
		void * data();

		/// Return a pointer to the beginning of the image instance data.
		void const * data() const;

		/// Return a pointer of type genType which size must match the image format block size.
		template <typename genType>
		genType * data();

		/// Return a pointer of type genType which size must match the image format block size.
		template <typename genType>
		genType const * data() const;

		/// Clear the entire image storage with zeros
		void clear();

		/// Clear the entire image storage with Texel which type must match the image storage format block size
		/// If the type of genType doesn't match the type of the image format, no conversion is performed and the data will be reinterpreted as if is was of the image format. 
		template <typename genType>
		void clear(genType const & Texel);

		/// Load the texel located at TexelCoord coordinates.
		/// It's an error to call this function if the format is compressed.
		/// It's an error if TexelCoord values aren't between [0, dimensions].
		template <typename genType>
		genType load(dim_type const & TexelCoord);

		/// Store the texel located at TexelCoord coordinates.
		/// It's an error to call this function if the format is compressed.
		/// It's an error if TexelCoord values aren't between [0, dimensions].
		template <typename genType>
		void store(dim_type const & TexelCoord, genType const & Data);

	private:
		/// Create an image object by sharing an existing image storage from another image instance.
		/// This image object is effectively an image view where the layer, the face and the level allows identifying
		/// a specific subset of the image storage source. 
		/// This image object is effectively a image view where the format can be reinterpreted
		/// with a different compatible image format.
		explicit image(
			std::shared_ptr<storage> Storage,
			format_type Format,
			size_type BaseLayer,
			size_type BaseFace,
			size_type BaseLevel);

		std::shared_ptr<storage> Storage;
		format_type const Format;
		size_type const BaseLevel;
		data_type * Data;
		size_type const Size;

		data_type * compute_data(size_type BaseLayer, size_type BaseFace, size_type BaseLevel);
		size_type compute_size(size_type Level) const;
	};
}//namespace gli

#include "./core/image.inl"
