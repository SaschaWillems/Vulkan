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
/// @file gli/save_dds.hpp
/// @date 2013-01-28 / 2013-01-28
/// @author Christophe Riccio
///////////////////////////////////////////////////////////////////////////////////

#pragma once

#include "load_dds.hpp"

namespace gli
{
	/// Save a texture storage to a DDS file.
	/// 
	/// @param Path Path for where to save the file. It must include the filaname and filename extension.
	/// This function ignores the filename extension in the path and save to DDS anyway but keep the requested filename extension.
	/// @return Returns false if the function fails to save the file.
	bool save_dds(texture const & Texture, char const * Filename);

	/// Save a texture storage to a DDS file.
	/// 
	/// @param Path Path for where to save the file. It must include the filaname and filename extension.
	/// This function ignores the filename extension in the path and save to DDS anyway but keep the requested filename extension.
	/// @return Returns false if the function fails to save the file.
	bool save_dds(texture const & Texture, std::string const & Filename);

	/// Save a texture storage to a DDS file.
	/// 
	/// @param Memory Storage for the DDS container. The function resizes the containers to fit the necessary storage.
	/// @return Returns false if the function fails to save the file.
	bool save_dds(texture const & Texture, std::vector<char> & Memory);
}//namespace gli

#include "./core/save_dds.inl"
