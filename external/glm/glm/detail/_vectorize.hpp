/// @ref core
/// @file glm/detail/_vectorize.hpp

#pragma once

#include "type_vec1.hpp"
#include "type_vec2.hpp"
#include "type_vec3.hpp"
#include "type_vec4.hpp"

namespace glm{
namespace detail
{
	template<length_t L, typename R, typename T, precision P>
	struct functor1{};

	template<typename R, typename T, precision P>
	struct functor1<1, R, T, P>
	{
		GLM_FUNC_QUALIFIER static vec<1, R, P> call(R (*Func) (T x), vec<1, T, P> const & v)
		{
			return vec<1, R, P>(Func(v.x));
		}
	};

	template<typename R, typename T, precision P>
	struct functor1<2, R, T, P>
	{
		GLM_FUNC_QUALIFIER static vec<2, R, P> call(R (*Func) (T x), vec<2, T, P> const & v)
		{
			return vec<2, R, P>(Func(v.x), Func(v.y));
		}
	};

	template<typename R, typename T, precision P>
	struct functor1<3, R, T, P>
	{
		GLM_FUNC_QUALIFIER static vec<3, R, P> call(R (*Func) (T x), vec<3, T, P> const & v)
		{
			return vec<3, R, P>(Func(v.x), Func(v.y), Func(v.z));
		}
	};

	template<typename R, typename T, precision P>
	struct functor1<4, R, T, P>
	{
		GLM_FUNC_QUALIFIER static vec<4, R, P> call(R (*Func) (T x), vec<4, T, P> const & v)
		{
			return vec<4, R, P>(Func(v.x), Func(v.y), Func(v.z), Func(v.w));
		}
	};

	template<length_t L, typename T, precision P>
	struct functor2{};

	template<typename T, precision P>
	struct functor2<1, T, P>
	{
		GLM_FUNC_QUALIFIER static vec<1, T, P> call(T (*Func) (T x, T y), vec<1, T, P> const & a, vec<1, T, P> const & b)
		{
			return vec<1, T, P>(Func(a.x, b.x));
		}
	};

	template<typename T, precision P>
	struct functor2<2, T, P>
	{
		GLM_FUNC_QUALIFIER static vec<2, T, P> call(T (*Func) (T x, T y), vec<2, T, P> const & a, vec<2, T, P> const & b)
		{
			return vec<2, T, P>(Func(a.x, b.x), Func(a.y, b.y));
		}
	};

	template<typename T, precision P>
	struct functor2<3, T, P>
	{
		GLM_FUNC_QUALIFIER static vec<3, T, P> call(T (*Func) (T x, T y), vec<3, T, P> const & a, vec<3, T, P> const & b)
		{
			return vec<3, T, P>(Func(a.x, b.x), Func(a.y, b.y), Func(a.z, b.z));
		}
	};

	template<typename T, precision P>
	struct functor2<4, T, P>
	{
		GLM_FUNC_QUALIFIER static vec<4, T, P> call(T (*Func) (T x, T y), vec<4, T, P> const & a, vec<4, T, P> const & b)
		{
			return vec<4, T, P>(Func(a.x, b.x), Func(a.y, b.y), Func(a.z, b.z), Func(a.w, b.w));
		}
	};

	template<length_t L, typename T, precision P>
	struct functor2_vec_sca{};

	template<typename T, precision P>
	struct functor2_vec_sca<1, T, P>
	{
		GLM_FUNC_QUALIFIER static vec<1, T, P> call(T (*Func) (T x, T y), vec<1, T, P> const & a, T b)
		{
			return vec<1, T, P>(Func(a.x, b));
		}
	};

	template<typename T, precision P>
	struct functor2_vec_sca<2, T, P>
	{
		GLM_FUNC_QUALIFIER static vec<2, T, P> call(T (*Func) (T x, T y), vec<2, T, P> const & a, T b)
		{
			return vec<2, T, P>(Func(a.x, b), Func(a.y, b));
		}
	};

	template<typename T, precision P>
	struct functor2_vec_sca<3, T, P>
	{
		GLM_FUNC_QUALIFIER static vec<3, T, P> call(T (*Func) (T x, T y), vec<3, T, P> const & a, T b)
		{
			return vec<3, T, P>(Func(a.x, b), Func(a.y, b), Func(a.z, b));
		}
	};

	template<typename T, precision P>
	struct functor2_vec_sca<4, T, P>
	{
		GLM_FUNC_QUALIFIER static vec<4, T, P> call(T (*Func) (T x, T y), vec<4, T, P> const & a, T b)
		{
			return vec<4, T, P>(Func(a.x, b), Func(a.y, b), Func(a.z, b), Func(a.w, b));
		}
	};
}//namespace detail
}//namespace glm
