/// @ref core
/// @file glm/detail/type_mat4x3.hpp

#pragma once

#include "../fwd.hpp"
#include "type_vec3.hpp"
#include "type_vec4.hpp"
#include "type_mat.hpp"
#include <limits>
#include <cstddef>

namespace glm
{
	template<typename T, precision P>
	struct mat<4, 3, T, P>
	{
		typedef vec<3, T, P> col_type;
		typedef vec<4, T, P> row_type;
		typedef mat<4, 3, T, P> type;
		typedef mat<3, 4, T, P> transpose_type;
		typedef T value_type;

	private:
		col_type value[4];

	public:
		// -- Constructors --

		GLM_FUNC_DECL mat() GLM_DEFAULT_CTOR;
		GLM_FUNC_DECL mat(mat<4, 3, T, P> const & m) GLM_DEFAULT;
		template<precision Q>
		GLM_FUNC_DECL mat(mat<4, 3, T, Q> const & m);

		GLM_FUNC_DECL GLM_CONSTEXPR_CTOR explicit mat(ctor);
		GLM_FUNC_DECL explicit mat(T const & x);
		GLM_FUNC_DECL mat(
			T const & x0, T const & y0, T const & z0,
			T const & x1, T const & y1, T const & z1,
			T const & x2, T const & y2, T const & z2,
			T const & x3, T const & y3, T const & z3);
		GLM_FUNC_DECL mat(
			col_type const & v0,
			col_type const & v1,
			col_type const & v2,
			col_type const & v3);

		// -- Conversions --

		template<
			typename X1, typename Y1, typename Z1,
			typename X2, typename Y2, typename Z2,
			typename X3, typename Y3, typename Z3,
			typename X4, typename Y4, typename Z4>
		GLM_FUNC_DECL mat(
			X1 const & x1, Y1 const & y1, Z1 const & z1,
			X2 const & x2, Y2 const & y2, Z2 const & z2,
			X3 const & x3, Y3 const & y3, Z3 const & z3,
			X4 const & x4, Y4 const & y4, Z4 const & z4);

		template<typename V1, typename V2, typename V3, typename V4>
		GLM_FUNC_DECL mat(
			vec<3, V1, P> const & v1,
			vec<3, V2, P> const & v2,
			vec<3, V3, P> const & v3,
			vec<3, V4, P> const & v4);

		// -- Matrix conversions --

		template<typename U, precision Q>
		GLM_FUNC_DECL GLM_EXPLICIT mat(mat<4, 3, U, Q> const & m);

		GLM_FUNC_DECL GLM_EXPLICIT mat(mat<2, 2, T, P> const & x);
		GLM_FUNC_DECL GLM_EXPLICIT mat(mat<3, 3, T, P> const & x);
		GLM_FUNC_DECL GLM_EXPLICIT mat(mat<4, 4, T, P> const & x);
		GLM_FUNC_DECL GLM_EXPLICIT mat(mat<2, 3, T, P> const & x);
		GLM_FUNC_DECL GLM_EXPLICIT mat(mat<3, 2, T, P> const & x);
		GLM_FUNC_DECL GLM_EXPLICIT mat(mat<2, 4, T, P> const & x);
		GLM_FUNC_DECL GLM_EXPLICIT mat(mat<4, 2, T, P> const & x);
		GLM_FUNC_DECL GLM_EXPLICIT mat(mat<3, 4, T, P> const & x);

		// -- Accesses --

		typedef length_t length_type;
		GLM_FUNC_DECL static length_type length(){return 4;}

		GLM_FUNC_DECL col_type & operator[](length_type i);
		GLM_FUNC_DECL col_type const & operator[](length_type i) const;

		// -- Unary arithmetic operators --

		GLM_FUNC_DECL mat<4, 3, T, P> & operator=(mat<4, 3, T, P> const & m) GLM_DEFAULT;

		template<typename U>
		GLM_FUNC_DECL mat<4, 3, T, P> & operator=(mat<4, 3, U, P> const & m);
		template<typename U>
		GLM_FUNC_DECL mat<4, 3, T, P> & operator+=(U s);
		template<typename U>
		GLM_FUNC_DECL mat<4, 3, T, P> & operator+=(mat<4, 3, U, P> const & m);
		template<typename U>
		GLM_FUNC_DECL mat<4, 3, T, P> & operator-=(U s);
		template<typename U>
		GLM_FUNC_DECL mat<4, 3, T, P> & operator-=(mat<4, 3, U, P> const & m);
		template<typename U>
		GLM_FUNC_DECL mat<4, 3, T, P> & operator*=(U s);
		template<typename U>
		GLM_FUNC_DECL mat<4, 3, T, P> & operator/=(U s);

		// -- Increment and decrement operators --

		GLM_FUNC_DECL mat<4, 3, T, P>& operator++();
		GLM_FUNC_DECL mat<4, 3, T, P>& operator--();
		GLM_FUNC_DECL mat<4, 3, T, P> operator++(int);
		GLM_FUNC_DECL mat<4, 3, T, P> operator--(int);
	};

	// -- Unary operators --

	template<typename T, precision P>
	GLM_FUNC_DECL mat<4, 3, T, P> operator+(mat<4, 3, T, P> const & m);

	template<typename T, precision P>
	GLM_FUNC_DECL mat<4, 3, T, P> operator-(mat<4, 3, T, P> const & m);

	// -- Binary operators --

	template<typename T, precision P>
	GLM_FUNC_DECL mat<4, 3, T, P> operator+(mat<4, 3, T, P> const & m, T const & s);

	template<typename T, precision P>
	GLM_FUNC_DECL mat<4, 3, T, P> operator+(mat<4, 3, T, P> const & m1, mat<4, 3, T, P> const & m2);

	template<typename T, precision P>
	GLM_FUNC_DECL mat<4, 3, T, P> operator-(mat<4, 3, T, P> const & m, T const & s);

	template<typename T, precision P>
	GLM_FUNC_DECL mat<4, 3, T, P> operator-(mat<4, 3, T, P> const & m1, mat<4, 3, T, P> const & m2);

	template<typename T, precision P>
	GLM_FUNC_DECL mat<4, 3, T, P> operator*(mat<4, 3, T, P> const & m, T const & s);

	template<typename T, precision P>
	GLM_FUNC_DECL mat<4, 3, T, P> operator*(T const & s, mat<4, 3, T, P> const & m);

	template<typename T, precision P>
	GLM_FUNC_DECL typename mat<4, 3, T, P>::col_type operator*(mat<4, 3, T, P> const & m, typename mat<4, 3, T, P>::row_type const & v);

	template<typename T, precision P>
	GLM_FUNC_DECL typename mat<4, 3, T, P>::row_type operator*(typename mat<4, 3, T, P>::col_type const & v, mat<4, 3, T, P> const & m);

	template<typename T, precision P>
	GLM_FUNC_DECL mat<2, 3, T, P> operator*(mat<4, 3, T, P> const & m1, mat<2, 4, T, P> const & m2);

	template<typename T, precision P>
	GLM_FUNC_DECL mat<3, 3, T, P> operator*(mat<4, 3, T, P> const & m1,	mat<3, 4, T, P> const& m2);

	template<typename T, precision P>
	GLM_FUNC_DECL mat<4, 3, T, P> operator*(mat<4, 3, T, P> const & m1, mat<4, 4, T, P> const & m2);

	template<typename T, precision P>
	GLM_FUNC_DECL mat<4, 3, T, P> operator/(mat<4, 3, T, P> const & m, T const & s);

	template<typename T, precision P>
	GLM_FUNC_DECL mat<4, 3, T, P> operator/(T const & s, mat<4, 3, T, P> const & m);

	// -- Boolean operators --

	template<typename T, precision P>
	GLM_FUNC_DECL bool operator==(mat<4, 3, T, P> const & m1, mat<4, 3, T, P> const & m2);

	template<typename T, precision P>
	GLM_FUNC_DECL bool operator!=(mat<4, 3, T, P> const & m1, mat<4, 3, T, P> const & m2);
}//namespace glm

#ifndef GLM_EXTERNAL_TEMPLATE
#include "type_mat4x3.inl"
#endif //GLM_EXTERNAL_TEMPLATE
