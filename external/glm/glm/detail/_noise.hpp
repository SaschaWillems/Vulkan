/// @ref core
/// @file glm/detail/_noise.hpp

#pragma once

#include "../vec2.hpp"
#include "../vec3.hpp"
#include "../vec4.hpp"
#include "../common.hpp"

namespace glm{
namespace detail
{
	template<typename T>
	GLM_FUNC_QUALIFIER T mod289(T const & x)
	{
		return x - floor(x * (static_cast<T>(1.0) / static_cast<T>(289.0))) * static_cast<T>(289.0);
	}

	template<typename T>
	GLM_FUNC_QUALIFIER T permute(T const & x)
	{
		return mod289(((x * static_cast<T>(34)) + static_cast<T>(1)) * x);
	}

	template<typename T, precision P>
	GLM_FUNC_QUALIFIER vec<2, T, P> permute(vec<2, T, P> const & x)
	{
		return mod289(((x * static_cast<T>(34)) + static_cast<T>(1)) * x);
	}
	
	template<typename T, precision P>
	GLM_FUNC_QUALIFIER vec<3, T, P> permute(vec<3, T, P> const & x)
	{
		return mod289(((x * static_cast<T>(34)) + static_cast<T>(1)) * x);
	}
	
	template<typename T, precision P>
	GLM_FUNC_QUALIFIER vec<4, T, P> permute(vec<4, T, P> const & x)
	{
		return mod289(((x * static_cast<T>(34)) + static_cast<T>(1)) * x);
	}
/*
	template<typename T, precision P, template<typename> class vecType>
	GLM_FUNC_QUALIFIER vecType<L, T, P> permute(vecType<L, T, P> const & x)
	{
		return mod289(((x * T(34)) + T(1)) * x);
	}
*/
	template<typename T>
	GLM_FUNC_QUALIFIER T taylorInvSqrt(T const & r)
	{
		return T(1.79284291400159) - T(0.85373472095314) * r;
	}
	
	template<typename T, precision P>
	GLM_FUNC_QUALIFIER vec<2, T, P> taylorInvSqrt(vec<2, T, P> const & r)
	{
		return T(1.79284291400159) - T(0.85373472095314) * r;
	}
	
	template<typename T, precision P>
	GLM_FUNC_QUALIFIER vec<3, T, P> taylorInvSqrt(vec<3, T, P> const & r)
	{
		return T(1.79284291400159) - T(0.85373472095314) * r;
	}
	
	template<typename T, precision P>
	GLM_FUNC_QUALIFIER vec<4, T, P> taylorInvSqrt(vec<4, T, P> const & r)
	{
		return T(1.79284291400159) - T(0.85373472095314) * r;
	}
/*
	template<typename T, precision P, template<typename> class vecType>
	GLM_FUNC_QUALIFIER vecType<L, T, P> taylorInvSqrt(vecType<L, T, P> const & r)
	{
		return T(1.79284291400159) - T(0.85373472095314) * r;
	}
*/
	
	template<typename T, precision P>
	GLM_FUNC_QUALIFIER vec<2, T, P> fade(vec<2, T, P> const & t)
	{
		return (t * t * t) * (t * (t * T(6) - T(15)) + T(10));
	}
	
	template<typename T, precision P>
	GLM_FUNC_QUALIFIER vec<3, T, P> fade(vec<3, T, P> const & t)
	{
		return (t * t * t) * (t * (t * T(6) - T(15)) + T(10));
	}
	
	template<typename T, precision P>
	GLM_FUNC_QUALIFIER vec<4, T, P> fade(vec<4, T, P> const & t)
	{
		return (t * t * t) * (t * (t * T(6) - T(15)) + T(10));
	}
/*
	template<typename T, precision P, template<typename> class vecType>
	GLM_FUNC_QUALIFIER vecType<L, T, P> fade(vecType<L, T, P> const & t)
	{
		return (t * t * t) * (t * (t * T(6) - T(15)) + T(10));
	}
*/
}//namespace detail
}//namespace glm

