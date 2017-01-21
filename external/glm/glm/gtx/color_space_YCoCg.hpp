/// @ref gtx_color_space_YCoCg
/// @file glm/gtx/color_space_YCoCg.hpp
///
/// @see core (dependence)
///
/// @defgroup gtx_color_space_YCoCg GLM_GTX_color_space_YCoCg
/// @ingroup gtx
///
/// @brief RGB to YCoCg conversions and operations
///
/// <glm/gtx/color_space_YCoCg.hpp> need to be included to use these functionalities.

#pragma once

// Dependency:
#include "../glm.hpp"

#ifndef GLM_ENABLE_EXPERIMENTAL
#	error "GLM: GLM_GTX_color_space_YCoCg is an experimental extension and may change in the future. Use #define GLM_ENABLE_EXPERIMENTAL before including it, if you really want to use it."
#endif

#if GLM_MESSAGES == GLM_MESSAGES_ENABLED && !defined(GLM_EXT_INCLUDED)
#	pragma message("GLM: GLM_GTX_color_space_YCoCg extension included")
#endif

namespace glm
{
	/// @addtogroup gtx_color_space_YCoCg
	/// @{

	/// Convert a color from RGB color space to YCoCg color space.
	/// @see gtx_color_space_YCoCg
	template<typename T, precision P>
	GLM_FUNC_DECL vec<3, T, P> rgb2YCoCg(
		vec<3, T, P> const & rgbColor);

	/// Convert a color from YCoCg color space to RGB color space.
	/// @see gtx_color_space_YCoCg
	template<typename T, precision P>
	GLM_FUNC_DECL vec<3, T, P> YCoCg2rgb(
		vec<3, T, P> const & YCoCgColor);

	/// Convert a color from RGB color space to YCoCgR color space.
	/// @see "YCoCg-R: A Color Space with RGB Reversibility and Low Dynamic Range"
	/// @see gtx_color_space_YCoCg
	template<typename T, precision P>
	GLM_FUNC_DECL vec<3, T, P> rgb2YCoCgR(
		vec<3, T, P> const & rgbColor);

	/// Convert a color from YCoCgR color space to RGB color space.
	/// @see "YCoCg-R: A Color Space with RGB Reversibility and Low Dynamic Range"
	/// @see gtx_color_space_YCoCg
	template<typename T, precision P>
	GLM_FUNC_DECL vec<3, T, P> YCoCgR2rgb(
		vec<3, T, P> const & YCoCgColor);

	/// @}
}//namespace glm

#include "color_space_YCoCg.inl"
