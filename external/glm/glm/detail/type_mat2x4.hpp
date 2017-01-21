/// @ref core
/// @file glm/detail/type_mat2x4.hpp

#pragma once

#include "../fwd.hpp"
#include "type_vec2.hpp"
#include "type_vec4.hpp"
#include "type_mat.hpp"
#include <limits>
#include <cstddef>

namespace glm
{
	template<typename T, precision P>
	struct mat<2, 4, T, P>
	{
		typedef vec<4, T, P> col_type;
		typedef vec<2, T, P> row_type;
		typedef mat<2, 4, T, P> type;
		typedef mat<4, 2, T, P> transpose_type;
		typedef T value_type;

	private:
		col_type value[2];

	public:
		// -- Constructors --

		GLM_FUNC_DECL mat() GLM_DEFAULT_CTOR;
		GLM_FUNC_DECL mat(mat<2, 4, T, P> const & m) GLM_DEFAULT;
		template<precision Q>
		GLM_FUNC_DECL mat(mat<2, 4, T, Q> const & m);

		GLM_FUNC_DECL GLM_CONSTEXPR_CTOR explicit mat(ctor);
		GLM_FUNC_DECL explicit mat(T scalar);
		GLM_FUNC_DECL mat(
			T x0, T y0, T z0, T w0,
			T x1, T y1, T z1, T w1);
		GLM_FUNC_DECL mat(
			col_type const & v0,
			col_type const & v1);

		// -- Conversions --

		template<
			typename X1, typename Y1, typename Z1, typename W1,
			typename X2, typename Y2, typename Z2, typename W2>
		GLM_FUNC_DECL mat(
			X1 x1, Y1 y1, Z1 z1, W1 w1,
			X2 x2, Y2 y2, Z2 z2, W2 w2);

		template<typename U, typename V>
		GLM_FUNC_DECL mat(
			vec<4, U, P> const & v1,
			vec<4, V, P> const & v2);

		// -- Matrix conversions --

		template<typename U, precision Q>
		GLM_FUNC_DECL GLM_EXPLICIT mat(mat<2, 4, U, Q> const & m);

		GLM_FUNC_DECL GLM_EXPLICIT mat(mat<2, 2, T, P> const & x);
		GLM_FUNC_DECL GLM_EXPLICIT mat(mat<3, 3, T, P> const & x);
		GLM_FUNC_DECL GLM_EXPLICIT mat(mat<4, 4, T, P> const & x);
		GLM_FUNC_DECL GLM_EXPLICIT mat(mat<2, 3, T, P> const & x);
		GLM_FUNC_DECL GLM_EXPLICIT mat(mat<3, 2, T, P> const & x);
		GLM_FUNC_DECL GLM_EXPLICIT mat(mat<3, 4, T, P> const & x);
		GLM_FUNC_DECL GLM_EXPLICIT mat(mat<4, 2, T, P> const & x);
		GLM_FUNC_DECL GLM_EXPLICIT mat(mat<4, 3, T, P> const & x);

		// -- Accesses --

		typedef length_t length_type;
		GLM_FUNC_DECL static length_type length(){return 2;}

		GLM_FUNC_DECL col_type & operator[](length_type i);
		GLM_FUNC_DECL col_type const & operator[](length_type i) const;

		// -- Unary arithmetic operators --

		GLM_FUNC_DECL mat<2, 4, T, P> & operator=(mat<2, 4, T, P> const & m) GLM_DEFAULT;

		template<typename U>
		GLM_FUNC_DECL mat<2, 4, T, P> & operator=(mat<2, 4, U, P> const & m);
		template<typename U>
		GLM_FUNC_DECL mat<2, 4, T, P> & operator+=(U s);
		template<typename U>
		GLM_FUNC_DECL mat<2, 4, T, P> & operator+=(mat<2, 4, U, P> const & m);
		template<typename U>
		GLM_FUNC_DECL mat<2, 4, T, P> & operator-=(U s);
		template<typename U>
		GLM_FUNC_DECL mat<2, 4, T, P> & operator-=(mat<2, 4, U, P> const & m);
		template<typename U>
		GLM_FUNC_DECL mat<2, 4, T, P> & operator*=(U s);
		template<typename U>
		GLM_FUNC_DECL mat<2, 4, T, P> & operator/=(U s);

		// -- Increment and decrement operators --

		GLM_FUNC_DECL mat<2, 4, T, P> & operator++ ();
		GLM_FUNC_DECL mat<2, 4, T, P> & operator-- ();
		GLM_FUNC_DECL mat<2, 4, T, P> operator++(int);
		GLM_FUNC_DECL mat<2, 4, T, P> operator--(int);
	};

	// -- Unary operators --

	template<typename T, precision P>
	GLM_FUNC_DECL mat<2, 4, T, P> operator+(mat<2, 4, T, P> const & m);

	template<typename T, precision P>
	GLM_FUNC_DECL mat<2, 4, T, P> operator-(mat<2, 4, T, P> const & m);

	// -- Binary operators --

	template<typename T, precision P>
	GLM_FUNC_DECL mat<2, 4, T, P> operator+(mat<2, 4, T, P> const & m, T scalar);

	template<typename T, precision P>
	GLM_FUNC_DECL mat<2, 4, T, P> operator+(mat<2, 4, T, P> const & m1, mat<2, 4, T, P> const & m2);

	template<typename T, precision P>
	GLM_FUNC_DECL mat<2, 4, T, P> operator-(mat<2, 4, T, P> const & m, T scalar);

	template<typename T, precision P>
	GLM_FUNC_DECL mat<2, 4, T, P> operator-(mat<2, 4, T, P> const & m1, mat<2, 4, T, P> const & m2);

	template<typename T, precision P>
	GLM_FUNC_DECL mat<2, 4, T, P> operator*(mat<2, 4, T, P> const & m, T scalar);

	template<typename T, precision P>
	GLM_FUNC_DECL mat<2, 4, T, P> operator*(T scalar, mat<2, 4, T, P> const & m);

	template<typename T, precision P>
	GLM_FUNC_DECL typename mat<2, 4, T, P>::col_type operator*(mat<2, 4, T, P> const & m, typename mat<2, 4, T, P>::row_type const & v);

	template<typename T, precision P>
	GLM_FUNC_DECL typename mat<2, 4, T, P>::row_type operator*(typename mat<2, 4, T, P>::col_type const & v, mat<2, 4, T, P> const & m);

	template<typename T, precision P>
	GLM_FUNC_DECL mat<4, 4, T, P> operator*(mat<2, 4, T, P> const & m1, mat<4, 2, T, P> const & m2);

	template<typename T, precision P>
	GLM_FUNC_DECL mat<2, 4, T, P> operator*(mat<2, 4, T, P> const & m1, mat<2, 2, T, P> const & m2);

	template<typename T, precision P>
	GLM_FUNC_DECL mat<3, 4, T, P> operator*(mat<2, 4, T, P> const & m1, mat<3, 2, T, P> const & m2);

	template<typename T, precision P>
	GLM_FUNC_DECL mat<2, 4, T, P> operator/(mat<2, 4, T, P> const & m, T scalar);

	template<typename T, precision P>
	GLM_FUNC_DECL mat<2, 4, T, P> operator/(T scalar, mat<2, 4, T, P> const & m);

	// -- Boolean operators --

	template<typename T, precision P>
	GLM_FUNC_DECL bool operator==(mat<2, 4, T, P> const & m1, mat<2, 4, T, P> const & m2);

	template<typename T, precision P>
	GLM_FUNC_DECL bool operator!=(mat<2, 4, T, P> const & m1, mat<2, 4, T, P> const & m2);
}//namespace glm

#ifndef GLM_EXTERNAL_TEMPLATE
#include "type_mat2x4.inl"
#endif
