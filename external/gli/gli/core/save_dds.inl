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
/// @file gli/core/save_dds.inl
/// @date 2013-01-28 / 2013-01-28
/// @author Christophe Riccio
///////////////////////////////////////////////////////////////////////////////////

#include <cstdio>

namespace gli{
namespace detail
{
	inline D3D10_RESOURCE_DIMENSION getDimension(gli::target const & Target)
	{
		static D3D10_RESOURCE_DIMENSION Table[] = //TARGET_COUNT
		{
			D3D10_RESOURCE_DIMENSION_TEXTURE1D,		//TARGET_1D,
			D3D10_RESOURCE_DIMENSION_TEXTURE1D,		//TARGET_1D_ARRAY,
			D3D10_RESOURCE_DIMENSION_TEXTURE2D,		//TARGET_2D,
			D3D10_RESOURCE_DIMENSION_TEXTURE2D,		//TARGET_2D_ARRAY,
			D3D10_RESOURCE_DIMENSION_TEXTURE3D,		//TARGET_3D,
			D3D10_RESOURCE_DIMENSION_TEXTURE2D,		//TARGET_CUBE,
			D3D10_RESOURCE_DIMENSION_TEXTURE2D		//TARGET_CUBE_ARRAY
		};
		static_assert(sizeof(Table) / sizeof(Table[0]) == TARGET_COUNT, "Table needs to be updated");

		return Table[Target];
	}
}

	inline bool save_dds(texture const & Texture, std::vector<char> & Memory)
	{
		if(Texture.empty())
			return false;

		dx DX;
		dx::format const & DXFormat = DX.translate(Texture.format());

		dx::D3DFORMAT const FourCC = Texture.layers() > 1 ? dx::D3DFMT_DX10 : DXFormat.D3DFormat;

		Memory.resize(Texture.size() + sizeof(detail::ddsHeader) + (FourCC == dx::D3DFMT_DX10 ? sizeof(detail::ddsHeader10) : 0));

		detail::ddsHeader & Header = *reinterpret_cast<detail::ddsHeader*>(&Memory[0]);

		detail::formatInfo const & Desc = detail::get_format_info(Texture.format());

		std::uint32_t Caps = detail::DDSD_CAPS | detail::DDSD_WIDTH | detail::DDSD_PIXELFORMAT | detail::DDSD_MIPMAPCOUNT;
		Caps |= !is_target_1d(Texture.target()) ? detail::DDSD_HEIGHT : 0;
		Caps |= Texture.target() == TARGET_3D ? detail::DDSD_DEPTH : 0;
		//Caps |= Storage.levels() > 1 ? detail::DDSD_MIPMAPCOUNT : 0;
		Caps |= (Desc.Flags & detail::CAP_COMPRESSED_BIT) ? detail::DDSD_LINEARSIZE : detail::DDSD_PITCH;

		bool const RequireFOURCCDX10 = is_target_array(Texture.target()) || is_target_1d(Texture.target());

		memcpy(Header.Magic, "DDS ", sizeof(Header.Magic));
		memset(Header.Reserved1, 0, sizeof(Header.Reserved1));
		memset(Header.Reserved2, 0, sizeof(Header.Reserved2));
		Header.Size = sizeof(detail::ddsHeader) - sizeof(Header.Magic);
		Header.Flags = Caps;
		assert(Texture.dimensions().x < std::numeric_limits<glm::uint32>::max());
		Header.Width = static_cast<std::uint32_t>(Texture.dimensions().x);
		assert(Texture.dimensions().y < std::numeric_limits<glm::uint32>::max());
		Header.Height = static_cast<std::uint32_t>(Texture.dimensions().y);
		Header.Pitch = static_cast<std::uint32_t>((Desc.Flags & detail::CAP_COMPRESSED_BIT) ? Texture.size() / Texture.faces() : 32);
		assert(Texture.dimensions().z < std::numeric_limits<glm::uint32>::max());
		Header.Depth = static_cast<std::uint32_t>(Texture.dimensions().z > 1 ? Texture.dimensions().z : 0);
		Header.MipMapLevels = static_cast<std::uint32_t>(Texture.levels());
		Header.Format.size = sizeof(detail::ddsPixelFormat);
		Header.Format.flags = RequireFOURCCDX10 ? dx::DDPF_FOURCC : DXFormat.DDPixelFormat;
		Header.Format.fourCC = RequireFOURCCDX10 ? dx::D3DFMT_DX10 : DXFormat.D3DFormat;
		Header.Format.bpp = static_cast<std::uint32_t>(detail::bits_per_pixel(Texture.format()));
		Header.Format.Mask = DXFormat.Mask;
		//Header.surfaceFlags = detail::DDSCAPS_TEXTURE | (Storage.levels() > 1 ? detail::DDSCAPS_MIPMAP : 0);
		Header.SurfaceFlags = detail::DDSCAPS_TEXTURE | detail::DDSCAPS_MIPMAP;
		Header.CubemapFlags = 0;

		// Cubemap
		if(Texture.faces() > 1)
		{
			assert(Texture.faces() == 6);
			Header.CubemapFlags |= detail::DDSCAPS2_CUBEMAP_ALLFACES | detail::DDSCAPS2_CUBEMAP;
		}

		// Texture3D
		if(Texture.dimensions().z > 1)
			Header.CubemapFlags |= detail::DDSCAPS2_VOLUME;

		size_t Offset = sizeof(detail::ddsHeader);
		if(Header.Format.fourCC == dx::D3DFMT_DX10)
		{
			detail::ddsHeader10 & Header10 = *reinterpret_cast<detail::ddsHeader10*>(&Memory[0] + sizeof(detail::ddsHeader));
			Offset += sizeof(detail::ddsHeader10);

			Header10.ArraySize = static_cast<std::uint32_t>(Texture.layers());
			Header10.ResourceDimension = detail::getDimension(Texture.target());
			Header10.MiscFlag = 0;//Storage.levels() > 0 ? detail::D3D10_RESOURCE_MISC_GENERATE_MIPS : 0;
			Header10.Format = static_cast<dx::dxgiFormat>(DXFormat.DXGIFormat);
			Header10.Reserved = 0;
		}

		std::memcpy(&Memory[0] + Offset, Texture.data(), Texture.size());

		return true;
	}

	inline bool save_dds(texture const & Texture, char const * Filename)
	{
		if(Texture.empty())
			return false;

		FILE* File = std::fopen(Filename, "wb");
		if(!File)
			return false;

		std::vector<char> Memory;
		bool const Result = save_dds(Texture, Memory);

		std::fwrite(&Memory[0], 1, Memory.size(), File);
		std::fclose(File);

		return Result;
	}

	inline bool save_dds(texture const & Texture, std::string const & Filename)
	{
		return save_dds(Texture, Filename.c_str());
	}
}//namespace gli
