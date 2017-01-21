/// @ref core
/// @file glm/detail/type_vec2.hpp

#pragma once

#include "type_vec.hpp"
#if GLM_SWIZZLE == GLM_SWIZZLE_ENABLED
#	if GLM_HAS_UNRESTRICTED_UNIONS
#		include "_swizzle.hpp"
#	else
#		include "_swizzle_func.hpp"
#	endif
#endif //GLM_SWIZZLE
#include <cstddef>

namespace glm
{
	template<typename T, precision P>
	struct vec<2, T, P>
	{
		// -- Implementation detail --

		typedef T value_type;
		typedef vec type;
		typedef vec<2, bool, P> bool_type;

		// -- Data --

#		if GLM_HAS_ONLY_XYZW
			T x, y;

#		elif GLM_HAS_ALIGNED_TYPE
#			if GLM_COMPILER & GLM_COMPILER_GCC
#				pragma GCC diagnostic push
#				pragma GCC diagnostic ignored "-Wpedantic"
#			endif
#			if GLM_COMPILER & GLM_COMPILER_CLANG
#				pragma clang diagnostic push
#				pragma clang diagnostic ignored "-Wgnu-anonymous-struct"
#				pragma clang diagnostic ignored "-Wnested-anon-types"
#			endif
		
			union
			{
				struct{ T x, y; };
				struct{ T r, g; };
				struct{ T s, t; };

#				if GLM_SWIZZLE == GLM_SWIZZLE_ENABLED
					_GLM_SWIZZLE2_2_MEMBERS(T, P, x, y)
					_GLM_SWIZZLE2_2_MEMBERS(T, P, r, g)
					_GLM_SWIZZLE2_2_MEMBERS(T, P, s, t)
					_GLM_SWIZZLE2_3_MEMBERS(T, P, x, y)
					_GLM_SWIZZLE2_3_MEMBERS(T, P, r, g)
					_GLM_SWIZZLE2_3_MEMBERS(T, P, s, t)
					_GLM_SWIZZLE2_4_MEMBERS(T, P, x, y)
					_GLM_SWIZZLE2_4_MEMBERS(T, P, r, g)
					_GLM_SWIZZLE2_4_MEMBERS(T, P, s, t)
#				endif//GLM_SWIZZLE

			};
		
#			if GLM_COMPILER & GLM_COMPILER_CLANG
#				pragma clang diagnostic pop
#			endif
#			if GLM_COMPILER & GLM_COMPILER_GCC
#				pragma GCC diagnostic pop
#			endif
#		else
			union {T x, r, s;};
			union {T y, g, t;};

#			if GLM_SWIZZLE == GLM_SWIZZLE_ENABLED
				GLM_SWIZZLE_GEN_VEC_FROM_VEC2(T, P)
#			endif//GLM_SWIZZLE
#		endif

		// -- Component accesses --

		/// Return the count of components of the vector
		typedef length_t length_type;
		GLM_FUNC_DECL static length_type length(){return 2;}

		GLM_FUNC_DECL T& operator[](length_type i);
		GLM_FUNC_DECL T const& operator[](length_type i) const;

		// -- Implicit basic constructors --

		GLM_FUNC_DECL GLM_CONSTEXPR_CTOR vec() GLM_DEFAULT_CTOR;
		GLM_FUNC_DECL GLM_CONSTEXPR_CTOR vec(vec const& v) GLM_DEFAULT;
		template<precision Q>
		GLM_FUNC_DECL GLM_CONSTEXPR_CTOR vec(vec<2, T, Q> const& v);

		// -- Explicit basic constructors --

		GLM_FUNC_DECL GLM_CONSTEXPR_CTOR explicit vec(ctor);
		GLM_FUNC_DECL GLM_CONSTEXPR_CTOR explicit vec(T scalar);
		GLM_FUNC_DECL GLM_CONSTEXPR_CTOR vec(T x, T y);

		// -- Conversion constructors --

		/// Explicit conversions (From section 5.4.1 Conversion and scalar constructors of GLSL 1.30.08 specification)
		template<typename A, typename B>
		GLM_FUNC_DECL GLM_CONSTEXPR_CTOR vec(A x, B y);
		template<typename A, typename B>
		GLM_FUNC_DECL GLM_CONSTEXPR_CTOR vec(vec<1, A, P> const& x, vec<1, B, P> const& y);

		// -- Conversion vector constructors --

		/// Explicit conversions (From section 5.4.1 Conversion and scalar constructors of GLSL 1.30.08 specification)
		template<typename U, precision Q>
		GLM_FUNC_DECL GLM_CONSTEXPR_CTOR GLM_EXPLICIT vec(vec<3, U, Q> const& v);
		/// Explicit conversions (From section 5.4.1 Conversion and scalar constructors of GLSL 1.30.08 specification)
		template<typename U, precision Q>
		GLM_FUNC_DECL GLM_CONSTEXPR_CTOR GLM_EXPLICIT vec(vec<4, U, Q> const& v);

		/// Explicit conversions (From section 5.4.1 Conversion and scalar constructors of GLSL 1.30.08 specification)
		template<typename U, precision Q>
		GLM_FUNC_DECL GLM_CONSTEXPR_CTOR GLM_EXPLICIT vec(vec<2, U, Q> const& v);

		// -- Swizzle constructors --
#		if GLM_HAS_UNRESTRICTED_UNIONS && (GLM_SWIZZLE == GLM_SWIZZLE_ENABLED)
			template<int E0, int E1>
			GLM_FUNC_DECL vec(detail::_swizzle<2, T, P, E0, E1,-1,-2> const& that)
			{
				*this = that();
			}
#		endif// GLM_HAS_UNRESTRICTED_UNIONS && (GLM_SWIZZLE == GLM_SWIZZLE_ENABLED)

		// -- Unary arithmetic operators --

		GLM_FUNC_DECL vec& operator=(vec const & v) GLM_DEFAULT;

		template<typename U>
		GLM_FUNC_DECL vec& operator=(vec<2, U, P> const& v);
		template<typename U>
		GLM_FUNC_DECL vec& operator+=(U scalar);
		template<typename U>
		GLM_FUNC_DECL vec& operator+=(vec<1, U, P> const& v);
		template<typename U>
		GLM_FUNC_DECL vec& operator+=(vec<2, U, P> const& v);
		template<typename U>
		GLM_FUNC_DECL vec& operator-=(U scalar);
		template<typename U>
		GLM_FUNC_DECL vec& operator-=(vec<1, U, P> const& v);
		template<typename U>
		GLM_FUNC_DECL vec& operator-=(vec<2, U, P> const& v);
		template<typename U>
		GLM_FUNC_DECL vec& operator*=(U scalar);
		template<typename U>
		GLM_FUNC_DECL vec& operator*=(vec<1, U, P> const& v);
		template<typename U>
		GLM_FUNC_DECL vec& operator*=(vec<2, U, P> const& v);
		template<typename U>
		GLM_FUNC_DECL vec& operator/=(U scalar);
		template<typename U>
		GLM_FUNC_DECL vec& operator/=(vec<1, U, P> const& v);
		template<typename U>
		GLM_FUNC_DECL vec& operator/=(vec<2, U, P> const& v);

		// -- Increment and decrement operators --

		GLM_FUNC_DECL vec & operator++();
		GLM_FUNC_DECL vec & operator--();
		GLM_FUNC_DECL vec operator++(int);
		GLM_FUNC_DECL vec operator--(int);

		// -- Unary bit operators --

		template<typename U> 
		GLM_FUNC_DECL vec & operator%=(U scalar);
		template<typename U> 
		GLM_FUNC_DECL vec & operator%=(vec<1, U, P> const & v);
		template<typename U> 
		GLM_FUNC_DECL vec & operator%=(vec<2, U, P> const & v);
		template<typename U> 
		GLM_FUNC_DECL vec & operator&=(U scalar);
		template<typename U> 
		GLM_FUNC_DECL vec & operator&=(vec<1, U, P> const & v);
		template<typename U> 
		GLM_FUNC_DECL vec & operator&=(vec<2, U, P> const & v);
		template<typename U> 
		GLM_FUNC_DECL vec & operator|=(U scalar);
		template<typename U> 
		GLM_FUNC_DECL vec & operator|=(vec<1, U, P> const & v);
		template<typename U> 
		GLM_FUNC_DECL vec & operator|=(vec<2, U, P> const & v);
		template<typename U> 
		GLM_FUNC_DECL vec & operator^=(U scalar);
		template<typename U> 
		GLM_FUNC_DECL vec & operator^=(vec<1, U, P> const & v);
		template<typename U> 
		GLM_FUNC_DECL vec & operator^=(vec<2, U, P> const & v);
		template<typename U> 
		GLM_FUNC_DECL vec & operator<<=(U scalar);
		template<typename U> 
		GLM_FUNC_DECL vec & operator<<=(vec<1, U, P> const & v);
		template<typename U> 
		GLM_FUNC_DECL vec & operator<<=(vec<2, U, P> const & v);
		template<typename U> 
		GLM_FUNC_DECL vec & operator>>=(U scalar);
		template<typename U> 
		GLM_FUNC_DECL vec & operator>>=(vec<1, U, P> const & v);
		template<typename U> 
		GLM_FUNC_DECL vec & operator>>=(vec<2, U, P> const & v);
	};

	// -- Unary operators --

	template<typename T, precision P>
	GLM_FUNC_DECL vec<2, T, P> operator+(vec<2, T, P> const & v);

	template<typename T, precision P>
	GLM_FUNC_DECL vec<2, T, P> operator-(vec<2, T, P> const & v);

	// -- Binary operators --

	template<typename T, precision P>
	GLM_FUNC_DECL vec<2, T, P> operator+(vec<2, T, P> const & v, T scalar);

	template<typename T, precision P>
	GLM_FUNC_DECL vec<2, T, P> operator+(vec<2, T, P> const & v1, vec<1, T, P> const & v2);

	template<typename T, precision P>
	GLM_FUNC_DECL vec<2, T, P> operator+(T scalar, vec<2, T, P> const & v);

	template<typename T, precision P>
	GLM_FUNC_DECL vec<2, T, P> operator+(vec<1, T, P> const & v1, vec<2, T, P> const & v2);

	template<typename T, precision P>
	GLM_FUNC_DECL vec<2, T, P> operator+(vec<2, T, P> const & v1, vec<2, T, P> const & v2);

	template<typename T, precision P>
	GLM_FUNC_DECL vec<2, T, P> operator-(vec<2, T, P> const & v, T scalar);

	template<typename T, precision P>
	GLM_FUNC_DECL vec<2, T, P> operator-(vec<2, T, P> const & v1, vec<1, T, P> const & v2);

	template<typename T, precision P>
	GLM_FUNC_DECL vec<2, T, P> operator-(T scalar, vec<2, T, P> const & v);

	template<typename T, precision P>
	GLM_FUNC_DECL vec<2, T, P> operator-(vec<1, T, P> const & v1, vec<2, T, P> const & v2);

	template<typename T, precision P>
	GLM_FUNC_DECL vec<2, T, P> operator-(vec<2, T, P> const & v1, vec<2, T, P> const & v2);

	template<typename T, precision P>
	GLM_FUNC_DECL vec<2, T, P> operator*(vec<2, T, P> const & v, T scalar);

	template<typename T, precision P>
	GLM_FUNC_DECL vec<2, T, P> operator*(vec<2, T, P> const & v1, vec<1, T, P> const & v2);

	template<typename T, precision P>
	GLM_FUNC_DECL vec<2, T, P> operator*(T scalar, vec<2, T, P> const & v);

	template<typename T, precision P>
	GLM_FUNC_DECL vec<2, T, P> operator*(vec<1, T, P> const & v1, vec<2, T, P> const & v2);

	template<typename T, precision P>
	GLM_FUNC_DECL vec<2, T, P> operator*(vec<2, T, P> const & v1, vec<2, T, P> const & v2);

	template<typename T, precision P>
	GLM_FUNC_DECL vec<2, T, P> operator/(vec<2, T, P> const & v, T scalar);

	template<typename T, precision P>
	GLM_FUNC_DECL vec<2, T, P> operator/(vec<2, T, P> const & v1, vec<1, T, P> const & v2);

	template<typename T, precision P>
	GLM_FUNC_DECL vec<2, T, P> operator/(T scalar, vec<2, T, P> const & v);

	template<typename T, precision P>
	GLM_FUNC_DECL vec<2, T, P> operator/(vec<1, T, P> const & v1, vec<2, T, P> const & v2);

	template<typename T, precision P>
	GLM_FUNC_DECL vec<2, T, P> operator/(vec<2, T, P> const & v1, vec<2, T, P> const & v2);

	template<typename T, precision P>
	GLM_FUNC_DECL vec<2, T, P> operator%(vec<2, T, P> const & v, T scalar);

	template<typename T, precision P>
	GLM_FUNC_DECL vec<2, T, P> operator%(vec<2, T, P> const & v1, vec<1, T, P> const & v2);

	template<typename T, precision P>
	GLM_FUNC_DECL vec<2, T, P> operator%(T scalar, vec<2, T, P> const & v);

	template<typename T, precision P>
	GLM_FUNC_DECL vec<2, T, P> operator%(vec<1, T, P> const & v1, vec<2, T, P> const & v2);

	template<typename T, precision P>
	GLM_FUNC_DECL vec<2, T, P> operator%(vec<2, T, P> const & v1, vec<2, T, P> const & v2);

	template<typename T, precision P>
	GLM_FUNC_DECL vec<2, T, P> operator&(vec<2, T, P> const & v, T scalar);

	template<typename T, precision P>
	GLM_FUNC_DECL vec<2, T, P> operator&(vec<2, T, P> const & v1, vec<1, T, P> const & v2);

	template<typename T, precision P>
	GLM_FUNC_DECL vec<2, T, P> operator&(T scalar, vec<2, T, P> const & v);

	template<typename T, precision P>
	GLM_FUNC_DECL vec<2, T, P> operator&(vec<1, T, P> const & v1, vec<2, T, P> const & v2);

	template<typename T, precision P>
	GLM_FUNC_DECL vec<2, T, P> operator&(vec<2, T, P> const & v1, vec<2, T, P> const & v2);

	template<typename T, precision P>
	GLM_FUNC_DECL vec<2, T, P> operator|(vec<2, T, P> const & v, T scalar);

	template<typename T, precision P>
	GLM_FUNC_DECL vec<2, T, P> operator|(vec<2, T, P> const & v1, vec<1, T, P> const & v2);

	template<typename T, precision P>
	GLM_FUNC_DECL vec<2, T, P> operator|(T scalar, vec<2, T, P> const & v);

	template<typename T, precision P>
	GLM_FUNC_DECL vec<2, T, P> operator|(vec<1, T, P> const & v1, vec<2, T, P> const & v2);

	template<typename T, precision P>
	GLM_FUNC_DECL vec<2, T, P> operator|(vec<2, T, P> const & v1, vec<2, T, P> const & v2);

	template<typename T, precision P>
	GLM_FUNC_DECL vec<2, T, P> operator^(vec<2, T, P> const & v, T scalar);

	template<typename T, precision P>
	GLM_FUNC_DECL vec<2, T, P> operator^(vec<2, T, P> const & v1, vec<1, T, P> const & v2);

	template<typename T, precision P>
	GLM_FUNC_DECL vec<2, T, P> operator^(T scalar, vec<2, T, P> const & v);

	template<typename T, precision P>
	GLM_FUNC_DECL vec<2, T, P> operator^(vec<1, T, P> const & v1, vec<2, T, P> const & v2);

	template<typename T, precision P>
	GLM_FUNC_DECL vec<2, T, P> operator^(vec<2, T, P> const & v1, vec<2, T, P> const & v2);

	template<typename T, precision P>
	GLM_FUNC_DECL vec<2, T, P> operator<<(vec<2, T, P> const & v, T scalar);

	template<typename T, precision P>
	GLM_FUNC_DECL vec<2, T, P> operator<<(vec<2, T, P> const & v1, vec<1, T, P> const & v2);

	template<typename T, precision P>
	GLM_FUNC_DECL vec<2, T, P> operator<<(T scalar, vec<2, T, P> const & v);

	template<typename T, precision P>
	GLM_FUNC_DECL vec<2, T, P> operator<<(vec<1, T, P> const & v1, vec<2, T, P> const & v2);

	template<typename T, precision P>
	GLM_FUNC_DECL vec<2, T, P> operator<<(vec<2, T, P> const & v1, vec<2, T, P> const & v2);

	template<typename T, precision P>
	GLM_FUNC_DECL vec<2, T, P> operator>>(vec<2, T, P> const & v, T scalar);

	template<typename T, precision P>
	GLM_FUNC_DECL vec<2, T, P> operator>>(vec<2, T, P> const & v1, vec<1, T, P> const & v2);

	template<typename T, precision P>
	GLM_FUNC_DECL vec<2, T, P> operator>>(T scalar, vec<2, T, P> const & v);

	template<typename T, precision P>
	GLM_FUNC_DECL vec<2, T, P> operator>>(vec<1, T, P> const & v1, vec<2, T, P> const & v2);

	template<typename T, precision P>
	GLM_FUNC_DECL vec<2, T, P> operator>>(vec<2, T, P> const & v1, vec<2, T, P> const & v2);

	template<typename T, precision P>
	GLM_FUNC_DECL vec<2, T, P> operator~(vec<2, T, P> const & v);

	// -- Boolean operators --

	template<typename T, precision P>
	GLM_FUNC_DECL bool operator==(vec<2, T, P> const & v1, vec<2, T, P> const & v2);

	template<typename T, precision P>
	GLM_FUNC_DECL bool operator!=(vec<2, T, P> const & v1, vec<2, T, P> const & v2);

	template<precision P>
	GLM_FUNC_DECL vec<2, bool, P> operator&&(vec<2, bool, P> const & v1, vec<2, bool, P> const & v2);

	template<precision P>
	GLM_FUNC_DECL vec<2, bool, P> operator||(vec<2, bool, P> const & v1, vec<2, bool, P> const & v2);
}//namespace glm

#ifndef GLM_EXTERNAL_TEMPLATE
#include "type_vec2.inl"
#endif//GLM_EXTERNAL_TEMPLATE
