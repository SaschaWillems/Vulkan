/// @ref gtc_round
/// @file glm/gtc/round.hpp
///
/// @see core (dependence)
/// @see gtc_round (dependence)
///
/// @defgroup gtc_round GLM_GTC_round
/// @ingroup gtc
///
/// @brief rounding value to specific boundings
///
/// <glm/gtc/round.hpp> need to be included to use these functionalities.

#pragma once

// Dependencies
#include "../detail/setup.hpp"
#include "../detail/precision.hpp"
#include "../detail/_vectorize.hpp"
#include "../vector_relational.hpp"
#include "../common.hpp"
#include <limits>

#if GLM_MESSAGES == GLM_MESSAGES_ENABLED && !defined(GLM_EXT_INCLUDED)
#	pragma message("GLM: GLM_GTC_integer extension included")
#endif

namespace glm
{
	/// @addtogroup gtc_round
	/// @{

	/// Return true if the value is a power of two number.
	///
	/// @see gtc_round
	template<typename genIUType>
	GLM_FUNC_DECL bool isPowerOfTwo(genIUType Value);

	/// Return true if the value is a power of two number.
	///
	/// @see gtc_round
	template<length_t L, typename T, precision P, template<length_t, typename, precision> class vecType>
	GLM_FUNC_DECL vecType<L, bool, P> isPowerOfTwo(vecType<L, T, P> const & value);

	/// Return the power of two number which value is just higher the input value,
	/// round up to a power of two.
	///
	/// @see gtc_round
	template<typename genIUType>
	GLM_FUNC_DECL genIUType ceilPowerOfTwo(genIUType Value);

	/// Return the power of two number which value is just higher the input value,
	/// round up to a power of two.
	///
	/// @see gtc_round
	template<length_t L, typename T, precision P, template<length_t, typename, precision> class vecType>
	GLM_FUNC_DECL vecType<L, T, P> ceilPowerOfTwo(vecType<L, T, P> const & value);

	/// Return the power of two number which value is just lower the input value,
	/// round down to a power of two.
	///
	/// @see gtc_round
	template<typename genIUType>
	GLM_FUNC_DECL genIUType floorPowerOfTwo(genIUType Value);

	/// Return the power of two number which value is just lower the input value,
	/// round down to a power of two.
	///
	/// @see gtc_round
	template<length_t L, typename T, precision P, template<length_t, typename, precision> class vecType>
	GLM_FUNC_DECL vecType<L, T, P> floorPowerOfTwo(vecType<L, T, P> const & value);

	/// Return the power of two number which value is the closet to the input value.
	///
	/// @see gtc_round
	template<typename genIUType>
	GLM_FUNC_DECL genIUType roundPowerOfTwo(genIUType Value);

	/// Return the power of two number which value is the closet to the input value.
	///
	/// @see gtc_round
	template<length_t L, typename T, precision P, template<length_t, typename, precision> class vecType>
	GLM_FUNC_DECL vecType<L, T, P> roundPowerOfTwo(vecType<L, T, P> const & value);

	/// Return true if the 'Value' is a multiple of 'Multiple'.
	///
	/// @see gtc_round
	template<typename genIUType>
	GLM_FUNC_DECL bool isMultiple(genIUType Value, genIUType Multiple);

	/// Return true if the 'Value' is a multiple of 'Multiple'.
	///
	/// @see gtc_round
	template<length_t L, typename T, precision P, template<length_t, typename, precision> class vecType>
	GLM_FUNC_DECL vecType<L, bool, P> isMultiple(vecType<L, T, P> const & Value, T Multiple);

	/// Return true if the 'Value' is a multiple of 'Multiple'.
	///
	/// @see gtc_round
	template<length_t L, typename T, precision P, template<length_t, typename, precision> class vecType>
	GLM_FUNC_DECL vecType<L, bool, P> isMultiple(vecType<L, T, P> const & Value, vecType<L, T, P> const & Multiple);

	/// Higher multiple number of Source.
	///
	/// @tparam genType Floating-point or integer scalar or vector types.
	/// @param Source 
	/// @param Multiple Must be a null or positive value
	///
	/// @see gtc_round
	template<typename genType>
	GLM_FUNC_DECL genType ceilMultiple(genType Source, genType Multiple);

	/// Higher multiple number of Source.
	///
	/// @tparam genType Floating-point or integer scalar or vector types.
	/// @param Source 
	/// @param Multiple Must be a null or positive value
	///
	/// @see gtc_round
	template<length_t L, typename T, precision P, template<length_t, typename, precision> class vecType>
	GLM_FUNC_DECL vecType<L, T, P> ceilMultiple(vecType<L, T, P> const & Source, vecType<L, T, P> const & Multiple);

	/// Lower multiple number of Source.
	///
	/// @tparam genType Floating-point or integer scalar or vector types.
	/// @param Source 
	/// @param Multiple Must be a null or positive value
	///
	/// @see gtc_round
	template<typename genType>
	GLM_FUNC_DECL genType floorMultiple(
		genType Source,
		genType Multiple);

	/// Lower multiple number of Source.
	///
	/// @tparam genType Floating-point or integer scalar or vector types.
	/// @param Source 
	/// @param Multiple Must be a null or positive value
	///
	/// @see gtc_round
	template<length_t L, typename T, precision P, template<length_t, typename, precision> class vecType>
	GLM_FUNC_DECL vecType<L, T, P> floorMultiple(
		vecType<L, T, P> const& Source,
		vecType<L, T, P> const& Multiple);

	/// Lower multiple number of Source.
	///
	/// @tparam genType Floating-point or integer scalar or vector types.
	/// @param Source 
	/// @param Multiple Must be a null or positive value
	///
	/// @see gtc_round
	template<typename genType>
	GLM_FUNC_DECL genType roundMultiple(
		genType Source,
		genType Multiple);

	/// Lower multiple number of Source.
	///
	/// @tparam genType Floating-point or integer scalar or vector types.
	/// @param Source 
	/// @param Multiple Must be a null or positive value
	///
	/// @see gtc_round
	template<length_t L, typename T, precision P, template<length_t, typename, precision> class vecType>
	GLM_FUNC_DECL vecType<L, T, P> roundMultiple(
		vecType<L, T, P> const& Source,
		vecType<L, T, P> const& Multiple);

	/// @}
} //namespace glm

#include "round.inl"
