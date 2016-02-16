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
/// @file gli/core/load_ktx.inl
/// @date 2015-08-05 / 2015-08-05
/// @author Christophe Riccio
///////////////////////////////////////////////////////////////////////////////////

#include "../gl.hpp"
#include <cstdio>
#include <cassert>

namespace gli{
namespace detail
{
	struct ktxHeader
	{
		char Identifier[12];
		std::uint32_t Endianness;
		std::uint32_t GLType;
		std::uint32_t GLTypeSize;
		std::uint32_t GLFormat;
		std::uint32_t GLInternalFormat;
		std::uint32_t GLBaseInternalFormat;
		std::uint32_t PixelWidth;
		std::uint32_t PixelHeight;
		std::uint32_t PixelDepth;
		std::uint32_t NumberOfArrayElements;
		std::uint32_t NumberOfFaces;
		std::uint32_t NumberOfMipmapLevels;
		std::uint32_t BytesOfKeyValueData;
	};

	inline target getTarget(ktxHeader const & Header)
	{
		if(Header.NumberOfFaces > 1)
		{
			if(Header.NumberOfArrayElements > 0)
				return TARGET_CUBE_ARRAY;
			else
				return TARGET_CUBE;
		}
		else if(Header.NumberOfArrayElements > 0)
		{
			if(Header.PixelHeight == 0)
				return TARGET_1D_ARRAY;
			else
				return TARGET_2D_ARRAY;
		}
		else if(Header.PixelHeight == 0)
			return TARGET_1D;
		else if(Header.PixelDepth > 0)
			return TARGET_3D;
		else
			return TARGET_2D;
	}
}//namespace detail

	inline texture load_ktx(char const * Data, std::size_t Size)
	{
		assert(Data && (Size >= sizeof(detail::ktxHeader)));

		detail::ktxHeader const & Header(*reinterpret_cast<detail::ktxHeader const *>(Data));

		static unsigned char const Identifier[] =
		{
			0xAB, 0x4B, 0x54, 0x58, 0x20, 0x31, 0x31, 0xBB, 0x0D, 0x0A, 0x1A, 0x0A
		};

		if(memcmp(Header.Identifier, Identifier, sizeof(Identifier)) != 0)
			return texture();

		size_t Offset = sizeof(detail::ktxHeader);

		// Skip key value data
		Offset += Header.BytesOfKeyValueData;

		gl GL;
		gli::format const Format = GL.find(
			static_cast<gli::gl::internalFormat>(Header.GLInternalFormat),
			static_cast<gli::gl::externalFormat>(Header.GLFormat),
			static_cast<gli::gl::typeFormat>(Header.GLType));
		assert(Format != static_cast<format>(gli::FORMAT_INVALID));
		
		std::uint32_t const BlockSize = block_size(Format);

		texture Texture(
			detail::getTarget(Header),
			Format,
			texture::dim_type(
				Header.PixelWidth,
				std::max<std::uint32_t>(Header.PixelHeight, 1),
				std::max<std::uint32_t>(Header.PixelDepth, 1)),
			std::max<std::uint32_t>(Header.NumberOfArrayElements, 1),
			std::max<std::uint32_t>(Header.NumberOfFaces, 1),
			std::max<std::uint32_t>(Header.NumberOfMipmapLevels, 1));

		for(std::size_t Level = 0, Levels = Texture.levels(); Level < Levels; ++Level)
		{
			Offset += sizeof(std::uint32_t);

			for(std::size_t Layer = 0, Layers = Texture.layers(); Layer < Layers; ++Layer)
			{
				for(std::size_t Face = 0, Faces = Texture.faces(); Face < Faces; ++Face)
				{
					std::uint32_t const FaceSize = static_cast<std::uint32_t>(Texture.size(Level));

					std::memcpy(Texture.data(Layer, Face, Level), Data + Offset, FaceSize);

					Offset += std::max(BlockSize, glm::ceilMultiple(FaceSize, static_cast<std::uint32_t>(4)));
				}
			}
		}

		return Texture;
	}

	inline texture load_ktx(char const * Filename)
	{
		FILE* File = std::fopen(Filename, "rb");
		if(!File)
			return texture();

		long Beg = std::ftell(File);
		std::fseek(File, 0, SEEK_END);
		long End = std::ftell(File);
		std::fseek(File, 0, SEEK_SET);

		std::vector<char> Data(static_cast<std::size_t>(End - Beg));

		std::fread(&Data[0], 1, Data.size(), File);
		std::fclose(File);

		return load_ktx(&Data[0], Data.size());
	}

	inline texture load_ktx(std::string const & Filename)
	{
		return load_ktx(Filename.c_str());
	}
}//namespace gli
