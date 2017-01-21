/// @ref gtx_closest_point
/// @file glm/gtx/closest_point.hpp
///
/// @see core (dependence)
///
/// @defgroup gtx_closest_point GLM_GTX_closest_point
/// @ingroup gtx
///
/// @brief Find the point on a straight line which is the closet of a point.
///
/// <glm/gtx/closest_point.hpp> need to be included to use these functionalities.

#pragma once

// Dependency:
#include "../glm.hpp"

#ifndef GLM_ENABLE_EXPERIMENTAL
#	error "GLM: GLM_GTX_closest_point is an experimental extension and may change in the future. Use #define GLM_ENABLE_EXPERIMENTAL before including it, if you really want to use it."
#endif

#if GLM_MESSAGES == GLM_MESSAGES_ENABLED && !defined(GLM_EXT_INCLUDED)
#	pragma message("GLM: GLM_GTX_closest_point extension included")
#endif

namespace glm
{
	/// @addtogroup gtx_closest_point
	/// @{

	/// Find the point on a straight line which is the closet of a point. 
	/// @see gtx_closest_point
	template<typename T, precision P>
	GLM_FUNC_DECL vec<3, T, P> closestPointOnLine(
		vec<3, T, P> const & point,
		vec<3, T, P> const & a, 
		vec<3, T, P> const & b);
	
	/// 2d lines work as well	
	template<typename T, precision P>
	GLM_FUNC_DECL vec<2, T, P> closestPointOnLine(
		vec<2, T, P> const & point,
		vec<2, T, P> const & a, 
		vec<2, T, P> const & b);	

	/// @}
}// namespace glm

#include "closest_point.inl"
