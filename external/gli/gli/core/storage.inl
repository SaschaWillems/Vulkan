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
/// @file gli/core/storage.inl
/// @date 2012-06-21 / 2015-08-22
/// @author Christophe Riccio
///////////////////////////////////////////////////////////////////////////////////

namespace gli
{
	inline storage::storage()
		: Layers(0)
		, Faces(0)
		, Levels(0)
		, BlockSize(0)
		, BlockCount(0)
		, Dimensions(0)
	{}

	inline storage::storage
	(
		format_type Format,
		dim_type const & Dimensions,
		size_type Layers,
		size_type Faces,
		size_type Levels
	)
		: Layers(Layers)
		, Faces(Faces)
		, Levels(Levels)
		, BlockSize(gli::block_size(Format))
		, BlockCount(glm::max(Dimensions / block_dimensions(Format), gli::dim3_t(1)))
		, Dimensions(Dimensions)
	{
		assert(Layers > 0);
		assert(Faces > 0);
		assert(Levels > 0);
		assert(glm::all(glm::greaterThan(Dimensions, dim_type(0))));

		this->Data.resize(this->layer_size(0, Faces - 1, 0, Levels - 1) * Layers, 0);
	}

	inline bool storage::empty() const
	{
		return this->Data.empty();
	}

	inline storage::size_type storage::layers() const
	{
		return this->Layers;
	}

	inline storage::size_type storage::faces() const
	{
		return this->Faces;
	}

	inline storage::size_type storage::levels() const
	{
		return this->Levels;
	}

	inline storage::size_type storage::block_size() const
	{
		return this->BlockSize;
	}

	inline storage::dim_type storage::block_count(size_type Level) const
	{
		assert(Level < this->Levels);

		return glm::max(this->BlockCount >> storage::dim_type(static_cast<glm::uint>(Level)), storage::dim_type(static_cast<glm::uint>(1)));
	}

	inline storage::dim_type storage::dimensions(size_type Level) const
	{
		return glm::max(this->Dimensions >> Level, storage::dim_type(1));
	}

	inline storage::size_type storage::size() const
	{
		assert(!this->empty());

		return this->Data.size();
	}

	inline storage::data_type * storage::data()
	{
		assert(!this->empty());

		return &this->Data[0];
	}

	inline storage::size_type storage::offset
	(
		size_type Layer,
		size_type Face,
		size_type Level
	) const
	{
		assert(Layer < this->layers());
		assert(Face < this->faces());
		assert(Level < this->levels());

		size_t const LayerSize = this->layer_size(0, this->faces() - 1, 0, this->levels() - 1);
		size_t const FaceSize = this->face_size(0, this->levels() - 1);
		size_t BaseOffset = LayerSize * Layer + FaceSize * Face;

		for(size_t LevelIndex = 0, LevelCount = Level; LevelIndex < LevelCount; ++LevelIndex)
			BaseOffset += this->level_size(LevelIndex);

		return BaseOffset;
	}

	inline storage::size_type storage::level_size(size_type Level) const
	{
		assert(Level < this->levels());

		return this->BlockSize * glm::compMul(this->block_count(Level));
	}

	inline storage::size_type storage::face_size(
		size_type BaseLevel,
		size_type MaxLevel) const
	{
		assert(MaxLevel < this->levels());
		
		size_type FaceSize(0);

		// The size of a face is the sum of the size of each level.
		for(storage::size_type Level(BaseLevel); Level <= MaxLevel; ++Level)
			FaceSize += this->level_size(Level);

		return FaceSize;
	}

	inline storage::size_type storage::layer_size(
		size_type BaseFace, size_type MaxFace,
		size_type BaseLevel, size_type MaxLevel) const
	{
		assert(MaxFace < this->faces());
		assert(MaxLevel < this->levels());

		// The size of a layer is the sum of the size of each face.
		// All the faces have the same size.
		return this->face_size(BaseLevel, MaxLevel) * (MaxFace - BaseFace + 1);
	}

/*
	inline storage extractLayers
	(
		storage const & Storage, 
		storage::size_type const & Offset, 
		storage::size_type const & Size
	)
	{
		assert(Storage.layers() > 1);
		assert(Storage.layers() >= Size);
		assert(Storage.faces() > 0);
		assert(Storage.levels() > 0);

		storage SubStorage(
			Size, 
			Storage.faces(), 
			Storage.levels(),
			Storage.dimensions(0),
			Storage.blockSize());

		memcpy(
			SubStorage.data(), 
			Storage.data() + Storage.imageAddressing(Offset, 0, 0), 
			Storage.layerSize() * Size);

		return SubStorage;
	}
*/
/*
	inline storage extractFace
	(
		storage const & Storage, 
		face const & Face
	)
	{
		assert(Storage.faces() > 1);
		assert(Storage.levels() > 0);

		storage SubStorage(
			Storage.layers(),
			Face, 
			Storage.levels(),
			Storage.dimensions(0),
			Storage.blockSize());

		memcpy(
			SubStorage.data(), 
			Storage.data() + Storage.imageAddressing(0, storage::size_type(Face), 0), 
			Storage.faceSize());

		return SubStorage;
	}
*/
/*
	inline storage extractLevel
	(
		storage const & Storage, 
		storage::size_type const & Level
	)
	{
		assert(Storage.layers() == 1);
		assert(Storage.faces() == 1);
		assert(Storage.levels() >= 1);

		storage SubStorage(
			1, // layer
			glm::uint(FACE_DEFAULT),
			1, // level
			Storage.dimensions(0),
			Storage.blockSize());

		memcpy(
			SubStorage.data(), 
			Storage.data() + Storage.imageAddressing(0, 0, Level), 
			Storage.levelSize(Level));

		return SubStorage;
	}
*/
/*
	inline void copy_layers
	(
		storage const & SourceStorage, 
		storage::size_type const & SourceLayerOffset,
		storage::size_type const & SourceLayerSize,
		storage & DestinationStorage, 
		storage::size_type const & DestinationLayerOffset
	)
	{
		assert(DestinationStorage.blockSize() == SourceStorage.blockSize());
		assert(DestinationStorage.layers() <= SourceStorage.layers());
		assert(SourceStorage.layers() <= SourceLayerOffset + SourceLayerSize);
		assert(DestinationStorage.layers() <= DestinationLayerOffset + SourceLayerSize);

		std::size_t OffsetSrc = SourceStorage.imageAddressing(SourceLayerOffset, 0, 0);
		std::size_t OffsetDst = DestinationStorage.imageAddressing(DestinationLayerOffset, 0, 0);

		memcpy(
			DestinationStorage.data() + OffsetDst * DestinationStorage.blockSize(), 
			SourceStorage.data() + OffsetSrc * SourceStorage.blockSize(), 
			SourceStorage.layerSize() * SourceLayerSize * SourceStorage.blockSize());
	}
*/
}//namespace gli
