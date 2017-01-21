/// @ref gtx_fast_trigonometry
/// @file glm/gtx/fast_trigonometry.inl

namespace glm{
namespace detail
{
	template<length_t L, typename T, precision P, template<length_t, typename, precision> class vecType>
	GLM_FUNC_QUALIFIER vecType<L, T, P> taylorCos(vecType<L, T, P> const & x)
	{
		return static_cast<T>(1)
			- (x * x) * (1.f / 2.f)
			+ ((x * x) * (x * x)) * (1.f / 24.f)
			- (((x * x) * (x * x)) * (x * x)) * (1.f / 720.f)
			+ (((x * x) * (x * x)) * ((x * x) * (x * x))) * (1.f / 40320.f);
	}

	template<typename T>
	GLM_FUNC_QUALIFIER T cos_52s(T x)
	{
		T const xx(x * x);
		return (T(0.9999932946) + xx * (T(-0.4999124376) + xx * (T(0.0414877472) + xx * T(-0.0012712095))));
	}

	template<length_t L, typename T, precision P, template<length_t, typename, precision> class vecType>
	GLM_FUNC_QUALIFIER vecType<L, T, P> cos_52s(vecType<L, T, P> const & x)
	{
		return detail::functor1<L, T, T, P>::call(cos_52s, x);
	}
}//namespace detail

	// wrapAngle
	template<typename T>
	GLM_FUNC_QUALIFIER T wrapAngle(T angle)
	{
		return abs<T>(mod<T>(angle, two_pi<T>()));
	}

	template<length_t L, typename T, precision P, template<length_t, typename, precision> class vecType>
	GLM_FUNC_QUALIFIER vecType<L, T, P> wrapAngle(vecType<L, T, P> const & x)
	{
		return detail::functor1<L, T, T, P>::call(wrapAngle, x);
	}

	// cos
	template<typename T> 
	GLM_FUNC_QUALIFIER T fastCos(T x)
	{
		T const angle(wrapAngle<T>(x));

		if(angle < half_pi<T>())
			return detail::cos_52s(angle);
		if(angle < pi<T>())
			return -detail::cos_52s(pi<T>() - angle);
		if(angle < (T(3) * half_pi<T>()))
			return -detail::cos_52s(angle - pi<T>());

		return detail::cos_52s(two_pi<T>() - angle);
	}

	template<length_t L, typename T, precision P, template<length_t, typename, precision> class vecType>
	GLM_FUNC_QUALIFIER vecType<L, T, P> fastCos(vecType<L, T, P> const & x)
	{
		return detail::functor1<L, T, T, P>::call(fastCos, x);
	}

	// sin
	template<typename T> 
	GLM_FUNC_QUALIFIER T fastSin(T x)
	{
		return fastCos<T>(half_pi<T>() - x);
	}

	template<length_t L, typename T, precision P, template<length_t, typename, precision> class vecType>
	GLM_FUNC_QUALIFIER vecType<L, T, P> fastSin(vecType<L, T, P> const & x)
	{
		return detail::functor1<L, T, T, P>::call(fastSin, x);
	}

	// tan
	template<typename T> 
	GLM_FUNC_QUALIFIER T fastTan(T x)
	{
		return x + (x * x * x * T(0.3333333333)) + (x * x * x * x * x * T(0.1333333333333)) + (x * x * x * x * x * x * x * T(0.0539682539));
	}

	template<length_t L, typename T, precision P, template<length_t, typename, precision> class vecType>
	GLM_FUNC_QUALIFIER vecType<L, T, P> fastTan(vecType<L, T, P> const & x)
	{
		return detail::functor1<L, T, T, P>::call(fastTan, x);
	}

	// asin
	template<typename T> 
	GLM_FUNC_QUALIFIER T fastAsin(T x)
	{
		return x + (x * x * x * T(0.166666667)) + (x * x * x * x * x * T(0.075)) + (x * x * x * x * x * x * x * T(0.0446428571)) + (x * x * x * x * x * x * x * x * x * T(0.0303819444));// + (x * x * x * x * x * x * x * x * x * x * x * T(0.022372159));
	}

	template<length_t L, typename T, precision P, template<length_t, typename, precision> class vecType>
	GLM_FUNC_QUALIFIER vecType<L, T, P> fastAsin(vecType<L, T, P> const & x)
	{
		return detail::functor1<L, T, T, P>::call(fastAsin, x);
	}

	// acos
	template<typename T> 
	GLM_FUNC_QUALIFIER T fastAcos(T x)
	{
		return T(1.5707963267948966192313216916398) - fastAsin(x); //(PI / 2)
	}

	template<length_t L, typename T, precision P, template<length_t, typename, precision> class vecType>
	GLM_FUNC_QUALIFIER vecType<L, T, P> fastAcos(vecType<L, T, P> const & x)
	{
		return detail::functor1<L, T, T, P>::call(fastAcos, x);
	}

	// atan
	template<typename T> 
	GLM_FUNC_QUALIFIER T fastAtan(T y, T x)
	{
		T sgn = sign(y) * sign(x);
		return abs(fastAtan(y / x)) * sgn;
	}

	template<length_t L, typename T, precision P, template<length_t, typename, precision> class vecType>
	GLM_FUNC_QUALIFIER vecType<L, T, P> fastAtan(vecType<L, T, P> const & y, vecType<L, T, P> const & x)
	{
		return detail::functor2<L, T, P>::call(fastAtan, y, x);
	}

	template<typename T> 
	GLM_FUNC_QUALIFIER T fastAtan(T x)
	{
		return x - (x * x * x * T(0.333333333333)) + (x * x * x * x * x * T(0.2)) - (x * x * x * x * x * x * x * T(0.1428571429)) + (x * x * x * x * x * x * x * x * x * T(0.111111111111)) - (x * x * x * x * x * x * x * x * x * x * x * T(0.0909090909));
	}

	template<length_t L, typename T, precision P, template<length_t, typename, precision> class vecType>
	GLM_FUNC_QUALIFIER vecType<L, T, P> fastAtan(vecType<L, T, P> const & x)
	{
		return detail::functor1<L, T, T, P>::call(fastAtan, x);
	}
}//namespace glm
