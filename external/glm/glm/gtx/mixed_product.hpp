/// @ref gtx_mixed_product
/// @file glm/gtx/mixed_product.hpp
///
/// @see core (dependence)
///
/// @defgroup gtx_mixed_product GLM_GTX_mixed_producte
/// @ingroup gtx
///
/// @brief Mixed product of 3 vectors.
///
/// <glm/gtx/mixed_product.hpp> need to be included to use these functionalities.

#pragma once

// Dependency:
#include "../glm.hpp"

#ifndef GLM_ENABLE_EXPERIMENTAL
#	error "GLM: GLM_GTX_mixed_product is an experimental extension and may change in the future. Use #define GLM_ENABLE_EXPERIMENTAL before including it, if you really want to use it."
#endif

#if GLM_MESSAGES == GLM_MESSAGES_ENABLED && !defined(GLM_EXT_INCLUDED)
#	pragma message("GLM: GLM_GTX_mixed_product extension included")
#endif

namespace glm
{
	/// @addtogroup gtx_mixed_product
	/// @{

	/// @brief Mixed product of 3 vectors (from GLM_GTX_mixed_product extension)
	template<typename T, precision P> 
	GLM_FUNC_DECL T mixedProduct(
		vec<3, T, P> const & v1, 
		vec<3, T, P> const & v2, 
		vec<3, T, P> const & v3);

	/// @}
}// namespace glm

#include "mixed_product.inl"
