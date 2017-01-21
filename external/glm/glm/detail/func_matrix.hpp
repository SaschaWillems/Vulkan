/// @ref core
/// @file glm/detail/func_matrix.hpp
///
/// @see <a href="http://www.opengl.org/registry/doc/GLSLangSpec.4.20.8.pdf">GLSL 4.20.8 specification, section 8.6 Matrix Functions</a>
/// 
/// @defgroup core_func_matrix Matrix functions
/// @ingroup core
/// 
/// For each of the following built-in matrix functions, there is both a 
/// single-precision floating point version, where all arguments and return values 
/// are single precision, and a double-precision floating version, where all 
/// arguments and return values are double precision. Only the single-precision 
/// floating point version is shown.

#pragma once

// Dependencies
#include "../detail/precision.hpp"
#include "../detail/setup.hpp"
#include "../detail/type_mat.hpp"
#include "../vec2.hpp"
#include "../vec3.hpp"
#include "../vec4.hpp"
#include "../mat2x2.hpp"
#include "../mat2x3.hpp"
#include "../mat2x4.hpp"
#include "../mat3x2.hpp"
#include "../mat3x3.hpp"
#include "../mat3x4.hpp"
#include "../mat4x2.hpp"
#include "../mat4x3.hpp"
#include "../mat4x4.hpp"

namespace glm{
namespace detail
{
	template<typename T, precision P>
	struct outerProduct_trait<2, 2, T, P, vec, vec>
	{
		typedef mat<2, 2, T, P> type;
	};

	template<typename T, precision P>
	struct outerProduct_trait<2, 3, T, P, vec, vec>
	{
		typedef mat<3, 2, T, P> type;
	};

	template<typename T, precision P>
	struct outerProduct_trait<2, 4, T, P, vec, vec>
	{
		typedef mat<4, 2, T, P> type;
	};

	template<typename T, precision P>
	struct outerProduct_trait<3, 2, T, P, vec, vec>
	{
		typedef mat<2, 3, T, P> type;
	};

	template<typename T, precision P>
	struct outerProduct_trait<3, 3, T, P, vec, vec>
	{
		typedef mat<3, 3, T, P> type;
	};

	template<typename T, precision P>
	struct outerProduct_trait<3, 4, T, P, vec, vec>
	{
		typedef mat<4, 3, T, P> type;
	};

	template<typename T, precision P>
	struct outerProduct_trait<4, 2, T, P, vec, vec>
	{
		typedef mat<2, 4, T, P> type;
	};

	template<typename T, precision P>
	struct outerProduct_trait<4, 3, T, P, vec, vec>
	{
		typedef mat<3, 4, T, P> type;
	};

	template<typename T, precision P>
	struct outerProduct_trait<4, 4, T, P, vec, vec>
	{
		typedef mat<4, 4, T, P> type;
	};

}//namespace detail

	/// @addtogroup core_func_matrix
	/// @{

	/// Multiply matrix x by matrix y component-wise, i.e., 
	/// result[i][j] is the scalar product of x[i][j] and y[i][j].
	/// 
	/// @tparam matType Floating-point matrix types.
	///
	/// @see <a href="http://www.opengl.org/sdk/docs/manglsl/xhtml/matrixCompMult.xml">GLSL matrixCompMult man page</a>
	/// @see <a href="http://www.opengl.org/registry/doc/GLSLangSpec.4.20.8.pdf">GLSL 4.20.8 specification, section 8.6 Matrix Functions</a>
	template<typename T, precision P, template<typename, precision> class matType>
	GLM_FUNC_DECL matType<T, P> matrixCompMult(matType<T, P> const & x, matType<T, P> const & y);

	/// Treats the first parameter c as a column vector
	/// and the second parameter r as a row vector
	/// and does a linear algebraic matrix multiply c * r.
	/// 
	/// @tparam matType Floating-point matrix types.
	///
	/// @see <a href="http://www.opengl.org/sdk/docs/manglsl/xhtml/outerProduct.xml">GLSL outerProduct man page</a>
	/// @see <a href="http://www.opengl.org/registry/doc/GLSLangSpec.4.20.8.pdf">GLSL 4.20.8 specification, section 8.6 Matrix Functions</a>
	template<int DA, int DB, typename T, precision P, template<length_t, typename, precision> class vecTypeA, template<length_t, typename, precision> class vecTypeB>
	GLM_FUNC_DECL typename detail::outerProduct_trait<DA, DB, T, P, vecTypeA, vecTypeB>::type outerProduct(vecTypeA<DA, T, P> const & c, vecTypeB<DB, T, P> const & r);

	/// Returns the transposed matrix of x
	/// 
	/// @tparam matType Floating-point matrix types.
	///
	/// @see <a href="http://www.opengl.org/sdk/docs/manglsl/xhtml/transpose.xml">GLSL transpose man page</a>
	/// @see <a href="http://www.opengl.org/registry/doc/GLSLangSpec.4.20.8.pdf">GLSL 4.20.8 specification, section 8.6 Matrix Functions</a>
	template<typename T, precision P, template<typename, precision> class matType>
	GLM_FUNC_DECL typename matType<T, P>::transpose_type transpose(matType<T, P> const & x);
	
	/// Return the determinant of a squared matrix.
	/// 
	/// @tparam valType Floating-point scalar types.
	///
	/// @see <a href="http://www.opengl.org/sdk/docs/manglsl/xhtml/determinant.xml">GLSL determinant man page</a>
	/// @see <a href="http://www.opengl.org/registry/doc/GLSLangSpec.4.20.8.pdf">GLSL 4.20.8 specification, section 8.6 Matrix Functions</a>	
	template<typename T, precision P, template<typename, precision> class matType>
	GLM_FUNC_DECL T determinant(matType<T, P> const & m);

	/// Return the inverse of a squared matrix.
	/// 
	/// @tparam valType Floating-point scalar types.
	///
	/// @see <a href="http://www.opengl.org/sdk/docs/manglsl/xhtml/inverse.xml">GLSL inverse man page</a>
	/// @see <a href="http://www.opengl.org/registry/doc/GLSLangSpec.4.20.8.pdf">GLSL 4.20.8 specification, section 8.6 Matrix Functions</a>	 
	template<typename T, precision P, template<typename, precision> class matType>
	GLM_FUNC_DECL matType<T, P> inverse(matType<T, P> const & m);

	/// @}
}//namespace glm

#include "func_matrix.inl"
