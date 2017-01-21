/// @ref gtx_polar_coordinates
/// @file glm/gtx/polar_coordinates.inl

namespace glm
{
	template<typename T, precision P>
	GLM_FUNC_QUALIFIER vec<3, T, P> polar
	(
		vec<3, T, P> const & euclidean
	)
	{
		T const Length(length(euclidean));
		vec<3, T, P> const tmp(euclidean / Length);
		T const xz_dist(sqrt(tmp.x * tmp.x + tmp.z * tmp.z));

		return vec<3, T, P>(
			asin(tmp.y),	// latitude
			atan(tmp.x, tmp.z),		// longitude
			xz_dist);				// xz distance
	}

	template<typename T, precision P>
	GLM_FUNC_QUALIFIER vec<3, T, P> euclidean
	(
		vec<2, T, P> const & polar
	)
	{
		T const latitude(polar.x);
		T const longitude(polar.y);

		return vec<3, T, P>(
			cos(latitude) * sin(longitude),
			sin(latitude),
			cos(latitude) * cos(longitude));
	}

}//namespace glm
