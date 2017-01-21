/// @ref gtx_matrix_cross_product
/// @file glm/gtx/matrix_cross_product.inl

namespace glm
{
	template<typename T, precision P>
	GLM_FUNC_QUALIFIER mat<3, 3, T, P> matrixCross3
	(
		vec<3, T, P> const & x
	)
	{
		mat<3, 3, T, P> Result(T(0));
		Result[0][1] = x.z;
		Result[1][0] = -x.z;
		Result[0][2] = -x.y;
		Result[2][0] = x.y;
		Result[1][2] = x.x;
		Result[2][1] = -x.x;
		return Result;
	}

	template<typename T, precision P>
	GLM_FUNC_QUALIFIER mat<4, 4, T, P> matrixCross4
	(
		vec<3, T, P> const & x
	)
	{
		mat<4, 4, T, P> Result(T(0));
		Result[0][1] = x.z;
		Result[1][0] = -x.z;
		Result[0][2] = -x.y;
		Result[2][0] = x.y;
		Result[1][2] = x.x;
		Result[2][1] = -x.x;
		return Result;
	}

}//namespace glm
