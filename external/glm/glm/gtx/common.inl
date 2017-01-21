/// @ref gtx_common
/// @file glm/gtx/common.inl

#include <cmath>

namespace glm{
namespace detail
{
	template<length_t L, typename T, precision P, template<length_t, typename, precision> class vecType, bool isFloat = true>
	struct compute_fmod
	{
		GLM_FUNC_QUALIFIER static vecType<L, T, P> call(vecType<L, T, P> const & a, vecType<L, T, P> const & b)
		{
			return detail::functor2<L, T, P>::call(std::fmod, a, b);
		}
	};

	template<length_t L, typename T, precision P, template<length_t, typename, precision> class vecType>
	struct compute_fmod<L, T, P, vecType, false>
	{
		GLM_FUNC_QUALIFIER static vecType<L, T, P> call(vecType<L, T, P> const & a, vecType<L, T, P> const & b)
		{
			return a % b;
		}
	};
}//namespace detail

	template<typename T> 
	GLM_FUNC_QUALIFIER bool isdenormal(T const & x)
	{
		GLM_STATIC_ASSERT(std::numeric_limits<T>::is_iec559, "'isdenormal' only accept floating-point inputs");

#		if GLM_HAS_CXX11_STL
			return std::fpclassify(x) == FP_SUBNORMAL;
#		else
			return x != static_cast<T>(0) && std::fabs(x) < std::numeric_limits<T>::min();
#		endif
	}

	template<typename T, precision P>
	GLM_FUNC_QUALIFIER typename vec<1, T, P>::bool_type isdenormal
	(
		vec<1, T, P> const & x
	)
	{
		GLM_STATIC_ASSERT(std::numeric_limits<T>::is_iec559, "'isdenormal' only accept floating-point inputs");

		return typename vec<1, T, P>::bool_type(
			isdenormal(x.x));
	}

	template<typename T, precision P>
	GLM_FUNC_QUALIFIER typename vec<2, T, P>::bool_type isdenormal
	(
		vec<2, T, P> const & x
	)
	{
		GLM_STATIC_ASSERT(std::numeric_limits<T>::is_iec559, "'isdenormal' only accept floating-point inputs");

		return typename vec<2, T, P>::bool_type(
			isdenormal(x.x),
			isdenormal(x.y));
	}

	template<typename T, precision P>
	GLM_FUNC_QUALIFIER typename vec<3, T, P>::bool_type isdenormal
	(
		vec<3, T, P> const & x
	)
	{
		GLM_STATIC_ASSERT(std::numeric_limits<T>::is_iec559, "'isdenormal' only accept floating-point inputs");

		return typename vec<3, T, P>::bool_type(
			isdenormal(x.x),
			isdenormal(x.y),
			isdenormal(x.z));
	}

	template<typename T, precision P>
	GLM_FUNC_QUALIFIER typename vec<4, T, P>::bool_type isdenormal
	(
		vec<4, T, P> const & x
	)
	{
		GLM_STATIC_ASSERT(std::numeric_limits<T>::is_iec559, "'isdenormal' only accept floating-point inputs");

		return typename vec<4, T, P>::bool_type(
			isdenormal(x.x),
			isdenormal(x.y),
			isdenormal(x.z),
			isdenormal(x.w));
	}

	// fmod
	template<typename genType>
	GLM_FUNC_QUALIFIER genType fmod(genType x, genType y)
	{
		return fmod(vec<1, genType>(x), y).x;
	}

	template<length_t L, typename T, precision P, template<length_t, typename, precision> class vecType>
	GLM_FUNC_QUALIFIER vecType<L, T, P> fmod(vecType<L, T, P> const & x, T y)
	{
		return detail::compute_fmod<L, T, P, vecType, std::numeric_limits<T>::is_iec559>::call(x, vecType<L, T, P>(y));
	}

	template<length_t L, typename T, precision P, template<length_t, typename, precision> class vecType>
	GLM_FUNC_QUALIFIER vecType<L, T, P> fmod(vecType<L, T, P> const & x, vecType<L, T, P> const & y)
	{
		return detail::compute_fmod<L, T, P, vecType, std::numeric_limits<T>::is_iec559>::call(x, y);
	}
}//namespace glm
