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
/// @file gli/load_dds.hpp
/// @date 2010-09-08 / 2013-01-28
/// @author Christophe Riccio
///////////////////////////////////////////////////////////////////////////////////

#pragma once

#include "texture.hpp"

namespace gli
{
	/// Loads a texture storage from DDS file. Returns an empty storage in case of failure.
	///
	/// @param Path Path of the file to open including filaname and filename extension
	texture load_dds(char const * Path);

	/// Loads a texture storage from DDS file. Returns an empty storage in case of failure.
	///
	/// @param Path Path of the file to open including filaname and filename extension
	texture load_dds(std::string const & Filename);

	/// Loads a texture storage from DDS memory. Returns an empty storage in case of failure.
	///
	/// @param Path Path of the file to open including filaname and filename extension
	texture load_dds(char const * Data, std::size_t Size);
}//namespace gli

#include "./core/load_dds.inl"
