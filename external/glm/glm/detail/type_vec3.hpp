/// @ref core
/// @file glm/detail/type_vec3.hpp

#pragma once

#include "type_vec.hpp"
#if GLM_SWIZZLE == GLM_SWIZZLE_ENABLED
#	if GLM_HAS_UNRESTRICTED_UNIONS
#		include "_swizzle.hpp"
#	else
#		include "_swizzle_func.hpp"
#	endif
#endif //GLM_SWIZZLE == GLM_SWIZZLE_ENABLED
#include <cstddef>

namespace glm
{
	template<typename T, precision P>
	struct vec<3, T, P>
	{
		// -- Implementation detail --

		typedef T value_type;
		typedef vec type;
		typedef vec<3, bool, P> bool_type;

		// -- Data --

#		if GLM_HAS_ONLY_XYZW
			T x, y, z;

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
				struct{ T x, y, z; };
				struct{ T r, g, b; };
				struct{ T s, t, p; };

#				if GLM_SWIZZLE == GLM_SWIZZLE_ENABLED
					_GLM_SWIZZLE3_2_MEMBERS(T, P, x, y, z)
					_GLM_SWIZZLE3_2_MEMBERS(T, P, r, g, b)
					_GLM_SWIZZLE3_2_MEMBERS(T, P, s, t, p)
					_GLM_SWIZZLE3_3_MEMBERS(T, P, x, y, z)
					_GLM_SWIZZLE3_3_MEMBERS(T, P, r, g, b)
					_GLM_SWIZZLE3_3_MEMBERS(T, P, s, t, p)
					_GLM_SWIZZLE3_4_MEMBERS(T, P, x, y, z)
					_GLM_SWIZZLE3_4_MEMBERS(T, P, r, g, b)
					_GLM_SWIZZLE3_4_MEMBERS(T, P, s, t, p)
#				endif//GLM_SWIZZLE
			};
		
#			if GLM_COMPILER & GLM_COMPILER_CLANG
#				pragma clang diagnostic pop
#			endif
#			if GLM_COMPILER & GLM_COMPILER_GCC
#				pragma GCC diagnostic pop
#			endif
#		else
			union { T x, r, s; };
			union { T y, g, t; };
			union { T z, b, p; };

#			if GLM_SWIZZLE == GLM_SWIZZLE_ENABLED
				GLM_SWIZZLE_GEN_VEC_FROM_VEC3(T, P)
#			endif//GLM_SWIZZLE
#		endif//GLM_LANG

		// -- Component accesses --

		/// Return the count of components of the vector
		typedef length_t length_type;
		GLM_FUNC_DECL static length_type length(){return 3;}

		GLM_FUNC_DECL T & operator[](length_type i);
		GLM_FUNC_DECL T const & operator[](length_type i) const;

		// -- Implicit basic constructors --

		GLM_FUNC_DECL GLM_CONSTEXPR_CTOR vec() GLM_DEFAULT_CTOR;
		GLM_FUNC_DECL GLM_CONSTEXPR_CTOR vec(vec const & v) GLM_DEFAULT;
		template<precision Q>
		GLM_FUNC_DECL GLM_CONSTEXPR_CTOR vec(vec<3, T, Q> const & v);

		// -- Explicit basic constructors --

		GLM_FUNC_DECL GLM_CONSTEXPR_CTOR explicit vec(ctor);
		GLM_FUNC_DECL GLM_CONSTEXPR_CTOR explicit vec(T scalar);
		GLM_FUNC_DECL GLM_CONSTEXPR_CTOR vec(T a, T b, T c);

		// -- Conversion scalar constructors --

		/// Explicit converions (From section 5.4.1 Conversion and scalar constructors of GLSL 1.30.08 specification)
		template<typename X, typename Y, typename Z>
		GLM_FUNC_DECL GLM_CONSTEXPR_CTOR vec(X x, Y y, Z z);
		template<typename X, typename Y, typename Z>
		GLM_FUNC_DECL GLM_CONSTEXPR_CTOR vec(vec<1, X, P> const& _x, vec<1, Y, P> const& _y, vec<1, Z, P> const& _z);

		// -- Conversion vector constructors --

		/// Explicit conversions (From section 5.4.1 Conversion and scalar constructors of GLSL 1.30.08 specification)
		template<typename A, typename B, precision Q>
		GLM_FUNC_DECL GLM_CONSTEXPR_CTOR vec(vec<2, A, Q> const& _xy, B _z);
		/// Explicit conversions (From section 5.4.1 Conversion and scalar constructors of GLSL 1.30.08 specification)
		template<typename A, typename B, precision Q>
		GLM_FUNC_DECL GLM_CONSTEXPR_CTOR vec(vec<2, A, Q> const& _xy, vec<1, B, Q> const& _z);
		/// Explicit conversions (From section 5.4.1 Conversion and scalar constructors of GLSL 1.30.08 specification)
		template<typename A, typename B, precision Q>
		GLM_FUNC_DECL GLM_CONSTEXPR_CTOR vec(A _x, vec<2, B, Q> const& _yz);
		/// Explicit conversions (From section 5.4.1 Conversion and scalar constructors of GLSL 1.30.08 specification)
		template<typename A, typename B, precision Q>
		GLM_FUNC_DECL GLM_CONSTEXPR_CTOR vec(vec<1, A, Q> const& _x, vec<2, B, Q> const& _yz);
		/// Explicit conversions (From section 5.4.1 Conversion and scalar constructors of GLSL 1.30.08 specification)
		template<typename U, precision Q>
		GLM_FUNC_DECL GLM_CONSTEXPR_CTOR GLM_EXPLICIT vec(vec<4, U, Q> const& v);

		/// Explicit conversions (From section 5.4.1 Conversion and scalar constructors of GLSL 1.30.08 specification)
		template<typename U, precision Q>
		GLM_FUNC_DECL GLM_CONSTEXPR_CTOR GLM_EXPLICIT vec(vec<3, U, Q> const& v);

		// -- Swizzle constructors --
#		if GLM_HAS_UNRESTRICTED_UNIONS && (GLM_SWIZZLE == GLM_SWIZZLE_ENABLED)
			template<int E0, int E1, int E2>
			GLM_FUNC_DECL vec(detail::_swizzle<3, T, P, E0, E1, E2, -1> const & that)
			{
				*this = that();
			}

			template<int E0, int E1>
			GLM_FUNC_DECL vec(detail::_swizzle<2, T, P, E0, E1, -1, -2> const & v, T const & scalar)
			{
				*this = vec(v(), scalar);
			}

			template<int E0, int E1>
			GLM_FUNC_DECL vec(T const & scalar, detail::_swizzle<2, T, P, E0, E1, -1, -2> const & v)
			{
				*this = vec(scalar, v());
			}
#		endif// GLM_HAS_UNRESTRICTED_UNIONS && (GLM_SWIZZLE == GLM_SWIZZLE_ENABLED)

		// -- Unary arithmetic operators --

		GLM_FUNC_DECL vec & operator=(vec const & v) GLM_DEFAULT;

		template<typename U>
		GLM_FUNC_DECL vec & operator=(vec<3, U, P> const & v);
		template<typename U>
		GLM_FUNC_DECL vec & operator+=(U scalar);
		template<typename U>
		GLM_FUNC_DECL vec & operator+=(vec<1, U, P> const & v);
		template<typename U>
		GLM_FUNC_DECL vec & operator+=(vec<3, U, P> const & v);
		template<typename U>
		GLM_FUNC_DECL vec & operator-=(U scalar);
		template<typename U>
		GLM_FUNC_DECL vec & operator-=(vec<1, U, P> const & v);
		template<typename U>
		GLM_FUNC_DECL vec & operator-=(vec<3, U, P> const & v);
		template<typename U>
		GLM_FUNC_DECL vec & operator*=(U scalar);
		template<typename U>
		GLM_FUNC_DECL vec & operator*=(vec<1, U, P> const & v);
		template<typename U>
		GLM_FUNC_DECL vec & operator*=(vec<3, U, P> const & v);
		template<typename U>
		GLM_FUNC_DECL vec & operator/=(U scalar);
		template<typename U>
		GLM_FUNC_DECL vec & operator/=(vec<1, U, P> const & v);
		template<typename U>
		GLM_FUNC_DECL vec & operator/=(vec<3, U, P> const & v);

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
		GLM_FUNC_DECL vec & operator%=(vec<3, U, P> const & v);
		template<typename U>
		GLM_FUNC_DECL vec & operator&=(U scalar);
		template<typename U>
		GLM_FUNC_DECL vec & operator&=(vec<1, U, P> const & v);
		template<typename U>
		GLM_FUNC_DECL vec & operator&=(vec<3, U, P> const & v);
		template<typename U>
		GLM_FUNC_DECL vec & operator|=(U scalar);
		template<typename U>
		GLM_FUNC_DECL vec & operator|=(vec<1, U, P> const & v);
		template<typename U>
		GLM_FUNC_DECL vec & operator|=(vec<3, U, P> const & v);
		template<typename U>
		GLM_FUNC_DECL vec & operator^=(U scalar);
		template<typename U>
		GLM_FUNC_DECL vec & operator^=(vec<1, U, P> const & v);
		template<typename U>
		GLM_FUNC_DECL vec & operator^=(vec<3, U, P> const & v);
		template<typename U>
		GLM_FUNC_DECL vec & operator<<=(U scalar);
		template<typename U>
		GLM_FUNC_DECL vec & operator<<=(vec<1, U, P> const & v);
		template<typename U>
		GLM_FUNC_DECL vec & operator<<=(vec<3, U, P> const & v);
		template<typename U>
		GLM_FUNC_DECL vec & operator>>=(U scalar);
		template<typename U>
		GLM_FUNC_DECL vec & operator>>=(vec<1, U, P> const & v);
		template<typename U>
		GLM_FUNC_DECL vec & operator>>=(vec<3, U, P> const & v);
	};

	// -- Unary operators --

	template<typename T, precision P>
	GLM_FUNC_DECL vec<3, T, P> operator+(vec<3, T, P> const & v);

	template<typename T, precision P>
	GLM_FUNC_DECL vec<3, T, P> operator-(vec<3, T, P> const & v);

	// -- Binary operators --

	template<typename T, precision P>
	GLM_FUNC_DECL vec<3, T, P> operator+(vec<3, T, P> const & v, T scalar);

	template<typename T, precision P>
	GLM_FUNC_DECL vec<3, T, P> operator+(vec<3, T, P> const & v, vec<1, T, P> const & scalar);

	template<typename T, precision P>
	GLM_FUNC_DECL vec<3, T, P> operator+(T scalar, vec<3, T, P> const & v);

	template<typename T, precision P>
	GLM_FUNC_DECL vec<3, T, P> operator+(vec<1, T, P> const & v1, vec<3, T, P> const & v2);

	template<typename T, precision P>
	GLM_FUNC_DECL vec<3, T, P> operator+(vec<3, T, P> const & v1, vec<3, T, P> const & v2);

	template<typename T, precision P>
	GLM_FUNC_DECL vec<3, T, P> operator-(vec<3, T, P> const & v, T scalar);

	template<typename T, precision P>
	GLM_FUNC_DECL vec<3, T, P> operator-(vec<3, T, P> const & v1, vec<1, T, P> const & v2);

	template<typename T, precision P>
	GLM_FUNC_DECL vec<3, T, P> operator-(T scalar, vec<3, T, P> const & v);

	template<typename T, precision P>
	GLM_FUNC_DECL vec<3, T, P> operator-(vec<1, T, P> const & v1, vec<3, T, P> const & v2);

	template<typename T, precision P>
	GLM_FUNC_DECL vec<3, T, P> operator-(vec<3, T, P> const & v1, vec<3, T, P> const & v2);

	template<typename T, precision P>
	GLM_FUNC_DECL vec<3, T, P> operator*(vec<3, T, P> const & v, T scalar);

	template<typename T, precision P>
	GLM_FUNC_DECL vec<3, T, P> operator*(vec<3, T, P> const & v1, vec<1, T, P> const & v2);

	template<typename T, precision P>
	GLM_FUNC_DECL vec<3, T, P> operator*(T scalar, vec<3, T, P> const & v);

	template<typename T, precision P>
	GLM_FUNC_DECL vec<3, T, P> operator*(vec<1, T, P> const & v1, vec<3, T, P> const & v2);

	template<typename T, precision P>
	GLM_FUNC_DECL vec<3, T, P> operator*(vec<3, T, P> const & v1, vec<3, T, P> const & v2);

	template<typename T, precision P>
	GLM_FUNC_DECL vec<3, T, P> operator/(vec<3, T, P> const & v, T scalar);

	template<typename T, precision P>
	GLM_FUNC_DECL vec<3, T, P> operator/(vec<3, T, P> const & v1, vec<1, T, P> const & v2);

	template<typename T, precision P>
	GLM_FUNC_DECL vec<3, T, P> operator/(T scalar, vec<3, T, P> const & v);

	template<typename T, precision P>
	GLM_FUNC_DECL vec<3, T, P> operator/(vec<1, T, P> const & v1, vec<3, T, P> const & v2);

	template<typename T, precision P>
	GLM_FUNC_DECL vec<3, T, P> operator/(vec<3, T, P> const & v1, vec<3, T, P> const & v2);

	template<typename T, precision P>
	GLM_FUNC_DECL vec<3, T, P> operator%(vec<3, T, P> const & v, T scalar);

	template<typename T, precision P>
	GLM_FUNC_DECL vec<3, T, P> operator%(vec<3, T, P> const & v1, vec<1, T, P> const & v2);

	template<typename T, precision P>
	GLM_FUNC_DECL vec<3, T, P> operator%(T const & scalar, vec<3, T, P> const & v);

	template<typename T, precision P>
	GLM_FUNC_DECL vec<3, T, P> operator%(vec<1, T, P> const & v1, vec<3, T, P> const & v2);

	template<typename T, precision P>
	GLM_FUNC_DECL vec<3, T, P> operator%(vec<3, T, P> const & v1, vec<3, T, P> const & v2);

	template<typename T, precision P>
	GLM_FUNC_DECL vec<3, T, P> operator&(vec<3, T, P> const & v1, T scalar);

	template<typename T, precision P>
	GLM_FUNC_DECL vec<3, T, P> operator&(vec<3, T, P> const & v1, vec<1, T, P> const & v2);

	template<typename T, precision P>
	GLM_FUNC_DECL vec<3, T, P> operator&(T scalar, vec<3, T, P> const & v);

	template<typename T, precision P>
	GLM_FUNC_DECL vec<3, T, P> operator&(vec<1, T, P> const & v1, vec<3, T, P> const & v2);

	template<typename T, precision P>
	GLM_FUNC_DECL vec<3, T, P> operator&(vec<3, T, P> const & v1, vec<3, T, P> const & v2);

	template<typename T, precision P>
	GLM_FUNC_DECL vec<3, T, P> operator|(vec<3, T, P> const & v, T scalar);

	template<typename T, precision P>
	GLM_FUNC_DECL vec<3, T, P> operator|(vec<3, T, P> const & v1, vec<1, T, P> const & v2);

	template<typename T, precision P>
	GLM_FUNC_DECL vec<3, T, P> operator|(T scalar, vec<3, T, P> const & v);

	template<typename T, precision P>
	GLM_FUNC_DECL vec<3, T, P> operator|(vec<1, T, P> const & v1, vec<3, T, P> const & v2);

	template<typename T, precision P>
	GLM_FUNC_DECL vec<3, T, P> operator|(vec<3, T, P> const & v1, vec<3, T, P> const & v2);

	template<typename T, precision P>
	GLM_FUNC_DECL vec<3, T, P> operator^(vec<3, T, P> const & v, T scalar);

	template<typename T, precision P>
	GLM_FUNC_DECL vec<3, T, P> operator^(vec<3, T, P> const & v1, vec<1, T, P> const & v2);

	template<typename T, precision P>
	GLM_FUNC_DECL vec<3, T, P> operator^(T scalar, vec<3, T, P> const & v);

	template<typename T, precision P>
	GLM_FUNC_DECL vec<3, T, P> operator^(vec<1, T, P> const & v1, vec<3, T, P> const & v2);

	template<typename T, precision P>
	GLM_FUNC_DECL vec<3, T, P> operator^(vec<3, T, P> const & v1, vec<3, T, P> const & v2);

	template<typename T, precision P>
	GLM_FUNC_DECL vec<3, T, P> operator<<(vec<3, T, P> const & v, T scalar);

	template<typename T, precision P>
	GLM_FUNC_DECL vec<3, T, P> operator<<(vec<3, T, P> const & v1, vec<1, T, P> const & v2);

	template<typename T, precision P>
	GLM_FUNC_DECL vec<3, T, P> operator<<(T scalar, vec<3, T, P> const & v);

	template<typename T, precision P>
	GLM_FUNC_DECL vec<3, T, P> operator<<(vec<1, T, P> const & v1, vec<3, T, P> const & v2);

	template<typename T, precision P>
	GLM_FUNC_DECL vec<3, T, P> operator<<(vec<3, T, P> const & v1, vec<3, T, P> const & v2);

	template<typename T, precision P>
	GLM_FUNC_DECL vec<3, T, P> operator>>(vec<3, T, P> const & v, T scalar);

	template<typename T, precision P>
	GLM_FUNC_DECL vec<3, T, P> operator>>(vec<3, T, P> const & v1, vec<1, T, P> const & v2);

	template<typename T, precision P>
	GLM_FUNC_DECL vec<3, T, P> operator>>(T scalar, vec<3, T, P> const & v);

	template<typename T, precision P>
	GLM_FUNC_DECL vec<3, T, P> operator>>(vec<1, T, P> const & v1, vec<3, T, P> const & v2);

	template<typename T, precision P>
	GLM_FUNC_DECL vec<3, T, P> operator>>(vec<3, T, P> const & v1, vec<3, T, P> const & v2);

	template<typename T, precision P> 
	GLM_FUNC_DECL vec<3, T, P> operator~(vec<3, T, P> const & v);

	// -- Boolean operators --

	template<typename T, precision P>
	GLM_FUNC_DECL bool operator==(vec<3, T, P> const & v1, vec<3, T, P> const & v2);

	template<typename T, precision P>
	GLM_FUNC_DECL bool operator!=(vec<3, T, P> const & v1, vec<3, T, P> const & v2);

	template<precision P>
	GLM_FUNC_DECL vec<3, bool, P> operator&&(vec<3, bool, P> const & v1, vec<3, bool, P> const & v2);

	template<precision P>
	GLM_FUNC_DECL vec<3, bool, P> operator||(vec<3, bool, P> const & v1, vec<3, bool, P> const & v2);
}//namespace glm

#ifndef GLM_EXTERNAL_TEMPLATE
#include "type_vec3.inl"
#endif//GLM_EXTERNAL_TEMPLATE
