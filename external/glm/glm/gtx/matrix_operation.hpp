/// @ref gtx_matrix_operation
/// @file glm/gtx/matrix_operation.hpp
///
/// @see core (dependence)
///
/// @defgroup gtx_matrix_operation GLM_GTX_matrix_operation
/// @ingroup gtx
///
/// @brief Build diagonal matrices from vectors.
///
/// <glm/gtx/matrix_operation.hpp> need to be included to use these functionalities.

#pragma once

// Dependency:
#include "../glm.hpp"

#ifndef GLM_ENABLE_EXPERIMENTAL
#	error "GLM: GLM_GTX_matrix_operation is an experimental extension and may change in the future. Use #define GLM_ENABLE_EXPERIMENTAL before including it, if you really want to use it."
#endif

#if GLM_MESSAGES == GLM_MESSAGES_ENABLED && !defined(GLM_EXT_INCLUDED)
#	pragma message("GLM: GLM_GTX_matrix_operation extension included")
#endif

namespace glm
{
	/// @addtogroup gtx_matrix_operation
	/// @{

	//! Build a diagonal matrix.
	//! From GLM_GTX_matrix_operation extension.
	template<typename T, precision P>
	GLM_FUNC_DECL mat<2, 2, T, P> diagonal2x2(
		vec<2, T, P> const & v);

	//! Build a diagonal matrix.
	//! From GLM_GTX_matrix_operation extension.
	template<typename T, precision P>
	GLM_FUNC_DECL mat<2, 3, T, P> diagonal2x3(
		vec<2, T, P> const & v);

	//! Build a diagonal matrix.
	//! From GLM_GTX_matrix_operation extension.
	template<typename T, precision P>
	GLM_FUNC_DECL mat<2, 4, T, P> diagonal2x4(
		vec<2, T, P> const & v);

	//! Build a diagonal matrix.
	//! From GLM_GTX_matrix_operation extension.
	template<typename T, precision P>
	GLM_FUNC_DECL mat<3, 2, T, P> diagonal3x2(
		vec<2, T, P> const & v);

	//! Build a diagonal matrix.
	//! From GLM_GTX_matrix_operation extension.
	template<typename T, precision P>
	GLM_FUNC_DECL mat<3, 3, T, P> diagonal3x3(
		vec<3, T, P> const & v);

	//! Build a diagonal matrix.
	//! From GLM_GTX_matrix_operation extension.
	template<typename T, precision P>
	GLM_FUNC_DECL mat<3, 4, T, P> diagonal3x4(
		vec<3, T, P> const & v);

	//! Build a diagonal matrix.
	//! From GLM_GTX_matrix_operation extension.
	template<typename T, precision P>
	GLM_FUNC_DECL mat<4, 2, T, P> diagonal4x2(
		vec<2, T, P> const & v);

	//! Build a diagonal matrix.
	//! From GLM_GTX_matrix_operation extension.
	template<typename T, precision P>
	GLM_FUNC_DECL mat<4, 3, T, P> diagonal4x3(
		vec<3, T, P> const & v);

	//! Build a diagonal matrix.
	//! From GLM_GTX_matrix_operation extension.
	template<typename T, precision P>
	GLM_FUNC_DECL mat<4, 4, T, P> diagonal4x4(
		vec<4, T, P> const & v);

	/// @}
}//namespace glm

#include "matrix_operation.inl"
