/// @ref core
/// @file glm/detail/func_matrix_simd.inl

#if GLM_ARCH & GLM_ARCH_SSE2_BIT

#include "type_mat4x4.hpp"
#include "func_geometric.hpp"
#include "../simd/matrix.h"

namespace glm{
namespace detail
{
	template<precision P>
	struct compute_matrixCompMult<mat, 4, 4, float, P, true>
	{
		GLM_STATIC_ASSERT(detail::is_aligned<P>::value, "Specialization requires aligned");

		GLM_FUNC_QUALIFIER static mat<4, 4, float, P> call(mat<4, 4, float, P> const & x, mat<4, 4, float, P> const & y)
		{
			mat<4, 4, float, P> result(uninitialize);
			glm_mat4_matrixCompMult(
				*(glm_vec4 const (*)[4])&x[0].data,
				*(glm_vec4 const (*)[4])&y[0].data,
				*(glm_vec4(*)[4])&result[0].data);
			return result;
		}
	};

	template<precision P>
	struct compute_transpose<mat, 4, 4, float, P, true>
	{
		GLM_FUNC_QUALIFIER static mat<4, 4, float, P> call(mat<4, 4, float, P> const & m)
		{
			mat<4, 4, float, P> result(uninitialize);
			glm_mat4_transpose(
				*(glm_vec4 const (*)[4])&m[0].data,
				*(glm_vec4(*)[4])&result[0].data);
			return result;
		}
	};

	template<precision P>
	struct compute_determinant<mat, 4, 4, float, P, true>
	{
		GLM_FUNC_QUALIFIER static float call(mat<4, 4, float, P> const& m)
		{
			return _mm_cvtss_f32(glm_mat4_determinant(*reinterpret_cast<__m128 const(*)[4]>(&m[0].data)));
		}
	};

	template<precision P>
	struct compute_inverse<mat, 4, 4, float, P, true>
	{
		GLM_FUNC_QUALIFIER static mat<4, 4, float, P> call(mat<4, 4, float, P> const& m)
		{
			mat<4, 4, float, P> Result(uninitialize);
			glm_mat4_inverse(*reinterpret_cast<__m128 const(*)[4]>(&m[0].data), *reinterpret_cast<__m128(*)[4]>(&Result[0].data));
			return Result;
		}
	};
}//namespace detail

	template<>
	GLM_FUNC_QUALIFIER mat<4, 4, float, aligned_lowp> outerProduct<4, 4, float, aligned_lowp, vec, vec>(vec<4, float, aligned_lowp> const & c, vec<4, float, aligned_lowp> const & r)
	{
		mat<4, 4, float, aligned_lowp> m(uninitialize);
		glm_mat4_outerProduct(c.data, r.data, *reinterpret_cast<__m128(*)[4]>(&m[0].data));
		return m;
	}

	template<>
	GLM_FUNC_QUALIFIER mat<4, 4, float, aligned_mediump> outerProduct<4, 4, float, aligned_mediump, vec, vec>(vec<4, float, aligned_mediump> const & c, vec<4, float, aligned_mediump> const & r)
	{
		mat<4, 4, float, aligned_mediump> m(uninitialize);
		glm_mat4_outerProduct(c.data, r.data, *reinterpret_cast<__m128(*)[4]>(&m[0].data));
		return m;
	}

	template<>
	GLM_FUNC_QUALIFIER mat<4, 4, float, aligned_highp> outerProduct<4, 4, float, aligned_highp, vec, vec>(vec<4, float, aligned_highp> const & c, vec<4, float, aligned_highp> const & r)
	{
		mat<4, 4, float, aligned_highp> m(uninitialize);
		glm_mat4_outerProduct(c.data, r.data, *reinterpret_cast<__m128(*)[4]>(&m[0].data));
		return m;
	}
}//namespace glm

#endif
