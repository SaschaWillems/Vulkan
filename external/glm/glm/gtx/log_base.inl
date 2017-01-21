/// @ref gtx_log_base
/// @file glm/gtx/log_base.inl

namespace glm
{
	template<typename genType> 
	GLM_FUNC_QUALIFIER genType log(genType const & x, genType const & base)
	{
		assert(x != genType(0));
		return glm::log(x) / glm::log(base);
	}

	template<length_t L, typename T, precision P, template<length_t, typename, precision> class vecType>
	GLM_FUNC_QUALIFIER vecType<L, T, P> log(vecType<L, T, P> const & x, vecType<L, T, P> const & base)
	{
		return glm::log(x) / glm::log(base);
	}
}//namespace glm
