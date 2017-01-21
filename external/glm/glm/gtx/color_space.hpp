/// @ref gtx_color_space
/// @file glm/gtx/color_space.hpp
///
/// @see core (dependence)
///
/// @defgroup gtx_color_space GLM_GTX_color_space
/// @ingroup gtx
///
/// @brief Related to RGB to HSV conversions and operations.
///
/// <glm/gtx/color_space.hpp> need to be included to use these functionalities.

#pragma once

// Dependency:
#include "../glm.hpp"

#ifndef GLM_ENABLE_EXPERIMENTAL
#	error "GLM: GLM_GTX_color_space is an experimental extension and may change in the future. Use #define GLM_ENABLE_EXPERIMENTAL before including it, if you really want to use it."
#endif

#if GLM_MESSAGES == GLM_MESSAGES_ENABLED && !defined(GLM_EXT_INCLUDED)
#	pragma message("GLM: GLM_GTX_color_space extension included")
#endif

namespace glm
{
	/// @addtogroup gtx_color_space
	/// @{

	/// Converts a color from HSV color space to its color in RGB color space.
	/// @see gtx_color_space
	template<typename T, precision P>
	GLM_FUNC_DECL vec<3, T, P> rgbColor(
		vec<3, T, P> const & hsvValue);

	/// Converts a color from RGB color space to its color in HSV color space.
	/// @see gtx_color_space
	template<typename T, precision P>
	GLM_FUNC_DECL vec<3, T, P> hsvColor(
		vec<3, T, P> const & rgbValue);
		
	/// Build a saturation matrix.
	/// @see gtx_color_space
	template<typename T>
	GLM_FUNC_DECL mat<4, 4, T, defaultp> saturation(
		T const s);

	/// Modify the saturation of a color.
	/// @see gtx_color_space
	template<typename T, precision P>
	GLM_FUNC_DECL vec<3, T, P> saturation(
		T const s,
		vec<3, T, P> const & color);
		
	/// Modify the saturation of a color.
	/// @see gtx_color_space
	template<typename T, precision P>
	GLM_FUNC_DECL vec<4, T, P> saturation(
		T const s,
		vec<4, T, P> const & color);
		
	/// Compute color luminosity associating ratios (0.33, 0.59, 0.11) to RGB canals.
	/// @see gtx_color_space
	template<typename T, precision P>
	GLM_FUNC_DECL T luminosity(
		vec<3, T, P> const & color);

	/// @}
}//namespace glm

#include "color_space.inl"
