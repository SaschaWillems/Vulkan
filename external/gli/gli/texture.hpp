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
/// @file gli/texture.hpp
/// @date 2013-02-05 / 2013-02-05
/// @author Christophe Riccio
///////////////////////////////////////////////////////////////////////////////////

#pragma once

#include "image.hpp"
#include "target.hpp"

namespace gli
{
	/// Genetic texture class. It can support any target.
	class texture
	{
	public:
		typedef size_t size_type;
		typedef gli::format format_type;
		typedef gli::target target_type;
		typedef storage::dim_type dim_type;
		typedef storage::data_type data_type;

		/// Create an empty texture instance
		texture();

		/// Create a texture object and allocate a texture storoge for it
		texture(
			target_type Target,
			format_type Format,
			dim_type const & Dimensions,
			size_type Layers,
			size_type Faces,
			size_type Levels);

		/// Create a texture object by sharing an existing texture storage from another texture instance.
		/// This texture object is effectively a texture view where the layer, the face and the level allows identifying
		/// a specific subset of the texture storage source. 
		/// This texture object is effectively a texture view where the target and format can be reinterpreted
		/// with a different compatible texture target and texture format.
		texture(
			texture const & Texture,
			target_type Target,
			format_type Format,
			size_type BaseLayer, size_type MaxLayer,
			size_type BaseFace, size_type MaxFace,
			size_type BaseLevel, size_type MaxLevel);

		/// Create a texture object by sharing an existing texture storage from another texture instance.
		/// This texture object is effectively a texture view where the target and format can be reinterpreted
		/// with a different compatible texture target and texture format.
		texture(
			texture const & Texture,
			target_type Target,
			format_type Format);

		virtual ~texture(){}

		/// Return whether the texture instance is empty, no storage or description have been assigned to the instance.
		bool empty() const;

		/// Return the target of a texture instance. 
		target_type target() const{return this->Target;}

		/// Return the texture instance format
		format_type format() const;

		/// Return the base layer of the texture instance, effectively a memory offset in the actual texture storage to identify where to start reading the layers. 
		size_type base_layer() const;

		/// Return the max layer of the texture instance, effectively a memory offset to the beginning of the last layer in the actual texture storage that the texture instance can access. 
		size_type max_layer() const;

		/// Return max_layer() - base_layer() + 1
		size_type layers() const;

		/// Return the base face of the texture instance, effectively a memory offset in the actual texture storage to identify where to start reading the faces. 
		size_type base_face() const;

		/// Return the max face of the texture instance, effectively a memory offset to the beginning of the last face in the actual texture storage that the texture instance can access. 
		size_type max_face() const;

		/// Return max_face() - base_face() + 1
		size_type faces() const;

		/// Return the base level of the texture instance, effectively a memory offset in the actual texture storage to identify where to start reading the levels. 
		size_type base_level() const;

		/// Return the max level of the texture instance, effectively a memory offset to the beginning of the last level in the actual texture storage that the texture instance can access. 
		size_type max_level() const;

		/// Return max_level() - base_level() + 1.
		size_type levels() const;

		/// Return the dimensions of a texture instance: width, height and depth.
		dim_type dimensions(size_type Level = 0) const;

		/// Return the memory size of a texture instance storage in bytes.
		size_type size() const;

		/// Return the number of blocks contained in a texture instance storage.
		/// genType size must match the block size conresponding to the texture format.
		template <typename genType>
		size_type size() const;

		/// Return the memory size of a specific level identified by Level.
		size_type size(size_type Level) const;

		/// Return the memory size of a specific level identified by Level.
		/// genType size must match the block size conresponding to the texture format.
		template <typename genType>
		size_type size(size_type Level) const;

		/// Return a pointer to the beginning of the texture instance data.
		void * data();

		/// Return a pointer of type genType which size must match the texture format block size
		template <typename genType>
		genType * data();

		/// Return a pointer to the beginning of the texture instance data.
		void const * data() const;

		/// Return a pointer of type genType which size must match the texture format block size
		template <typename genType>
		genType const * data() const;

		/// Return a pointer to the beginning of the texture instance data.
		void * data(size_type Layer, size_type Face, size_type Level);

		/// Return a pointer to the beginning of the texture instance data.
		void const * data(size_type Layer, size_type Face, size_type Level) const;

		/// Return a pointer of type genType which size must match the texture format block size
		template <typename genType>
		genType * data(size_type Layer, size_type Face, size_type Level);

		/// Return a pointer of type genType which size must match the texture format block size
		template <typename genType>
		genType const * data(size_type Layer, size_type Face, size_type Level) const;

		/// Clear the entire texture storage with zeros
		void clear();

		/// Clear the entire texture storage with Texel which type must match the texture storage format block size
		/// If the type of genType doesn't match the type of the texture format, no conversion is performed and the data will be reinterpreted as if is was of the texture format. 
		template <typename genType>
		void clear(genType const & Texel);

	protected:
		/// Compute the relative memory offset to access the data for a specific layer, face and level
		size_type offset(size_type Layer, size_type Face, size_type Level) const;

		struct cache
		{
			data_type * Data;
			size_type Size;
		};

		std::shared_ptr<storage> Storage;
		target_type const Target;
		format_type const Format;
		size_type const BaseLayer;
		size_type const MaxLayer;
		size_type const BaseFace;
		size_type const MaxFace;
		size_type const BaseLevel;
		size_type const MaxLevel;
		cache Cache;

	private:
		void build_cache();
	};

}//namespace gli

#include "./core/texture.inl"

