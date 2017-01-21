/// @ref core
/// @file glm/detail/func_geometric_simd.inl

#include "../simd/geometric.h"

#if GLM_ARCH & GLM_ARCH_SSE2_BIT

namespace glm{
namespace detail
{
	template<precision P>
	struct compute_length<vec, 4, float, P, true>
	{
		GLM_FUNC_QUALIFIER static float call(vec<4, float, P> const & v)
		{
			return _mm_cvtss_f32(glm_vec4_length(v.data));
		}
	};

	template<precision P>
	struct compute_distance<vec, 4, float, P, true>
	{
		GLM_FUNC_QUALIFIER static float call(vec<4, float, P> const & p0, vec<4, float, P> const & p1)
		{
			return _mm_cvtss_f32(glm_vec4_distance(p0.data, p1.data));
		}
	};

	template<precision P>
	struct compute_dot<vec<4, float, P>, float, true>
	{
		GLM_FUNC_QUALIFIER static float call(vec<4, float, P> const& x, vec<4, float, P> const& y)
		{
			return _mm_cvtss_f32(glm_vec1_dot(x.data, y.data));
		}
	};

	template<precision P>
	struct compute_cross<float, P, true>
	{
		GLM_FUNC_QUALIFIER static vec<3, float, P> call(vec<3, float, P> const & a, vec<3, float, P> const & b)
		{
			__m128 const set0 = _mm_set_ps(0.0f, a.z, a.y, a.x);
			__m128 const set1 = _mm_set_ps(0.0f, b.z, b.y, b.x);
			__m128 const xpd0 = glm_vec4_cross(set0, set1);

			vec<4, float, P> result(uninitialize);
			result.data = xpd0;
			return vec<3, float, P>(result);
		}
	};

	template<precision P>
	struct compute_normalize<4, float, P, vec, true>
	{
		GLM_FUNC_QUALIFIER static vec<4, float, P> call(vec<4, float, P> const & v)
		{
			vec<4, float, P> result(uninitialize);
			result.data = glm_vec4_normalize(v.data);
			return result;
		}
	};

	template<precision P>
	struct compute_faceforward<4, float, P, vec, true>
	{
		GLM_FUNC_QUALIFIER static vec<4, float, P> call(vec<4, float, P> const& N, vec<4, float, P> const& I, vec<4, float, P> const& Nref)
		{
			vec<4, float, P> result(uninitialize);
			result.data = glm_vec4_faceforward(N.data, I.data, Nref.data);
			return result;
		}
	};

	template<precision P>
	struct compute_reflect<4, float, P, vec, true>
	{
		GLM_FUNC_QUALIFIER static vec<4, float, P> call(vec<4, float, P> const& I, vec<4, float, P> const& N)
		{
			vec<4, float, P> result(uninitialize);
			result.data = glm_vec4_reflect(I.data, N.data);
			return result;
		}
	};

	template<precision P>
	struct compute_refract<4, float, P, vec, true>
	{
		GLM_FUNC_QUALIFIER static vec<4, float, P> call(vec<4, float, P> const& I, vec<4, float, P> const& N, float eta)
		{
			vec<4, float, P> result(uninitialize);
			result.data = glm_vec4_refract(I.data, N.data, _mm_set1_ps(eta));
			return result;
		}
	};
}//namespace detail
}//namespace glm

#endif//GLM_ARCH & GLM_ARCH_SSE2_BIT
