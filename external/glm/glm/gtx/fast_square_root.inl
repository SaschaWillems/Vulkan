/// @ref gtx_fast_square_root
/// @file glm/gtx/fast_square_root.inl

namespace glm
{
	// fastSqrt
	template<typename genType>
	GLM_FUNC_QUALIFIER genType fastSqrt(genType x)
	{
		GLM_STATIC_ASSERT(std::numeric_limits<genType>::is_iec559, "'fastSqrt' only accept floating-point input");

		return genType(1) / fastInverseSqrt(x);
	}

	template<length_t L, typename T, precision P, template<length_t, typename, precision> class vecType>
	GLM_FUNC_QUALIFIER vecType<L, T, P> fastSqrt(vecType<L, T, P> const & x)
	{
		return detail::functor1<L, T, T, P>::call(fastSqrt, x);
	}

	// fastInversesqrt
	template<typename genType>
	GLM_FUNC_QUALIFIER genType fastInverseSqrt(genType x)
	{
#		ifdef __CUDACC__ // Wordaround for a CUDA compiler bug up to CUDA6
			vec<1, T, P> tmp(detail::compute_inversesqrt<tvec1, genType, lowp, detail::is_aligned<lowp>::value>::call(vec<1, genType, lowp>(x)));
			return tmp.x;
#		else
			return detail::compute_inversesqrt<1, genType, highp, detail::is_aligned<highp>::value>::call(vec<1, genType, lowp>(x)).x;
#		endif
	}

	template<length_t L, typename T, precision P, template<length_t, typename, precision> class vecType>
	GLM_FUNC_QUALIFIER vecType<L, T, P> fastInverseSqrt(vecType<L, T, P> const & x)
	{
		return detail::compute_inversesqrt<L, T, P, detail::is_aligned<P>::value>::call(x);
	}

	// fastLength
	template<typename genType>
	GLM_FUNC_QUALIFIER genType fastLength(genType x)
	{
		GLM_STATIC_ASSERT(std::numeric_limits<genType>::is_iec559, "'fastLength' only accept floating-point inputs");

		return abs(x);
	}

	template<length_t L, typename T, precision P, template<length_t, typename, precision> class vecType>
	GLM_FUNC_QUALIFIER T fastLength(vecType<L, T, P> const & x)
	{
		GLM_STATIC_ASSERT(std::numeric_limits<T>::is_iec559, "'fastLength' only accept floating-point inputs");

		return fastSqrt(dot(x, x));
	}

	// fastDistance
	template<typename genType>
	GLM_FUNC_QUALIFIER genType fastDistance(genType x, genType y)
	{
		return fastLength(y - x);
	}

	template<length_t L, typename T, precision P, template<length_t, typename, precision> class vecType>
	GLM_FUNC_QUALIFIER T fastDistance(vecType<L, T, P> const & x, vecType<L, T, P> const & y)
	{
		return fastLength(y - x);
	}

	// fastNormalize
	template<typename genType>
	GLM_FUNC_QUALIFIER genType fastNormalize(genType x)
	{
		return x > genType(0) ? genType(1) : -genType(1);
	}

	template<length_t L, typename T, precision P, template<length_t, typename, precision> class vecType>
	GLM_FUNC_QUALIFIER vecType<L, T, P> fastNormalize(vecType<L, T, P> const & x)
	{
		return x * fastInverseSqrt(dot(x, x));
	}
}//namespace glm
