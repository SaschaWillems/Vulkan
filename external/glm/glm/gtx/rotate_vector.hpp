/// @ref gtx_rotate_vector
/// @file glm/gtx/rotate_vector.hpp
///
/// @see core (dependence)
/// @see gtx_transform (dependence)
///
/// @defgroup gtx_rotate_vector GLM_GTX_rotate_vector
/// @ingroup gtx
///
/// @brief Function to directly rotate a vector
///
/// <glm/gtx/rotate_vector.hpp> need to be included to use these functionalities.

#pragma once

// Dependency:
#include "../glm.hpp"
#include "../gtx/transform.hpp"

#ifndef GLM_ENABLE_EXPERIMENTAL
#	error "GLM: GLM_GTX_rotate_vector is an experimental extension and may change in the future. Use #define GLM_ENABLE_EXPERIMENTAL before including it, if you really want to use it."
#endif

#if GLM_MESSAGES == GLM_MESSAGES_ENABLED && !defined(GLM_EXT_INCLUDED)
#	pragma message("GLM: GLM_GTX_rotate_vector extension included")
#endif

namespace glm
{
	/// @addtogroup gtx_rotate_vector
	/// @{

	/// Returns Spherical interpolation between two vectors
	/// 
	/// @param x A first vector
	/// @param y A second vector
	/// @param a Interpolation factor. The interpolation is defined beyond the range [0, 1].
	/// 
	/// @see gtx_rotate_vector
	template<typename T, precision P>
	GLM_FUNC_DECL vec<3, T, P> slerp(
		vec<3, T, P> const & x,
		vec<3, T, P> const & y,
		T const & a);

	//! Rotate a two dimensional vector.
	//! From GLM_GTX_rotate_vector extension.
	template<typename T, precision P>
	GLM_FUNC_DECL vec<2, T, P> rotate(
		vec<2, T, P> const & v,
		T const & angle);
		
	//! Rotate a three dimensional vector around an axis.
	//! From GLM_GTX_rotate_vector extension.
	template<typename T, precision P>
	GLM_FUNC_DECL vec<3, T, P> rotate(
		vec<3, T, P> const & v,
		T const & angle,
		vec<3, T, P> const & normal);
		
	//! Rotate a four dimensional vector around an axis.
	//! From GLM_GTX_rotate_vector extension.
	template<typename T, precision P>
	GLM_FUNC_DECL vec<4, T, P> rotate(
		vec<4, T, P> const & v,
		T const & angle,
		vec<3, T, P> const & normal);
		
	//! Rotate a three dimensional vector around the X axis.
	//! From GLM_GTX_rotate_vector extension.
	template<typename T, precision P>
	GLM_FUNC_DECL vec<3, T, P> rotateX(
		vec<3, T, P> const & v,
		T const & angle);

	//! Rotate a three dimensional vector around the Y axis.
	//! From GLM_GTX_rotate_vector extension.
	template<typename T, precision P>
	GLM_FUNC_DECL vec<3, T, P> rotateY(
		vec<3, T, P> const & v,
		T const & angle);
		
	//! Rotate a three dimensional vector around the Z axis.
	//! From GLM_GTX_rotate_vector extension.
	template<typename T, precision P>
	GLM_FUNC_DECL vec<3, T, P> rotateZ(
		vec<3, T, P> const & v,
		T const & angle);
		
	//! Rotate a four dimentionnals vector around the X axis.
	//! From GLM_GTX_rotate_vector extension.
	template<typename T, precision P>
	GLM_FUNC_DECL vec<4, T, P> rotateX(
		vec<4, T, P> const & v,
		T const & angle);
		
	//! Rotate a four dimensional vector around the X axis.
	//! From GLM_GTX_rotate_vector extension.
	template<typename T, precision P>
	GLM_FUNC_DECL vec<4, T, P> rotateY(
		vec<4, T, P> const & v,
		T const & angle);
		
	//! Rotate a four dimensional vector around the X axis.
	//! From GLM_GTX_rotate_vector extension.
	template<typename T, precision P>
	GLM_FUNC_DECL vec<4, T, P> rotateZ(
		vec<4, T, P> const & v,
		T const & angle);
		
	//! Build a rotation matrix from a normal and a up vector.
	//! From GLM_GTX_rotate_vector extension.
	template<typename T, precision P>
	GLM_FUNC_DECL mat<4, 4, T, P> orientation(
		vec<3, T, P> const & Normal,
		vec<3, T, P> const & Up);

	/// @}
}//namespace glm

#include "rotate_vector.inl"
