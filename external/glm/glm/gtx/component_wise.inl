///////////////////////////////////////////////////////////////////////////////////
/// OpenGL Mathematics (glm.g-truc.net)
///
/// Copyright (c) 2005 - 2015 G-Truc Creation (www.g-truc.net)
/// Permission is hereby granted, free of charge, to any person obtaining a copy
/// of this software and associated documentation files (the "Software"), to deal
/// in the Software without restriction, including without limitation the rights
/// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
/// copies of the Software, and to permit persons to whom the Software is
/// furnished to do so, subject to the following conditions:
/// 
/// The above copyright notice and this permission notice shall be included in
/// all copies or substantial portions of the Software.
/// 
/// Restrictions:
///		By making use of the Software for military purposes, you choose to make
///		a Bunny unhappy.
/// 
/// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
/// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
/// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
/// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
/// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
/// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
/// THE SOFTWARE.
///
/// @ref gtx_component_wise
/// @file glm/gtx/component_wise.inl
/// @date 2007-05-21 / 2011-06-07
/// @author Christophe Riccio
///////////////////////////////////////////////////////////////////////////////////////////////////

#include <limits>

namespace glm{
namespace detail
{
	template <typename T, typename floatType, precision P, template <typename, precision> class vecType, bool isInteger, bool signedType>
	struct compute_compNormalize
	{};

	template <typename T, typename floatType, precision P, template <typename, precision> class vecType>
	struct compute_compNormalize<T, floatType, P, vecType, true, true>
	{
		GLM_FUNC_QUALIFIER static vecType<floatType, P> call(vecType<T, P> const & v)
		{
			floatType const Min = static_cast<floatType>(std::numeric_limits<T>::min());
			floatType const Max = static_cast<floatType>(std::numeric_limits<T>::max());
			return (vecType<floatType, P>(v) - Min) / (Max - Min) * static_cast<floatType>(2) - static_cast<floatType>(1);
		}
	};

	template <typename T, typename floatType, precision P, template <typename, precision> class vecType>
	struct compute_compNormalize<T, floatType, P, vecType, true, false>
	{
		GLM_FUNC_QUALIFIER static vecType<floatType, P> call(vecType<T, P> const & v)
		{
			return vecType<floatType, P>(v) / static_cast<floatType>(std::numeric_limits<T>::max());
		}
	};

	template <typename T, typename floatType, precision P, template <typename, precision> class vecType>
	struct compute_compNormalize<T, floatType, P, vecType, false, true>
	{
		GLM_FUNC_QUALIFIER static vecType<floatType, P> call(vecType<T, P> const & v)
		{
			return v;
		}
	};

	template <typename T, typename floatType, precision P, template <typename, precision> class vecType, bool isInteger, bool signedType>
	struct compute_compScale
	{};

	template <typename T, typename floatType, precision P, template <typename, precision> class vecType>
	struct compute_compScale<T, floatType, P, vecType, true, true>
	{
		GLM_FUNC_QUALIFIER static vecType<T, P> call(vecType<floatType, P> const & v)
		{
			floatType const Max = static_cast<floatType>(std::numeric_limits<T>::max()) + static_cast<floatType>(0.5);
			vecType<floatType, P> const Scaled(v * Max);
			vecType<T, P> const Result(Scaled - static_cast<floatType>(0.5));
			return Result;
		}
	};

	template <typename T, typename floatType, precision P, template <typename, precision> class vecType>
	struct compute_compScale<T, floatType, P, vecType, true, false>
	{
		GLM_FUNC_QUALIFIER static vecType<T, P> call(vecType<floatType, P> const & v)
		{
			return vecType<T, P>(vecType<floatType, P>(v) * static_cast<floatType>(std::numeric_limits<T>::max()));
		}
	};

	template <typename T, typename floatType, precision P, template <typename, precision> class vecType>
	struct compute_compScale<T, floatType, P, vecType, false, true>
	{
		GLM_FUNC_QUALIFIER static vecType<T, P> call(vecType<floatType, P> const & v)
		{
			return v;
		}
	};
}//namespace detail

	template <typename floatType, typename T, precision P, template <typename, precision> class vecType>
	GLM_FUNC_QUALIFIER vecType<floatType, P> compNormalize(vecType<T, P> const & v)
	{
		GLM_STATIC_ASSERT(std::numeric_limits<floatType>::is_iec559, "'compNormalize' accepts only floating-point types for 'floatType' template parameter");

		return detail::compute_compNormalize<T, floatType, P, vecType, std::numeric_limits<T>::is_integer, std::numeric_limits<T>::is_signed>::call(v);
	}

	template <typename T, typename floatType, precision P, template <typename, precision> class vecType>
	GLM_FUNC_QUALIFIER vecType<T, P> compScale(vecType<floatType, P> const & v)
	{
		GLM_STATIC_ASSERT(std::numeric_limits<floatType>::is_iec559, "'compScale' accepts only floating-point types for 'floatType' template parameter");

		return detail::compute_compScale<T, floatType, P, vecType, std::numeric_limits<T>::is_integer, std::numeric_limits<T>::is_signed>::call(v);
	}

	template <typename T, precision P, template <typename, precision> class vecType>
	GLM_FUNC_QUALIFIER T compAdd(vecType<T, P> const & v)
	{
		T result(0);
		for(detail::component_count_t i = 0; i < detail::component_count(v); ++i)
			result += v[i];
		return result;
	}

	template <typename T, precision P, template <typename, precision> class vecType>
	GLM_FUNC_QUALIFIER T compMul(vecType<T, P> const & v)
	{
		T result(1);
		for(detail::component_count_t i = 0; i < detail::component_count(v); ++i)
			result *= v[i];
		return result;
	}

	template <typename T, precision P, template <typename, precision> class vecType>
	GLM_FUNC_QUALIFIER T compMin(vecType<T, P> const & v)
	{
		T result(v[0]);
		for(detail::component_count_t i = 1; i < detail::component_count(v); ++i)
			result = min(result, v[i]);
		return result;
	}

	template <typename T, precision P, template <typename, precision> class vecType>
	GLM_FUNC_QUALIFIER T compMax(vecType<T, P> const & v)
	{
		T result(v[0]);
		for(detail::component_count_t i = 1; i < detail::component_count(v); ++i)
			result = max(result, v[i]);
		return result;
	}
}//namespace glm
