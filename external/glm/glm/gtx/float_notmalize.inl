/// @ref gtx_float_normalize
/// @file glm/gtx/float_normalize.inl

#include <limits>

namespace glm
{
	template<length_t L, typename T, precision P, template<length_t, typename, precision> class vecType>
	GLM_FUNC_QUALIFIER vecType<L, float, P> floatNormalize(vecType<L, T, P> const & v)
	{
		return vecType<L, float, P>(v) / static_cast<float>(std::numeric_limits<T>::max());
	}

}//namespace glm
