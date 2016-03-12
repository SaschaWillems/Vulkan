///////////////////////////////////////////////////////////////////////////////////
/// OpenGL Mathematics (glm.g-truc.net)
///
/// Copyright (c) 2005 - 2015 G-Truc Creation (www.g-truc.net)
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
/// Restrictions:
///		By making use of the Software for military purposes, you choose to make
///		a Bunny unhappy.
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
/// @file glm/detail/func_packing.inl
/// @date 2010-03-17 / 2011-06-15
/// @author Christophe Riccio
///////////////////////////////////////////////////////////////////////////////////

#include "func_common.hpp"
#include "type_half.hpp"
#include "../fwd.hpp"

namespace glm
{
	GLM_FUNC_QUALIFIER uint packUnorm2x16(vec2 const & v)
	{
		union
		{
			u16  in[2];
			uint out;
		} u;

		u16vec2 result(round(clamp(v, 0.0f, 1.0f) * 65535.0f));

		u.in[0] = result[0];
		u.in[1] = result[1];

		return u.out;
	}

	GLM_FUNC_QUALIFIER vec2 unpackUnorm2x16(uint p)
	{
		union
		{
			uint in;
			u16  out[2];
		} u;

		u.in = p;

		return vec2(u.out[0], u.out[1]) * 1.5259021896696421759365224689097e-5f;
	}

	GLM_FUNC_QUALIFIER uint packSnorm2x16(vec2 const & v)
	{
		union
		{
			i16  in[2];
			uint out;
		} u;

		i16vec2 result(round(clamp(v, -1.0f, 1.0f) * 32767.0f));

		u.in[0] = result[0];
		u.in[1] = result[1];

		return u.out;
	}

	GLM_FUNC_QUALIFIER vec2 unpackSnorm2x16(uint p)
	{
		union
		{
			uint in;
			i16  out[2];
		} u;

		u.in = p;

		return clamp(vec2(u.out[0], u.out[1]) * 3.0518509475997192297128208258309e-5f, -1.0f, 1.0f);
	}

	GLM_FUNC_QUALIFIER uint packUnorm4x8(vec4 const & v)
	{
		union
		{
			u8   in[4];
			uint out;
		} u;

		u8vec4 result(round(clamp(v, 0.0f, 1.0f) * 255.0f));

		u.in[0] = result[0];
		u.in[1] = result[1];
		u.in[2] = result[2];
		u.in[3] = result[3];

		return u.out;
	}

	GLM_FUNC_QUALIFIER vec4 unpackUnorm4x8(uint p)
	{
		union
		{
			uint in;
			u8   out[4];
		} u;

		u.in = p;

		return vec4(u.out[0], u.out[1], u.out[2], u.out[3]) * 0.0039215686274509803921568627451f;
	}
	
	GLM_FUNC_QUALIFIER uint packSnorm4x8(vec4 const & v)
	{
		union
		{
			i8   in[4];
			uint out;
		} u;

		i8vec4 result(round(clamp(v, -1.0f, 1.0f) * 127.0f));

		u.in[0] = result[0];
		u.in[1] = result[1];
		u.in[2] = result[2];
		u.in[3] = result[3];

		return u.out;
	}
	
	GLM_FUNC_QUALIFIER glm::vec4 unpackSnorm4x8(uint p)
	{
		union
		{
			uint in;
			i8   out[4];
		} u;

		u.in = p;

		return clamp(vec4(u.out[0], u.out[1], u.out[2], u.out[3]) * 0.0078740157480315f, -1.0f, 1.0f);
	}

	GLM_FUNC_QUALIFIER double packDouble2x32(uvec2 const & v)
	{
		union
		{
			uint   in[2];
			double out;
		} u;

		u.in[0] = v[0];
		u.in[1] = v[1];

		return u.out;
	}

	GLM_FUNC_QUALIFIER uvec2 unpackDouble2x32(double v)
	{
		union
		{
			double in;
			uint   out[2];
		} u;

		u.in = v;

		return uvec2(u.out[0], u.out[1]);
	}

	GLM_FUNC_QUALIFIER uint packHalf2x16(vec2 const & v)
	{
		union
		{
			i16  in[2];
			uint out;
		} u;

		u.in[0] = detail::toFloat16(v.x);
		u.in[1] = detail::toFloat16(v.y);

		return u.out;
	}

	GLM_FUNC_QUALIFIER vec2 unpackHalf2x16(uint v)
	{
		union
		{
			uint in;
			i16  out[2];
		} u;

		u.in = v;

		return vec2(
			detail::toFloat32(u.out[0]),
			detail::toFloat32(u.out[1]));
	}
}//namespace glm

