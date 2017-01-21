/// @ref gtx_log_base
/// @file glm/gtx/log_base.hpp
///
/// @see core (dependence)
///
/// @defgroup gtx_log_base GLM_GTX_log_base
/// @ingroup gtx
///
/// @brief Logarithm for any base. base can be a vector or a scalar.
///
/// <glm/gtx/log_base.hpp> need to be included to use these functionalities.

#pragma once

// Dependency:
#include "../glm.hpp"

#ifndef GLM_ENABLE_EXPERIMENTAL
#	error "GLM: GLM_GTX_log_base is an experimental extension and may change in the future. Use #define GLM_ENABLE_EXPERIMENTAL before including it, if you really want to use it."
#endif

#if GLM_MESSAGES == GLM_MESSAGES_ENABLED && !defined(GLM_EXT_INCLUDED)
#	pragma message("GLM: GLM_GTX_log_base extension included")
#endif

namespace glm
{
	/// @addtogroup gtx_log_base
	/// @{

	/// Logarithm for any base.
	/// From GLM_GTX_log_base.
	template<typename genType>
	GLM_FUNC_DECL genType log(
		genType const & x,
		genType const & base);

	/// Logarithm for any base.
	/// From GLM_GTX_log_base.
	template<length_t L, typename T, precision P, template<length_t, typename, precision> class vecType>
	GLM_FUNC_DECL vecType<L, T, P> sign(
		vecType<L, T, P> const& x,
		vecType<L, T, P> const& base);

	/// @}
}//namespace glm

#include "log_base.inl"
