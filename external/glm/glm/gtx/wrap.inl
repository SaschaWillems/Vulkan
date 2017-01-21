/// @ref gtx_wrap
/// @file glm/gtx/wrap.inl

namespace glm
{
	template<length_t L, typename T, precision P, template<length_t, typename, precision> class vecType>
	GLM_FUNC_QUALIFIER vecType<L, T, P> clamp(vecType<L, T, P> const& Texcoord)
	{
		return glm::clamp(Texcoord, vecType<L, T, P>(0), vecType<L, T, P>(1));
	}

	template<typename genType>
	GLM_FUNC_QUALIFIER genType clamp(genType const & Texcoord)
	{
		return clamp(vec<1, genType, defaultp>(Texcoord)).x;
	}

	template<length_t L, typename T, precision P, template<length_t, typename, precision> class vecType>
	GLM_FUNC_QUALIFIER vecType<L, T, P> repeat(vecType<L, T, P> const& Texcoord)
	{
		return glm::fract(Texcoord);
	}

	template<typename genType>
	GLM_FUNC_QUALIFIER genType repeat(genType const & Texcoord)
	{
		return repeat(vec<1, genType, defaultp>(Texcoord)).x;
	}

	template<length_t L, typename T, precision P, template<length_t, typename, precision> class vecType>
	GLM_FUNC_QUALIFIER vecType<L, T, P> mirrorClamp(vecType<L, T, P> const& Texcoord)
	{
		return glm::fract(glm::abs(Texcoord));
	}

	template<typename genType>
	GLM_FUNC_QUALIFIER genType mirrorClamp(genType const & Texcoord)
	{
		return mirrorClamp(vec<1, genType, defaultp>(Texcoord)).x;
	}

	template<length_t L, typename T, precision P, template<length_t, typename, precision> class vecType>
	GLM_FUNC_QUALIFIER vecType<L, T, P> mirrorRepeat(vecType<L, T, P> const& Texcoord)
	{
		vecType<L, T, P> const Abs = glm::abs(Texcoord);
		vecType<L, T, P> const Clamp = glm::mod(glm::floor(Abs), vecType<L, T, P>(2));
		vecType<L, T, P> const Floor = glm::floor(Abs);
		vecType<L, T, P> const Rest = Abs - Floor;
		vecType<L, T, P> const Mirror = Clamp + Rest;
		return mix(Rest, vecType<L, T, P>(1) - Rest, glm::greaterThanEqual(Mirror, vecType<L, T, P>(1)));
	}

	template<typename genType>
	GLM_FUNC_QUALIFIER genType mirrorRepeat(genType const& Texcoord)
	{
		return mirrorRepeat(vec<1, genType, defaultp>(Texcoord)).x;
	}
}//namespace glm
