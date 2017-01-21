/// @ref gtx_vector_query
/// @file glm/gtx/vector_query.inl

#include <cassert>

namespace glm{
namespace detail
{
	template<length_t L, typename T, precision P, template<length_t, typename, precision> class vecType>
	struct compute_areCollinear{};

	template<typename T, precision P>
	struct compute_areCollinear<2, T, P, vec>
	{
		GLM_FUNC_QUALIFIER static bool call(vec<2, T, P> const & v0, vec<2, T, P> const & v1, T const & epsilon)
		{
			return length(cross(vec<3, T, P>(v0, static_cast<T>(0)), vec<3, T, P>(v1, static_cast<T>(0)))) < epsilon;
		}
	};

	template<typename T, precision P>
	struct compute_areCollinear<3, T, P, vec>
	{
		GLM_FUNC_QUALIFIER static bool call(vec<3, T, P> const & v0, vec<3, T, P> const & v1, T const & epsilon)
		{
			return length(cross(v0, v1)) < epsilon;
		}
	};

	template<typename T, precision P>
	struct compute_areCollinear<4, T, P, vec>
	{
		GLM_FUNC_QUALIFIER static bool call(vec<4, T, P> const & v0, vec<4, T, P> const & v1, T const & epsilon)
		{
			return length(cross(vec<3, T, P>(v0), vec<3, T, P>(v1))) < epsilon;
		}
	};

	template<length_t L, typename T, precision P, template<length_t, typename, precision> class vecType>
	struct compute_isCompNull{};

	template<typename T, precision P>
	struct compute_isCompNull<2, T, P, vec>
	{
		GLM_FUNC_QUALIFIER static vec<2, bool, P> call(vec<2, T, P> const & v, T const & epsilon)
		{
			return vec<2, bool, P>(
				(abs(v.x) < epsilon),
				(abs(v.y) < epsilon));
		}
	};

	template<typename T, precision P>
	struct compute_isCompNull<3, T, P, vec>
	{
		GLM_FUNC_QUALIFIER static vec<3, bool, P> call(vec<3, T, P> const & v, T const & epsilon)
		{
			return vec<3, bool, P>(
				(abs(v.x) < epsilon),
				(abs(v.y) < epsilon),
				(abs(v.z) < epsilon));
		}
	};

	template<typename T, precision P>
	struct compute_isCompNull<4, T, P, vec>
	{
		GLM_FUNC_QUALIFIER static vec<4, bool, P> call(vec<4, T, P> const & v, T const & epsilon)
		{
			return vec<4, bool, P>(
				(abs(v.x) < epsilon),
				(abs(v.y) < epsilon),
				(abs(v.z) < epsilon),
				(abs(v.w) < epsilon));
		}
	};

}//namespace detail

	template<length_t L, typename T, precision P, template<length_t, typename, precision> class vecType>
	GLM_FUNC_QUALIFIER bool areCollinear
	(
		vecType<L, T, P> const& v0,
		vecType<L, T, P> const& v1,
		T const & epsilon
	)
	{
		GLM_STATIC_ASSERT(std::numeric_limits<T>::is_iec559, "'areCollinear' only accept floating-point inputs");

		return detail::compute_areCollinear<L, T, P, vecType>::call(v0, v1, epsilon);
	}

	template<length_t L, typename T, precision P, template<length_t, typename, precision> class vecType>
	GLM_FUNC_QUALIFIER bool areOrthogonal
	(
		vecType<L, T, P> const& v0,
		vecType<L, T, P> const& v1,
		T const & epsilon
	)
	{
		GLM_STATIC_ASSERT(std::numeric_limits<T>::is_iec559, "'areOrthogonal' only accept floating-point inputs");

		return abs(dot(v0, v1)) <= max(
			static_cast<T>(1),
			length(v0)) * max(static_cast<T>(1), length(v1)) * epsilon;
	}

	template<length_t L, typename T, precision P, template<length_t, typename, precision> class vecType>
	GLM_FUNC_QUALIFIER bool isNormalized
	(
		vecType<L, T, P> const& v,
		T const & epsilon
	)
	{
		GLM_STATIC_ASSERT(std::numeric_limits<T>::is_iec559, "'isNormalized' only accept floating-point inputs");

		return abs(length(v) - static_cast<T>(1)) <= static_cast<T>(2) * epsilon;
	}

	template<length_t L, typename T, precision P, template<length_t, typename, precision> class vecType>
	GLM_FUNC_QUALIFIER bool isNull
	(
		vecType<L, T, P> const& v,
		T const & epsilon
	)
	{
		GLM_STATIC_ASSERT(std::numeric_limits<T>::is_iec559, "'isNull' only accept floating-point inputs");

		return length(v) <= epsilon;
	}

	template<length_t L, typename T, precision P, template<length_t, typename, precision> class vecType>
	GLM_FUNC_QUALIFIER vecType<L, bool, P> isCompNull
	(
		vecType<L, T, P> const& v,
		T const & epsilon
	)
	{
		GLM_STATIC_ASSERT(std::numeric_limits<T>::is_iec559, "'isCompNull' only accept floating-point inputs");

		return detail::compute_isCompNull<L, T, P, vecType>::call(v, epsilon);
	}

	template<typename T, precision P>
	GLM_FUNC_QUALIFIER vec<2, bool, P> isCompNull
	(
		vec<2, T, P> const & v,
		T const & epsilon)
	{
		return vec<2, bool, P>(
			abs(v.x) < epsilon,
			abs(v.y) < epsilon);
	}

	template<typename T, precision P>
	GLM_FUNC_QUALIFIER vec<3, bool, P> isCompNull
	(
		vec<3, T, P> const & v,
		T const & epsilon
	)
	{
		return vec<3, bool, P>(
			abs(v.x) < epsilon,
			abs(v.y) < epsilon,
			abs(v.z) < epsilon);
	}

	template<typename T, precision P>
	GLM_FUNC_QUALIFIER vec<4, bool, P> isCompNull
	(
		vec<4, T, P> const & v,
		T const & epsilon
	)
	{
		return vec<4, bool, P>(
			abs(v.x) < epsilon,
			abs(v.y) < epsilon,
			abs(v.z) < epsilon,
			abs(v.w) < epsilon);
	}

	template<length_t L, typename T, precision P, template<length_t, typename, precision> class vecType>
	GLM_FUNC_QUALIFIER bool areOrthonormal
	(
		vecType<L, T, P> const& v0,
		vecType<L, T, P> const& v1,
		T const & epsilon
	)
	{
		return isNormalized(v0, epsilon) && isNormalized(v1, epsilon) && (abs(dot(v0, v1)) <= epsilon);
	}

}//namespace glm
