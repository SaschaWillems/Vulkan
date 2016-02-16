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
/// @file gli/core/load_dds.inl
/// @date 2010-09-26 / 2015-06-16
/// @author Christophe Riccio
///////////////////////////////////////////////////////////////////////////////////

#include "../dx.hpp"
#include <cstdio>
#include <cassert>

namespace gli{
namespace detail
{
	enum ddsCubemapflag
	{
		DDSCAPS2_CUBEMAP				= 0x00000200,
		DDSCAPS2_CUBEMAP_POSITIVEX		= 0x00000400,
		DDSCAPS2_CUBEMAP_NEGATIVEX		= 0x00000800,
		DDSCAPS2_CUBEMAP_POSITIVEY		= 0x00001000,
		DDSCAPS2_CUBEMAP_NEGATIVEY		= 0x00002000,
		DDSCAPS2_CUBEMAP_POSITIVEZ		= 0x00004000,
		DDSCAPS2_CUBEMAP_NEGATIVEZ		= 0x00008000,
		DDSCAPS2_VOLUME					= 0x00200000
	};

	enum
	{
		DDSCAPS2_CUBEMAP_ALLFACES = DDSCAPS2_CUBEMAP_POSITIVEX | DDSCAPS2_CUBEMAP_NEGATIVEX | DDSCAPS2_CUBEMAP_POSITIVEY | DDSCAPS2_CUBEMAP_NEGATIVEY | DDSCAPS2_CUBEMAP_POSITIVEZ | DDSCAPS2_CUBEMAP_NEGATIVEZ
	};

	enum ddsFlag
	{
		DDSD_CAPS			= 0x00000001,
		DDSD_HEIGHT			= 0x00000002,
		DDSD_WIDTH			= 0x00000004,
		DDSD_PITCH			= 0x00000008,
		DDSD_PIXELFORMAT	= 0x00001000,
		DDSD_MIPMAPCOUNT	= 0x00020000,
		DDSD_LINEARSIZE		= 0x00080000,
		DDSD_DEPTH			= 0x00800000
	};

	enum ddsSurfaceflag
	{
		DDSCAPS_COMPLEX				= 0x00000008,
		DDSCAPS_MIPMAP				= 0x00400000,
		DDSCAPS_TEXTURE				= 0x00001000
	};

	struct ddsPixelFormat
	{
		std::uint32_t size; // 32
		dx::DDPF flags;
		dx::D3DFORMAT fourCC;
		std::uint32_t bpp;
		glm::u32vec4 Mask;
	};

	struct ddsHeader
	{
		char Magic[4];
		std::uint32_t Size;
		std::uint32_t Flags;
		std::uint32_t Height;
		std::uint32_t Width;
		std::uint32_t Pitch;
		std::uint32_t Depth;
		std::uint32_t MipMapLevels;
		std::uint32_t Reserved1[11];
		ddsPixelFormat Format;
		std::uint32_t SurfaceFlags;
		std::uint32_t CubemapFlags;
		std::uint32_t Reserved2[3];
	};

	static_assert(sizeof(ddsHeader) == 128, "DDS Header size mismatch");

	enum D3D10_RESOURCE_DIMENSION 
	{
		D3D10_RESOURCE_DIMENSION_UNKNOWN     = 0,
		D3D10_RESOURCE_DIMENSION_BUFFER      = 1,
		D3D10_RESOURCE_DIMENSION_TEXTURE1D   = 2,
		D3D10_RESOURCE_DIMENSION_TEXTURE2D   = 3,
		D3D10_RESOURCE_DIMENSION_TEXTURE3D   = 4 
	};

	enum D3D10_RESOURCE_MISC_FLAG
	{
		D3D10_RESOURCE_MISC_GENERATE_MIPS		= 0x01,
		D3D10_RESOURCE_MISC_SHARED				= 0x02,
		D3D10_RESOURCE_MISC_TEXTURECUBE			= 0x04,
		D3D10_RESOURCE_MISC_SHARED_KEYEDMUTEX	= 0x10,
		D3D10_RESOURCE_MISC_GDI_COMPATIBLE		= 0x20,
	};

	enum
	{
		DDS_ALPHA_MODE_UNKNOWN					= 0x0,
		DDS_ALPHA_MODE_STRAIGHT					= 0x1,
		DDS_ALPHA_MODE_PREMULTIPLIED			= 0x2,
		DDS_ALPHA_MODE_OPAQUE					= 0x3,
		DDS_ALPHA_MODE_CUSTOM					= 0x4
	};

	struct ddsHeader10
	{
		ddsHeader10() :
			Format(dx::DXGI_FORMAT_UNKNOWN),
			ResourceDimension(D3D10_RESOURCE_DIMENSION_UNKNOWN),
			MiscFlag(0),
			ArraySize(0),
			Reserved(0)
		{}

		dx::dxgiFormat				Format;
		D3D10_RESOURCE_DIMENSION	ResourceDimension;
		std::uint32_t				MiscFlag; // D3D10_RESOURCE_MISC_GENERATE_MIPS
		std::uint32_t				ArraySize;
		std::uint32_t				Reserved;
	};

	static_assert(sizeof(ddsHeader10) == 20, "DDS DX10 Extended Header size mismatch");

	inline target getTarget(ddsHeader const & Header, ddsHeader10 const & Header10)
	{
		if(Header.CubemapFlags & detail::DDSCAPS2_CUBEMAP)
		{
			if(Header10.ArraySize > 1)
				return TARGET_CUBE_ARRAY;
			else
				return TARGET_CUBE;
		}
		else if(Header10.ArraySize > 1)
		{
			if(Header.Flags & detail::DDSD_HEIGHT)
				return TARGET_2D_ARRAY;
			else
				return TARGET_1D_ARRAY;
		}
		else if(Header10.ResourceDimension == D3D10_RESOURCE_DIMENSION_TEXTURE1D)
			return TARGET_1D;
		else if(Header10.ResourceDimension == D3D10_RESOURCE_DIMENSION_TEXTURE3D || Header.Flags & detail::DDSD_DEPTH)
			return TARGET_3D;
		else
			return TARGET_2D;
	}
}//namespace detail

	inline texture load_dds(char const * Data, std::size_t Size)
	{
		assert(Data && (Size >= sizeof(detail::ddsHeader)));

		detail::ddsHeader const & Header(*reinterpret_cast<detail::ddsHeader const *>(Data));

		if(strncmp(Header.Magic, "DDS ", 4) != 0)
			return texture();

		size_t Offset = sizeof(detail::ddsHeader);

		detail::ddsHeader10 Header10;
		if(Header.Format.flags & dx::DDPF_FOURCC && Header.Format.fourCC == dx::D3DFMT_DX10)
		{
			std::memcpy(&Header10, Data + Offset, sizeof(Header10));
			Offset += sizeof(detail::ddsHeader10);
		}

		dx DX;

		gli::format Format(static_cast<gli::format>(gli::FORMAT_INVALID));
		if((Header.Format.flags & (dx::DDPF_RGB | dx::DDPF_ALPHAPIXELS | dx::DDPF_ALPHA | dx::DDPF_YUV | dx::DDPF_LUMINANCE)) && Format == static_cast<format>(gli::FORMAT_INVALID) && Header.Format.flags != dx::DDPF_FOURCC_ALPHAPIXELS)
		{
			switch(Header.Format.bpp)
			{
				case 8:
				{
					if(glm::all(glm::equal(Header.Format.Mask, DX.translate(FORMAT_L8_UNORM).Mask)))
						Format = FORMAT_L8_UNORM;
					else if(glm::all(glm::equal(Header.Format.Mask, DX.translate(FORMAT_A8_UNORM).Mask)))
						Format = FORMAT_A8_UNORM;
					else if(glm::all(glm::equal(Header.Format.Mask, DX.translate(FORMAT_R8_UNORM).Mask)))
						Format = FORMAT_R8_UNORM;
					else if(glm::all(glm::equal(Header.Format.Mask, DX.translate(FORMAT_RG3B2_UNORM).Mask)))
						Format = FORMAT_RG3B2_UNORM;
					break;
				}
				case 16:
				{
					if(glm::all(glm::equal(Header.Format.Mask, DX.translate(FORMAT_LA8_UNORM).Mask)))
						Format = FORMAT_LA8_UNORM;
					else if(glm::all(glm::equal(Header.Format.Mask, DX.translate(FORMAT_RG8_UNORM).Mask)))
						Format = FORMAT_RG8_UNORM;
					else if(glm::all(glm::equal(Header.Format.Mask, DX.translate(FORMAT_R5G6B5_UNORM).Mask)))
						Format = FORMAT_R5G6B5_UNORM;
					else if(glm::all(glm::equal(Header.Format.Mask, DX.translate(FORMAT_L16_UNORM).Mask)))
						Format = FORMAT_L16_UNORM;
					else if(glm::all(glm::equal(Header.Format.Mask, DX.translate(FORMAT_A16_UNORM).Mask)))
						Format = FORMAT_A16_UNORM;
					else if(glm::all(glm::equal(Header.Format.Mask, DX.translate(FORMAT_R16_UNORM).Mask)))
						Format = FORMAT_R16_UNORM;
					else if(glm::all(glm::equal(Header.Format.Mask, DX.translate(FORMAT_RGB5A1_UNORM).Mask)))
						Format = FORMAT_RGB5A1_UNORM;
					break;
				}
				case 24:
				{
					dx::format const & DXFormat = DX.translate(FORMAT_RGB8_UNORM);
					if(glm::all(glm::equal(Header.Format.Mask, DXFormat.Mask)))
						Format = FORMAT_RGB8_UNORM;
					break;
				}
				case 32:
				{
					if(glm::all(glm::equal(Header.Format.Mask, DX.translate(FORMAT_BGRX8_UNORM).Mask)))
						Format = FORMAT_BGRX8_UNORM;
					else if(glm::all(glm::equal(Header.Format.Mask, DX.translate(FORMAT_BGRA8_UNORM).Mask)))
						Format = FORMAT_BGRA8_UNORM;
					else if(glm::all(glm::equal(Header.Format.Mask, DX.translate(FORMAT_RGB10A2_UNORM).Mask)))
						Format = FORMAT_RGB10A2_UNORM;
					else if(glm::all(glm::equal(Header.Format.Mask, DX.translate(FORMAT_LA16_UNORM).Mask)))
						Format = FORMAT_LA16_UNORM;
					else if(glm::all(glm::equal(Header.Format.Mask, DX.translate(FORMAT_RG16_UNORM).Mask)))
						Format = FORMAT_RG16_UNORM;
					else if(glm::all(glm::equal(Header.Format.Mask, DX.translate(FORMAT_R32_SFLOAT).Mask)))
						Format = FORMAT_R32_SFLOAT;
				}
				break;
			}
		}
		else if((Header.Format.flags & dx::DDPF_FOURCC) && (Header.Format.fourCC != dx::D3DFMT_DX10) && (Format == static_cast<format>(gli::FORMAT_INVALID)))
			Format = DX.find(Header.Format.fourCC);
		else if((Header.Format.fourCC == dx::D3DFMT_DX10) && (Header10.Format != dx::DXGI_FORMAT_UNKNOWN))
			Format = DX.find(Header10.Format);

		assert(Format != static_cast<format>(gli::FORMAT_INVALID));

		size_t const MipMapCount = (Header.Flags & detail::DDSD_MIPMAPCOUNT) ? Header.MipMapLevels : 1;
		size_t FaceCount = 1;
		if(Header.CubemapFlags & detail::DDSCAPS2_CUBEMAP)
			FaceCount = int(glm::bitCount(Header.CubemapFlags & detail::DDSCAPS2_CUBEMAP_ALLFACES));

		size_t DepthCount = 1;
		if(Header.CubemapFlags & detail::DDSCAPS2_VOLUME)
			DepthCount = Header.Depth;

		texture Texture(
			getTarget(Header, Header10), Format,
			texture::dim_type(Header.Width, Header.Height, DepthCount),
			std::max<std::size_t>(Header10.ArraySize, 1), FaceCount, MipMapCount);

		assert(Offset + Texture.size() == Size);

		std::memcpy(Texture.data(), Data + Offset, Texture.size());

		return Texture;
	}

	inline texture load_dds(char const * Filename)
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

		return load_dds(&Data[0], Data.size());
	}

	inline texture load_dds(std::string const & Filename)
	{
		return load_dds(Filename.c_str());
	}
}//namespace gli
