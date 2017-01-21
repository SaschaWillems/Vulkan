/// @ref gtx_rotate_vector
/// @file glm/gtx/rotate_vector.inl

namespace glm
{
	template<typename T, precision P>
	GLM_FUNC_QUALIFIER vec<3, T, P> slerp
	(
		vec<3, T, P> const & x,
		vec<3, T, P> const & y,
		T const & a
	)
	{
		// get cosine of angle between vectors (-1 -> 1)
		T CosAlpha = dot(x, y);
		// get angle (0 -> pi)
		T Alpha = acos(CosAlpha);
		// get sine of angle between vectors (0 -> 1)
		T SinAlpha = sin(Alpha);
		// this breaks down when SinAlpha = 0, i.e. Alpha = 0 or pi
		T t1 = sin((static_cast<T>(1) - a) * Alpha) / SinAlpha;
		T t2 = sin(a * Alpha) / SinAlpha;

		// interpolate src vectors
		return x * t1 + y * t2;
	}

	template<typename T, precision P>
	GLM_FUNC_QUALIFIER vec<2, T, P> rotate
	(
		vec<2, T, P> const & v,
		T const & angle
	)
	{
		vec<2, T, P> Result;
		T const Cos(cos(angle));
		T const Sin(sin(angle));

		Result.x = v.x * Cos - v.y * Sin;
		Result.y = v.x * Sin + v.y * Cos;
		return Result;
	}

	template<typename T, precision P>
	GLM_FUNC_QUALIFIER vec<3, T, P> rotate
	(
		vec<3, T, P> const & v,
		T const & angle,
		vec<3, T, P> const & normal
	)
	{
		return mat<3, 3, T, P>(glm::rotate(angle, normal)) * v;
	}
	/*
	template<typename T, precision P>
	GLM_FUNC_QUALIFIER vec<3, T, P> rotateGTX(
		const vec<3, T, P>& x,
		T angle,
		const vec<3, T, P>& normal)
	{
		const T Cos = cos(radians(angle));
		const T Sin = sin(radians(angle));
		return x * Cos + ((x * normal) * (T(1) - Cos)) * normal + cross(x, normal) * Sin;
	}
	*/
	template<typename T, precision P>
	GLM_FUNC_QUALIFIER vec<4, T, P> rotate
	(
		vec<4, T, P> const & v,
		T const & angle,
		vec<3, T, P> const & normal
	)
	{
		return rotate(angle, normal) * v;
	}

	template<typename T, precision P>
	GLM_FUNC_QUALIFIER vec<3, T, P> rotateX
	(
		vec<3, T, P> const & v,
		T const & angle
	)
	{
		vec<3, T, P> Result(v);
		T const Cos(cos(angle));
		T const Sin(sin(angle));

		Result.y = v.y * Cos - v.z * Sin;
		Result.z = v.y * Sin + v.z * Cos;
		return Result;
	}

	template<typename T, precision P>
	GLM_FUNC_QUALIFIER vec<3, T, P> rotateY
	(
		vec<3, T, P> const & v,
		T const & angle
	)
	{
		vec<3, T, P> Result = v;
		T const Cos(cos(angle));
		T const Sin(sin(angle));

		Result.x =  v.x * Cos + v.z * Sin;
		Result.z = -v.x * Sin + v.z * Cos;
		return Result;
	}

	template<typename T, precision P>
	GLM_FUNC_QUALIFIER vec<3, T, P> rotateZ
	(
		vec<3, T, P> const & v,
		T const & angle
	)
	{
		vec<3, T, P> Result = v;
		T const Cos(cos(angle));
		T const Sin(sin(angle));

		Result.x = v.x * Cos - v.y * Sin;
		Result.y = v.x * Sin + v.y * Cos;
		return Result;
	}

	template<typename T, precision P>
	GLM_FUNC_QUALIFIER vec<4, T, P> rotateX
	(
		vec<4, T, P> const & v,
		T const & angle
	)
	{
		vec<4, T, P> Result = v;
		T const Cos(cos(angle));
		T const Sin(sin(angle));

		Result.y = v.y * Cos - v.z * Sin;
		Result.z = v.y * Sin + v.z * Cos;
		return Result;
	}

	template<typename T, precision P>
	GLM_FUNC_QUALIFIER vec<4, T, P> rotateY
	(
		vec<4, T, P> const & v,
		T const & angle
	)
	{
		vec<4, T, P> Result = v;
		T const Cos(cos(angle));
		T const Sin(sin(angle));

		Result.x =  v.x * Cos + v.z * Sin;
		Result.z = -v.x * Sin + v.z * Cos;
		return Result;
	}

	template<typename T, precision P>
	GLM_FUNC_QUALIFIER vec<4, T, P> rotateZ
	(
		vec<4, T, P> const & v,
		T const & angle
	)
	{
		vec<4, T, P> Result = v;
		T const Cos(cos(angle));
		T const Sin(sin(angle));

		Result.x = v.x * Cos - v.y * Sin;
		Result.y = v.x * Sin + v.y * Cos;
		return Result;
	}

	template<typename T, precision P>
	GLM_FUNC_QUALIFIER mat<4, 4, T, P> orientation
	(
		vec<3, T, P> const & Normal,
		vec<3, T, P> const & Up
	)
	{
		if(all(equal(Normal, Up)))
			return mat<4, 4, T, P>(T(1));

		vec<3, T, P> RotationAxis = cross(Up, Normal);
		T Angle = acos(dot(Normal, Up));

		return rotate(Angle, RotationAxis);
	}
}//namespace glm
