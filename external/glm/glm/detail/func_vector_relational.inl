/// @ref core
/// @file glm/detail/func_vector_relational.inl

#include <limits>

namespace glm
{
	template<length_t L, typename T, precision P, template<length_t, typename, precision> class vecType>
	GLM_FUNC_QUALIFIER vecType<L, bool, P> lessThan(vecType<L, T, P> const & x, vecType<L, T, P> const & y)
	{
		assert(x.length() == y.length());

		vecType<L, bool, P> Result(uninitialize);
		for(length_t i = 0; i < x.length(); ++i)
			Result[i] = x[i] < y[i];

		return Result;
	}

	template<length_t L, typename T, precision P, template<length_t, typename, precision> class vecType>
	GLM_FUNC_QUALIFIER vecType<L, bool, P> lessThanEqual(vecType<L, T, P> const & x, vecType<L, T, P> const & y)
	{
		assert(x.length() == y.length());

		vecType<L, bool, P> Result(uninitialize);
		for(length_t i = 0; i < x.length(); ++i)
			Result[i] = x[i] <= y[i];
		return Result;
	}

	template<length_t L, typename T, precision P, template<length_t, typename, precision> class vecType>
	GLM_FUNC_QUALIFIER vecType<L, bool, P> greaterThan(vecType<L, T, P> const & x, vecType<L, T, P> const & y)
	{
		assert(x.length() == y.length());

		vecType<L, bool, P> Result(uninitialize);
		for(length_t i = 0; i < x.length(); ++i)
			Result[i] = x[i] > y[i];
		return Result;
	}

	template<length_t L, typename T, precision P, template<length_t, typename, precision> class vecType>
	GLM_FUNC_QUALIFIER vecType<L, bool, P> greaterThanEqual(vecType<L, T, P> const & x, vecType<L, T, P> const & y)
	{
		assert(x.length() == y.length());

		vecType<L, bool, P> Result(uninitialize);
		for(length_t i = 0; i < x.length(); ++i)
			Result[i] = x[i] >= y[i];
		return Result;
	}

	template<length_t L, typename T, precision P, template<length_t, typename, precision> class vecType>
	GLM_FUNC_QUALIFIER vecType<L, bool, P> equal(vecType<L, T, P> const & x, vecType<L, T, P> const & y)
	{
		assert(x.length() == y.length());

		vecType<L, bool, P> Result(uninitialize);
		for(length_t i = 0; i < x.length(); ++i)
			Result[i] = x[i] == y[i];
		return Result;
	}

	template<length_t L, typename T, precision P, template<length_t, typename, precision> class vecType>
	GLM_FUNC_QUALIFIER vecType<L, bool, P> notEqual(vecType<L, T, P> const & x, vecType<L, T, P> const & y)
	{
		assert(x.length() == y.length());

		vecType<L, bool, P> Result(uninitialize);
		for(length_t i = 0; i < x.length(); ++i)
			Result[i] = x[i] != y[i];
		return Result;
	}

	template<length_t L, precision P, template<length_t, typename, precision> class vecType>
	GLM_FUNC_QUALIFIER bool any(vecType<L, bool, P> const & v)
	{
		bool Result = false;
		for(length_t i = 0; i < v.length(); ++i)
			Result = Result || v[i];
		return Result;
	}

	template<length_t L, precision P, template<length_t, typename, precision> class vecType>
	GLM_FUNC_QUALIFIER bool all(vecType<L, bool, P> const & v)
	{
		bool Result = true;
		for(length_t i = 0; i < v.length(); ++i)
			Result = Result && v[i];
		return Result;
	}

	template<length_t L, precision P, template<length_t, typename, precision> class vecType>
	GLM_FUNC_QUALIFIER vecType<L, bool, P> not_(vecType<L, bool, P> const & v)
	{
		vecType<L, bool, P> Result(uninitialize);
		for(length_t i = 0; i < v.length(); ++i)
			Result[i] = !v[i];
		return Result;
	}
}//namespace glm

#if GLM_ARCH != GLM_ARCH_PURE && GLM_HAS_UNRESTRICTED_UNIONS
#	include "func_vector_relational_simd.inl"
#endif
