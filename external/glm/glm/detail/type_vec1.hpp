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
/// @file glm/detail/type_vec1.hpp
/// @date 2008-08-25 / 2011-06-15
/// @author Christophe Riccio
///////////////////////////////////////////////////////////////////////////////////

#pragma once

#include "../fwd.hpp"
#include "type_vec.hpp"
#ifdef GLM_SWIZZLE
#	if GLM_HAS_ANONYMOUS_UNION
#		include "_swizzle.hpp"
#	else
#		include "_swizzle_func.hpp"
#	endif
#endif //GLM_SWIZZLE
#include <cstddef>

namespace glm
{
	template <typename T, precision P = defaultp>
	struct tvec1
	{
		// -- Implementation detail --

		typedef tvec1<T, P> type;
		typedef tvec1<bool, P> bool_type;
		typedef T value_type;

#		ifdef GLM_META_PROG_HELPERS
			static GLM_RELAXED_CONSTEXPR length_t components = 1;
			static GLM_RELAXED_CONSTEXPR precision prec = P;
#		endif//GLM_META_PROG_HELPERS

#		ifdef GLM_STATIC_CONST_MEMBERS
		static const type ZERO;
		static const type X;
#		endif
		// -- Data --

#		if GLM_HAS_ANONYMOUS_UNION
			union
			{
				T x;
				T r;
				T s;
/*
#				ifdef GLM_SWIZZLE
					_GLM_SWIZZLE1_2_MEMBERS(T, P, tvec2, x)
					_GLM_SWIZZLE1_2_MEMBERS(T, P, tvec2, r)
					_GLM_SWIZZLE1_2_MEMBERS(T, P, tvec2, s)
					_GLM_SWIZZLE1_3_MEMBERS(T, P, tvec3, x)
					_GLM_SWIZZLE1_3_MEMBERS(T, P, tvec3, r)
					_GLM_SWIZZLE1_3_MEMBERS(T, P, tvec3, s)
					_GLM_SWIZZLE1_4_MEMBERS(T, P, tvec4, x)
					_GLM_SWIZZLE1_4_MEMBERS(T, P, tvec4, r)
					_GLM_SWIZZLE1_4_MEMBERS(T, P, tvec4, s)
#				endif//GLM_SWIZZLE*/
			};
#		else
			union {T x, r, s;};
/*
#			ifdef GLM_SWIZZLE
				GLM_SWIZZLE_GEN_VEC_FROM_VEC1(T, P, tvec2, tvec2, tvec3, tvec4)
#			endif//GLM_SWIZZLE*/
#		endif

		// -- Component accesses --

#		ifdef GLM_FORCE_SIZE_FUNC
			/// Return the count of components of the vector
			typedef size_t size_type;
			GLM_FUNC_DECL GLM_CONSTEXPR size_type size() const;

			GLM_FUNC_DECL T & operator[](size_type i);
			GLM_FUNC_DECL T const & operator[](size_type i) const;
#		else
			/// Return the count of components of the vector
			typedef length_t length_type;
			GLM_FUNC_DECL GLM_CONSTEXPR length_type length() const;

			GLM_FUNC_DECL T & operator[](length_type i);
			GLM_FUNC_DECL T const & operator[](length_type i) const;
#		endif//GLM_FORCE_SIZE_FUNC

		// -- Implicit basic constructors --

		GLM_FUNC_DECL tvec1() GLM_DEFAULT_CTOR;
		GLM_FUNC_DECL tvec1(tvec1<T, P> const & v) GLM_DEFAULT;
		template <precision Q>
		GLM_FUNC_DECL tvec1(tvec1<T, Q> const & v);

		// -- Explicit basic constructors --

		GLM_FUNC_DECL explicit tvec1(ctor);
		GLM_FUNC_DECL explicit tvec1(T const & scalar);

		// -- Conversion vector constructors --

		/// Explicit conversions (From section 5.4.1 Conversion and scalar constructors of GLSL 1.30.08 specification)
		template <typename U, precision Q>
		GLM_FUNC_DECL GLM_EXPLICIT tvec1(tvec2<U, Q> const & v);
		/// Explicit conversions (From section 5.4.1 Conversion and scalar constructors of GLSL 1.30.08 specification)
		template <typename U, precision Q>
		GLM_FUNC_DECL GLM_EXPLICIT tvec1(tvec3<U, Q> const & v);
		/// Explicit conversions (From section 5.4.1 Conversion and scalar constructors of GLSL 1.30.08 specification)
		template <typename U, precision Q>
		GLM_FUNC_DECL GLM_EXPLICIT tvec1(tvec4<U, Q> const & v);

		/// Explicit conversions (From section 5.4.1 Conversion and scalar constructors of GLSL 1.30.08 specification)
		template <typename U, precision Q>
		GLM_FUNC_DECL GLM_EXPLICIT tvec1(tvec1<U, Q> const & v);

		// -- Swizzle constructors --

#		if(GLM_HAS_ANONYMOUS_UNION && defined(GLM_SWIZZLE))
			template <int E0>
			GLM_FUNC_DECL tvec1(detail::_swizzle<1, T, P, tvec1<T, P>, E0, -1,-2,-3> const & that)
			{
				*this = that();
			}
#		endif//(GLM_HAS_ANONYMOUS_UNION && defined(GLM_SWIZZLE))

		// -- Unary arithmetic operators --

		GLM_FUNC_DECL tvec1<T, P> & operator=(tvec1<T, P> const & v) GLM_DEFAULT;

		template <typename U>
		GLM_FUNC_DECL tvec1<T, P> & operator=(tvec1<U, P> const & v);
		template <typename U>
		GLM_FUNC_DECL tvec1<T, P> & operator+=(U const & scalar);
		template <typename U>
		GLM_FUNC_DECL tvec1<T, P> & operator+=(tvec1<U, P> const & v);
		template <typename U>
		GLM_FUNC_DECL tvec1<T, P> & operator-=(U const & scalar);
		template <typename U>
		GLM_FUNC_DECL tvec1<T, P> & operator-=(tvec1<U, P> const & v);
		template <typename U>
		GLM_FUNC_DECL tvec1<T, P> & operator*=(U const & scalar);
		template <typename U>
		GLM_FUNC_DECL tvec1<T, P> & operator*=(tvec1<U, P> const & v);
		template <typename U>
		GLM_FUNC_DECL tvec1<T, P> & operator/=(U const & scalar);
		template <typename U>
		GLM_FUNC_DECL tvec1<T, P> & operator/=(tvec1<U, P> const & v);

		// -- Increment and decrement operators --

		GLM_FUNC_DECL tvec1<T, P> & operator++();
		GLM_FUNC_DECL tvec1<T, P> & operator--();
		GLM_FUNC_DECL tvec1<T, P> operator++(int);
		GLM_FUNC_DECL tvec1<T, P> operator--(int);

		// -- Unary bit operators --

		template <typename U>
		GLM_FUNC_DECL tvec1<T, P> & operator%=(U const & scalar);
		template <typename U>
		GLM_FUNC_DECL tvec1<T, P> & operator%=(tvec1<U, P> const & v);
		template <typename U>
		GLM_FUNC_DECL tvec1<T, P> & operator&=(U const & scalar);
		template <typename U>
		GLM_FUNC_DECL tvec1<T, P> & operator&=(tvec1<U, P> const & v);
		template <typename U>
		GLM_FUNC_DECL tvec1<T, P> & operator|=(U const & scalar);
		template <typename U>
		GLM_FUNC_DECL tvec1<T, P> & operator|=(tvec1<U, P> const & v);
		template <typename U>
		GLM_FUNC_DECL tvec1<T, P> & operator^=(U const & scalar);
		template <typename U>
		GLM_FUNC_DECL tvec1<T, P> & operator^=(tvec1<U, P> const & v);
		template <typename U>
		GLM_FUNC_DECL tvec1<T, P> & operator<<=(U const & scalar);
		template <typename U>
		GLM_FUNC_DECL tvec1<T, P> & operator<<=(tvec1<U, P> const & v);
		template <typename U>
		GLM_FUNC_DECL tvec1<T, P> & operator>>=(U const & scalar);
		template <typename U>
		GLM_FUNC_DECL tvec1<T, P> & operator>>=(tvec1<U, P> const & v);
	};

	// -- Unary operators --

	template <typename T, precision P>
	GLM_FUNC_DECL tvec1<T, P> operator+(tvec1<T, P> const & v);

	template <typename T, precision P>
	GLM_FUNC_DECL tvec1<T, P> operator-(tvec1<T, P> const & v);

	// -- Binary operators --

	template <typename T, precision P>
	GLM_FUNC_DECL tvec1<T, P> operator+(tvec1<T, P> const & v, T const & scalar);

	template <typename T, precision P>
	GLM_FUNC_DECL tvec1<T, P> operator+(T const & scalar, tvec1<T, P> const & v);

	template <typename T, precision P>
	GLM_FUNC_DECL tvec1<T, P> operator+(tvec1<T, P> const & v1, tvec1<T, P> const & v2);

	template <typename T, precision P>
	GLM_FUNC_DECL tvec1<T, P> operator-(tvec1<T, P> const & v, T const & scalar);

	template <typename T, precision P>
	GLM_FUNC_DECL tvec1<T, P> operator-(T const & scalar, tvec1<T, P> const & v);

	template <typename T, precision P>
	GLM_FUNC_DECL tvec1<T, P> operator-	(tvec1<T, P> const & v1, tvec1<T, P> const & v2);

	template <typename T, precision P>
	GLM_FUNC_DECL tvec1<T, P> operator*(tvec1<T, P> const & v, T const & scalar);

	template <typename T, precision P>
	GLM_FUNC_DECL tvec1<T, P> operator*(T const & scalar, tvec1<T, P> const & v);

	template <typename T, precision P>
	GLM_FUNC_DECL tvec1<T, P> operator*(tvec1<T, P> const & v1, tvec1<T, P> const & v2);

	template <typename T, precision P>
	GLM_FUNC_DECL tvec1<T, P> operator/(tvec1<T, P> const & v, T const & scalar);

	template <typename T, precision P>
	GLM_FUNC_DECL tvec1<T, P> operator/(T const & scalar, tvec1<T, P> const & v);

	template <typename T, precision P>
	GLM_FUNC_DECL tvec1<T, P> operator/(tvec1<T, P> const & v1, tvec1<T, P> const & v2);

	template <typename T, precision P>
	GLM_FUNC_DECL tvec1<T, P> operator%(tvec1<T, P> const & v, T const & scalar);

	template <typename T, precision P>
	GLM_FUNC_DECL tvec1<T, P> operator%(T const & scalar, tvec1<T, P> const & v);

	template <typename T, precision P>
	GLM_FUNC_DECL tvec1<T, P> operator%(tvec1<T, P> const & v1, tvec1<T, P> const & v2);

	template <typename T, precision P>
	GLM_FUNC_DECL tvec1<T, P> operator&(tvec1<T, P> const & v, T const & scalar);

	template <typename T, precision P>
	GLM_FUNC_DECL tvec1<T, P> operator&(T const & scalar, tvec1<T, P> const & v);

	template <typename T, precision P>
	GLM_FUNC_DECL tvec1<T, P> operator&(tvec1<T, P> const & v1, tvec1<T, P> const & v2);

	template <typename T, precision P>
	GLM_FUNC_DECL tvec1<T, P> operator|(tvec1<T, P> const & v, T const & scalar);

	template <typename T, precision P>
	GLM_FUNC_DECL tvec1<T, P> operator|(T const & scalar, tvec1<T, P> const & v);

	template <typename T, precision P>
	GLM_FUNC_DECL tvec1<T, P> operator|(tvec1<T, P> const & v1, tvec1<T, P> const & v2);

	template <typename T, precision P>
	GLM_FUNC_DECL tvec1<T, P> operator^(tvec1<T, P> const & v, T const & scalar);

	template <typename T, precision P>
	GLM_FUNC_DECL tvec1<T, P> operator^(T const & scalar, tvec1<T, P> const & v);

	template <typename T, precision P>
	GLM_FUNC_DECL tvec1<T, P> operator^(tvec1<T, P> const & v1, tvec1<T, P> const & v2);

	template <typename T, precision P>
	GLM_FUNC_DECL tvec1<T, P> operator<<(tvec1<T, P> const & v, T const & scalar);

	template <typename T, precision P>
	GLM_FUNC_DECL tvec1<T, P> operator<<(T const & scalar, tvec1<T, P> const & v);

	template <typename T, precision P>
	GLM_FUNC_DECL tvec1<T, P> operator<<(tvec1<T, P> const & v1, tvec1<T, P> const & v2);

	template <typename T, precision P>
	GLM_FUNC_DECL tvec1<T, P> operator>>(tvec1<T, P> const & v, T const & scalar);

	template <typename T, precision P>
	GLM_FUNC_DECL tvec1<T, P> operator>>(T const & scalar, tvec1<T, P> const & v);

	template <typename T, precision P>
	GLM_FUNC_DECL tvec1<T, P> operator>>(tvec1<T, P> const & v1, tvec1<T, P> const & v2);

	template <typename T, precision P>
	GLM_FUNC_DECL tvec1<T, P> operator~(tvec1<T, P> const & v);

	// -- Boolean operators --

	template <typename T, precision P>
	GLM_FUNC_DECL bool operator==(tvec1<T, P> const & v1, tvec1<T, P> const & v2);

	template <typename T, precision P>
	GLM_FUNC_DECL bool operator!=(tvec1<T, P> const & v1, tvec1<T, P> const & v2);

	template <precision P>
	GLM_FUNC_DECL tvec1<bool, P> operator&&(tvec1<bool, P> const & v1, tvec1<bool, P> const & v2);

	template <precision P>
	GLM_FUNC_DECL tvec1<bool, P> operator||(tvec1<bool, P> const & v1, tvec1<bool, P> const & v2);

	// -- Is type --

	template <typename T, precision P>
	struct type<T, P, tvec1>
	{
		static bool const is_vec = true;
		static bool const is_mat = false;
		static bool const is_quat = false;
	};
}//namespace glm

#ifndef GLM_EXTERNAL_TEMPLATE
#include "type_vec1.inl"
#endif//GLM_EXTERNAL_TEMPLATE
