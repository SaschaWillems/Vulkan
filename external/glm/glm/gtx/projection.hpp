/// @ref gtx_projection
/// @file glm/gtx/projection.hpp
///
/// @see core (dependence)
///
/// @defgroup gtx_projection GLM_GTX_projection
/// @ingroup gtx
///
/// @brief Projection of a vector to other one
///
/// <glm/gtx/projection.hpp> need to be included to use these functionalities.

#pragma once

// Dependency:
#include "../geometric.hpp"

#ifndef GLM_ENABLE_EXPERIMENTAL
#	error "GLM: GLM_GTX_projection is an experimental extension and may change in the future. Use #define GLM_ENABLE_EXPERIMENTAL before including it, if you really want to use it."
#endif

#if GLM_MESSAGES == GLM_MESSAGES_ENABLED && !defined(GLM_EXT_INCLUDED)
#	pragma message("GLM: GLM_GTX_projection extension included")
#endif

namespace glm
{
	/// @addtogroup gtx_projection
	/// @{

	/// Projects x on Normal.
	///
	/// @see gtx_projection
	template<typename vecType>
	GLM_FUNC_DECL vecType proj(vecType const & x, vecType const & Normal);

	/// @}
}//namespace glm

#include "projection.inl"
