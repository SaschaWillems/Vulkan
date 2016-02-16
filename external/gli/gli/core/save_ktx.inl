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
/// @file gli/core/save_ktx.inl
/// @date 2015-08-05 / 2015-08-05
/// @author Christophe Riccio
///////////////////////////////////////////////////////////////////////////////////

#include <cstdio>
#include <glm/gtc/round.hpp>

namespace gli{
namespace detail
{
	inline std::size_t compute_ktx_storage_size(texture const & Texture)
	{
		std::uint32_t const BlockSize = block_size(Texture.format());
		std::size_t TotalSize = sizeof(detail::ktxHeader);

		for(std::size_t Level = 0, Levels = Texture.levels(); Level < Levels; ++Level)
		{
			TotalSize += sizeof(std::uint32_t);

			for(std::size_t Layer = 0, Layers = Texture.layers(); Layer < Layers; ++Layer)
			{
				for(std::size_t Face = 0, Faces = Texture.faces(); Face < Faces; ++Face)
				{
					std::uint32_t const FaceSize = static_cast<std::uint32_t>(Texture.size(Level));
					std::uint32_t const PaddedSize = std::max(BlockSize, glm::ceilMultiple(FaceSize, static_cast<std::uint32_t>(4)));

					TotalSize += PaddedSize;
				}
			}
		}

		return TotalSize;
	}
	
}//namespace detail

	inline bool save_ktx(texture const & Texture, std::vector<char> & Memory)
	{
		if(Texture.empty())
			return false;

		gl GL;
		gl::format const & Format = GL.translate(Texture.format());
		target const Target = Texture.target();

		detail::formatInfo const & Desc = detail::get_format_info(Texture.format());

		Memory.resize(detail::compute_ktx_storage_size(Texture) + sizeof(detail::ktxHeader));

		detail::ktxHeader & Header = *reinterpret_cast<detail::ktxHeader*>(&Memory[0]);

		static unsigned char const Identifier[] =
		{
		   0xAB, 0x4B, 0x54, 0x58, 0x20, 0x31, 0x31, 0xBB, 0x0D, 0x0A, 0x1A, 0x0A
		};

		memcpy(Header.Identifier, Identifier, sizeof(Header.Identifier));
		Header.Endianness = 0x04030201;
		Header.GLType = Format.Type;
		Header.GLTypeSize = Format.Type == gl::TYPE_NONE ? 1 : Desc.BlockSize;
		Header.GLFormat = Format.External;
		Header.GLInternalFormat = Format.Internal;
		Header.GLBaseInternalFormat = Format.External;
		Header.PixelWidth = static_cast<std::uint32_t>(Texture.dimensions().x);
		Header.PixelHeight = !is_target_1d(Target) ? static_cast<std::uint32_t>(Texture.dimensions().y) : 0;
		Header.PixelDepth = Target == TARGET_3D ? static_cast<std::uint32_t>(Texture.dimensions().z) : 0;
		Header.NumberOfArrayElements = is_target_array(Target) ? static_cast<std::uint32_t>(Texture.layers()) : 0;
		Header.NumberOfFaces = is_target_cube(Target) ? static_cast<std::uint32_t>(Texture.faces()) : 0;
		Header.NumberOfMipmapLevels = static_cast<std::uint32_t>(Texture.levels());
		Header.BytesOfKeyValueData = 0;

		std::size_t Offset = sizeof(detail::ktxHeader);

		for(std::size_t Level = 0, Levels = Texture.levels(); Level < Levels; ++Level)
		{
			std::uint32_t& ImageSize = *reinterpret_cast<std::uint32_t*>(&Memory[0] + Offset);
			Offset += sizeof(std::uint32_t);

			for(std::size_t Layer = 0, Layers = Texture.layers(); Layer < Layers; ++Layer)
			{
				for(std::size_t Face = 0, Faces = Texture.faces(); Face < Faces; ++Face)
				{
					std::uint32_t const FaceSize = static_cast<std::uint32_t>(Texture.size(Level));

					std::memcpy(&Memory[0] + Offset, Texture.data(Layer, Face, Level), FaceSize);

					std::uint32_t const PaddedSize = glm::ceilMultiple(FaceSize, static_cast<std::uint32_t>(4));

					ImageSize += PaddedSize;
					Offset += PaddedSize;

					assert(Offset <= Memory.size());
				}
			}

			ImageSize = glm::ceilMultiple(ImageSize, static_cast<std::uint32_t>(4));
		}

		return true;
	}

	inline bool save_ktx(texture const & Texture, char const * Filename)
	{
		if(Texture.empty())
			return false;

		FILE* File = std::fopen(Filename, "wb");
		if(!File)
			return false;

		std::vector<char> Memory;
		bool const Result = save_ktx(Texture, Memory);

		std::fwrite(&Memory[0], 1, Memory.size(), File);
		std::fclose(File);

		return Result;
	}

	inline bool save_ktx(texture const & Texture, std::string const & Filename)
	{
		return save_ktx(Texture, Filename.c_str());
	}
}//namespace gli
