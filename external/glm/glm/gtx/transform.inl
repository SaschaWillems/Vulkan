/// @ref gtx_transform
/// @file glm/gtx/transform.inl

namespace glm
{
	template<typename T, precision P>
	GLM_FUNC_QUALIFIER mat<4, 4, T, P> translate(vec<3, T, P> const & v)
	{
		return translate(mat<4, 4, T, P>(static_cast<T>(1)), v);
	}

	template<typename T, precision P>
	GLM_FUNC_QUALIFIER mat<4, 4, T, P> rotate(T angle, vec<3, T, P> const & v)
	{
		return rotate(mat<4, 4, T, P>(static_cast<T>(1)), angle, v);
	}

	template<typename T, precision P>
	GLM_FUNC_QUALIFIER mat<4, 4, T, P> scale(vec<3, T, P> const & v)
	{
		return scale(mat<4, 4, T, P>(static_cast<T>(1)), v);
	}

}//namespace glm
