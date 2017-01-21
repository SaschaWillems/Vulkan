/// @ref gtx_component_wise
/// @file glm/gtx/component_wise.hpp
/// @date 2007-05-21 / 2011-06-07
/// @author Christophe Riccio
/// 
/// @see core (dependence)
///
/// @defgroup gtx_component_wise GLM_GTX_component_wise
/// @ingroup gtx
///
/// @brief Operations between components of a type
///
/// <glm/gtx/component_wise.hpp> need to be included to use these functionalities.

#pragma once

// Dependencies
#include "../detail/setup.hpp"
#include "../detail/precision.hpp"

#ifndef GLM_ENABLE_EXPERIMENTAL
#	error "GLM: GLM_GTX_component_wise is an experimental extension and may change in the future. Use #define GLM_ENABLE_EXPERIMENTAL before including it, if you really want to use it."
#endif

#if GLM_MESSAGES == GLM_MESSAGES_ENABLED && !defined(GLM_EXT_INCLUDED)
#	pragma message("GLM: GLM_GTX_component_wise extension included")
#endif

namespace glm
{
	/// @addtogroup gtx_component_wise
	/// @{

	/// Convert an integer vector to a normalized float vector.
	/// If the parameter value type is already a floating precision type, the value is passed through.
	/// @see gtx_component_wise
	template<typename floatType, length_t L, typename T, precision P, template<length_t, typename, precision> class vecType>
	GLM_FUNC_DECL vecType<L, floatType, P> compNormalize(vecType<L, T, P> const & v);

	/// Convert a normalized float vector to an integer vector.
	/// If the parameter value type is already a floating precision type, the value is passed through.
	/// @see gtx_component_wise
	template<length_t L, typename T, typename floatType, precision P, template<length_t, typename, precision> class vecType>
	GLM_FUNC_DECL vecType<L, T, P> compScale(vecType<L, floatType, P> const & v);

	/// Add all vector components together. 
	/// @see gtx_component_wise
	template<typename genType> 
	GLM_FUNC_DECL typename genType::value_type compAdd(genType const & v);

	/// Multiply all vector components together. 
	/// @see gtx_component_wise
	template<typename genType> 
	GLM_FUNC_DECL typename genType::value_type compMul(genType const & v);

	/// Find the minimum value between single vector components.
	/// @see gtx_component_wise
	template<typename genType> 
	GLM_FUNC_DECL typename genType::value_type compMin(genType const & v);

	/// Find the maximum value between single vector components.
	/// @see gtx_component_wise
	template<typename genType> 
	GLM_FUNC_DECL typename genType::value_type compMax(genType const & v);

	/// @}
}//namespace glm

#include "component_wise.inl"
