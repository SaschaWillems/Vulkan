/// @ref gtx_matrix_cross_product
/// @file glm/gtx/matrix_cross_product.hpp
///
/// @see core (dependence)
/// @see gtx_extented_min_max (dependence)
///
/// @defgroup gtx_matrix_cross_product GLM_GTX_matrix_cross_product
/// @ingroup gtx
///
/// @brief Build cross product matrices
///
/// <glm/gtx/matrix_cross_product.hpp> need to be included to use these functionalities.

#pragma once

// Dependency:
#include "../glm.hpp"

#ifndef GLM_ENABLE_EXPERIMENTAL
#	error "GLM: GLM_GTX_matrix_cross_product is an experimental extension and may change in the future. Use #define GLM_ENABLE_EXPERIMENTAL before including it, if you really want to use it."
#endif

#if GLM_MESSAGES == GLM_MESSAGES_ENABLED && !defined(GLM_EXT_INCLUDED)
#	pragma message("GLM: GLM_GTX_matrix_cross_product extension included")
#endif

namespace glm
{
	/// @addtogroup gtx_matrix_cross_product
	/// @{

	//! Build a cross product matrix.
	//! From GLM_GTX_matrix_cross_product extension.
	template<typename T, precision P>
	GLM_FUNC_DECL mat<3, 3, T, P> matrixCross3(
		vec<3, T, P> const & x);
		
	//! Build a cross product matrix.
	//! From GLM_GTX_matrix_cross_product extension.
	template<typename T, precision P>
	GLM_FUNC_DECL mat<4, 4, T, P> matrixCross4(
		vec<3, T, P> const & x);

	/// @}
}//namespace glm

#include "matrix_cross_product.inl"
