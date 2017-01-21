/// @ref gtx_normal
/// @file glm/gtx/normal.hpp
///
/// @see core (dependence)
/// @see gtx_extented_min_max (dependence)
///
/// @defgroup gtx_normal GLM_GTX_normal
/// @ingroup gtx
///
/// @brief Compute the normal of a triangle.
///
/// <glm/gtx/normal.hpp> need to be included to use these functionalities.

#pragma once

// Dependency:
#include "../glm.hpp"

#ifndef GLM_ENABLE_EXPERIMENTAL
#	error "GLM: GLM_GTX_normal is an experimental extension and may change in the future. Use #define GLM_ENABLE_EXPERIMENTAL before including it, if you really want to use it."
#endif

#if GLM_MESSAGES == GLM_MESSAGES_ENABLED && !defined(GLM_EXT_INCLUDED)
#	pragma message("GLM: GLM_GTX_normal extension included")
#endif

namespace glm
{
	/// @addtogroup gtx_normal
	/// @{

	//! Computes triangle normal from triangle points. 
	//! From GLM_GTX_normal extension.
	template<typename T, precision P> 
	GLM_FUNC_DECL vec<3, T, P> triangleNormal(
		vec<3, T, P> const & p1, 
		vec<3, T, P> const & p2, 
		vec<3, T, P> const & p3);

	/// @}
}//namespace glm

#include "normal.inl"
