/// @ref gtx_closest_point
/// @file glm/gtx/closest_point.inl

namespace glm
{
	template<typename T, precision P>
	GLM_FUNC_QUALIFIER vec<3, T, P> closestPointOnLine
	(
		vec<3, T, P> const & point,
		vec<3, T, P> const & a,
		vec<3, T, P> const & b
	)
	{
		T LineLength = distance(a, b);
		vec<3, T, P> Vector = point - a;
		vec<3, T, P> LineDirection = (b - a) / LineLength;

		// Project Vector to LineDirection to get the distance of point from a
		T Distance = dot(Vector, LineDirection);

		if(Distance <= T(0)) return a;
		if(Distance >= LineLength) return b;
		return a + LineDirection * Distance;
	}
	
	template<typename T, precision P>
	GLM_FUNC_QUALIFIER vec<2, T, P> closestPointOnLine
	(
		vec<2, T, P> const & point,
		vec<2, T, P> const & a,
		vec<2, T, P> const & b
	)
	{
		T LineLength = distance(a, b);
		vec<2, T, P> Vector = point - a;
		vec<2, T, P> LineDirection = (b - a) / LineLength;

		// Project Vector to LineDirection to get the distance of point from a
		T Distance = dot(Vector, LineDirection);

		if(Distance <= T(0)) return a;
		if(Distance >= LineLength) return b;
		return a + LineDirection * Distance;
	}
	
}//namespace glm
