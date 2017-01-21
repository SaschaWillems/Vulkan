/// @ref gtx_extend
/// @file glm/gtx/extend.inl

namespace glm
{
	template<typename genType>
	GLM_FUNC_QUALIFIER genType extend
	(
		genType const & Origin, 
		genType const & Source, 
		genType const & Distance
	)
	{
		return Origin + (Source - Origin) * Distance;
	}

	template<typename T, precision P>
	GLM_FUNC_QUALIFIER vec<2, T, P> extend
	(
		vec<2, T, P> const & Origin,
		vec<2, T, P> const & Source,
		T const & Distance
	)
	{
		return Origin + (Source - Origin) * Distance;
	}

	template<typename T, precision P>
	GLM_FUNC_QUALIFIER vec<3, T, P> extend
	(
		vec<3, T, P> const & Origin,
		vec<3, T, P> const & Source,
		T const & Distance
	)
	{
		return Origin + (Source - Origin) * Distance;
	}

	template<typename T, precision P>
	GLM_FUNC_QUALIFIER vec<4, T, P> extend
	(
		vec<4, T, P> const & Origin,
		vec<4, T, P> const & Source,
		T const & Distance
	)
	{
		return Origin + (Source - Origin) * Distance;
	}
}//namespace glm
