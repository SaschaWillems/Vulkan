/// @ref gtx_handed_coordinate_space
/// @file glm/gtx/handed_coordinate_space.inl

namespace glm
{
	template<typename T, precision P>
	GLM_FUNC_QUALIFIER bool rightHanded
	(
		vec<3, T, P> const & tangent,
		vec<3, T, P> const & binormal,
		vec<3, T, P> const & normal
	)
	{
		return dot(cross(normal, tangent), binormal) > T(0);
	}

	template<typename T, precision P>
	GLM_FUNC_QUALIFIER bool leftHanded
	(
		vec<3, T, P> const & tangent,
		vec<3, T, P> const & binormal,
		vec<3, T, P> const & normal
	)
	{
		return dot(cross(normal, tangent), binormal) < T(0);
	}
}//namespace glm
