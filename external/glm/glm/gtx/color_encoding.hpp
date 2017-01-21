/// @ref gtx_color_encoding
/// @file glm/gtx/color_encoding.hpp
///
/// @see core (dependence)
/// @see gtx_color_encoding (dependence)
///
/// @defgroup gtx_color_encoding GLM_GTX_color_encoding
/// @ingroup gtx
///
/// @brief Allow to perform bit operations on integer values
///
/// <glm/gtc/color_encoding.hpp> need to be included to use these functionalities.

#pragma once

// Dependencies
#include "../detail/setup.hpp"
#include "../detail/precision.hpp"
#include "../vec3.hpp"
#include <limits>

#if GLM_MESSAGES == GLM_MESSAGES_ENABLED && !defined(GLM_EXT_INCLUDED)
#	pragma message("GLM: GLM_GTC_color_encoding extension included")
#endif

namespace glm
{
	/// @addtogroup gtx_color_encoding
	/// @{

	/// Convert a linear sRGB color to D65 YUV.
	template<typename T, precision P>
	GLM_FUNC_DECL vec<3, T, P> convertLinearSRGBToD65XYZ(vec<3, T, P> const& ColorLinearSRGB);

	/// Convert a linear sRGB color to D50 YUV.
	template<typename T, precision P>
	GLM_FUNC_DECL vec<3, T, P> convertLinearSRGBToD50XYZ(vec<3, T, P> const& ColorLinearSRGB);

	/// Convert a D65 YUV color to linear sRGB.
	template<typename T, precision P>
	GLM_FUNC_DECL vec<3, T, P> convertD65XYZToLinearSRGB(vec<3, T, P> const& ColorD65XYZ);

	/// Convert a D65 YUV color to D50 YUV.
	template<typename T, precision P>
	GLM_FUNC_DECL vec<3, T, P> convertD65XYZToD50XYZ(vec<3, T, P> const& ColorD65XYZ);

	/// @}
} //namespace glm

#include "color_encoding.inl"
