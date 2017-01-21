/// @ref gtx_normalize_dot
/// @file glm/gtx/normalize_dot.inl

namespace glm
{
	template<length_t L, typename T, precision P, template<length_t, typename, precision> class vecType>
	GLM_FUNC_QUALIFIER T normalizeDot(vecType<L, T, P> const & x, vecType<L, T, P> const & y)
	{
		return glm::dot(x, y) * glm::inversesqrt(glm::dot(x, x) * glm::dot(y, y));
	}

	template<length_t L, typename T, precision P, template<length_t, typename, precision> class vecType>
	GLM_FUNC_QUALIFIER T fastNormalizeDot(vecType<L, T, P> const & x, vecType<L, T, P> const & y)
	{
		return glm::dot(x, y) * glm::fastInverseSqrt(glm::dot(x, x) * glm::dot(y, y));
	}
}//namespace glm
