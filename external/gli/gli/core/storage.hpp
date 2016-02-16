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
/// @file gli/core/storage.hpp
/// @date 2012-06-21 / 2015-08-22
/// @author Christophe Riccio
///////////////////////////////////////////////////////////////////////////////////

#pragma once

// STD
#include <vector>
#include <queue>
#include <string>
#include <cassert>
#include <cmath>
#include <cstring>
#include <memory>

#include "../type.hpp"
#include "../format.hpp"

// GLM
#include <glm/gtc/round.hpp>
#include <glm/gtx/component_wise.hpp>
#include <glm/gtx/integer.hpp>
#include <glm/gtx/bit.hpp>
#include <glm/gtx/raw_data.hpp>
#include <glm/gtx/wrap.hpp>

static_assert(GLM_VERSION >= 97, "GLI requires at least GLM 0.9.7");

namespace gli
{
	class storage
	{
	public:
		typedef dim3_t dim_type;
		typedef size_t size_type;
		typedef gli::format format_type;
		typedef glm::byte data_type;

	public:
		storage();

		storage(
			format_type Format,
			dim_type const & Dimensions,
			size_type Layers,
			size_type Faces,
			size_type Levels);

		bool empty() const;
		size_type size() const; // Express is bytes
		size_type layers() const;
		size_type levels() const;
		size_type faces() const;

		size_type block_size() const;
		dim_type block_count(size_type Level) const;
		dim_type dimensions(size_type Level) const;

		data_type * data();

		/// Compute the relative memory offset to access the data for a specific layer, face and level
		size_type offset(
			size_type Layer,
			size_type Face,
			size_type Level) const;

		size_type level_size(
			size_type Level) const;
		size_type face_size(
			size_type BaseLevel, size_type MaxLevel) const;
		size_type layer_size(
			size_type BaseFace, size_type MaxFace,
			size_type BaseLevel, size_type MaxLevel) const;

	private:
		size_type const Layers;
		size_type const Faces;
		size_type const Levels;
		size_type const BlockSize;
		dim_type const BlockCount;
		dim_type const Dimensions;
		std::vector<data_type> Data;
	};

/*
	storage extractLayers(
		storage const & Storage, 
		storage::size_type const & Offset, 
		storage::size_type const & Size);
*/
/*
	storage extractFace(
		storage const & Storage, 
		face const & Face);
*/
/*
	storage extractLevels(
		storage const & Storage, 
		storage::size_type const & Offset, 
		storage::size_type const & Size);
*/
/*
	void copy_layers(
		storage const & SourceStorage, 
		storage::size_type const & SourceLayerOffset,
		storage::size_type const & SourceLayerSize,
		storage & DestinationStorage, 
		storage::size_type const & DestinationLayerOffset);

	void copy_faces(
		storage const & SourceStorage, 
		storage::size_type const & SourceLayerOffset,
		storage::size_type const & SourceFaceOffset,
		storage::size_type const & SourceLayerSize,
		storage & DestinationStorage, 
		storage::size_type const & DestinationLayerOffset,
		storage::size_type const & DestinationFaceOffset);

	void copy_levels(
		storage const & SourceStorage, 
		storage::size_type const & SourceLayerOffset,
		storage::size_type const & SourceFaceOffset,
		storage::size_type const & SourceLevelOffset,
		storage::size_type const & SourceLayerSize,
		storage & DestinationStorage, 
		storage::size_type const & DestinationLayerOffset,
		storage::size_type const & DestinationFaceOffset,
		storage::size_type const & DestinationlevelOffset);
*/

}//namespace gli

#include "storage.inl"
