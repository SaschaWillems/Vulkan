/// @ref core
/// @file glm/detail/func_geometric.inl

#include "func_exponential.hpp"
#include "func_common.hpp"
#include "type_vec2.hpp"
#include "type_vec4.hpp"
#include "type_float.hpp"

namespace glm{
namespace detail
{
	template<template<length_t, typename, precision> class vecType, length_t L, typename T, precision P, bool Aligned>
	struct compute_length
	{
		GLM_FUNC_QUALIFIER static T call(vecType<L, T, P> const & v)
		{
			return sqrt(dot(v, v));
		}
	};

	template<template<length_t, typename, precision> class vecType, length_t L, typename T, precision P, bool Aligned>
	struct compute_distance
	{
		GLM_FUNC_QUALIFIER static T call(vecType<L, T, P> const & p0, vecType<L, T, P> const & p1)
		{
			return length(p1 - p0);
		}
	};

	template<typename V, typename T, bool Aligned>
	struct compute_dot{};

	template<typename T, precision P, bool Aligned>
	struct compute_dot<vec<1, T, P>, T, Aligned>
	{
		GLM_FUNC_QUALIFIER static T call(vec<1, T, P> const & a, vec<1, T, P> const & b)
		{
			return a.x * b.x;
		}
	};

	template<typename T, precision P, bool Aligned>
	struct compute_dot<vec<2, T, P>, T, Aligned>
	{
		GLM_FUNC_QUALIFIER static T call(vec<2, T, P> const & a, vec<2, T, P> const & b)
		{
			vec<2, T, P> tmp(a * b);
			return tmp.x + tmp.y;
		}
	};

	template<typename T, precision P, bool Aligned>
	struct compute_dot<vec<3, T, P>, T, Aligned>
	{
		GLM_FUNC_QUALIFIER static T call(vec<3, T, P> const & a, vec<3, T, P> const & b)
		{
			vec<3, T, P> tmp(a * b);
			return tmp.x + tmp.y + tmp.z;
		}
	};

	template<typename T, precision P, bool Aligned>
	struct compute_dot<vec<4, T, P>, T, Aligned>
	{
		GLM_FUNC_QUALIFIER static T call(vec<4, T, P> const & a, vec<4, T, P> const & b)
		{
			vec<4, T, P> tmp(a * b);
			return (tmp.x + tmp.y) + (tmp.z + tmp.w);
		}
	};

	template<typename T, precision P, bool Aligned>
	struct compute_cross
	{
		GLM_FUNC_QUALIFIER static vec<3, T, P> call(vec<3, T, P> const & x, vec<3, T, P> const & y)
		{
			GLM_STATIC_ASSERT(std::numeric_limits<T>::is_iec559, "'cross' accepts only floating-point inputs");

			return vec<3, T, P>(
				x.y * y.z - y.y * x.z,
				x.z * y.x - y.z * x.x,
				x.x * y.y - y.x * x.y);
		}
	};

	template<length_t L, typename T, precision P, template<length_t, typename, precision> class vecType, bool Aligned>
	struct compute_normalize
	{
		GLM_FUNC_QUALIFIER static vecType<L, T, P> call(vecType<L, T, P> const & v)
		{
			GLM_STATIC_ASSERT(std::numeric_limits<T>::is_iec559, "'normalize' accepts only floating-point inputs");

			return v * inversesqrt(dot(v, v));
		}
	};

	template<length_t L, typename T, precision P, template<length_t, typename, precision> class vecType, bool Aligned>
	struct compute_faceforward
	{
		GLM_FUNC_QUALIFIER static vecType<L, T, P> call(vecType<L, T, P> const & N, vecType<L, T, P> const & I, vecType<L, T, P> const & Nref)
		{
			GLM_STATIC_ASSERT(std::numeric_limits<T>::is_iec559, "'normalize' accepts only floating-point inputs");

			return dot(Nref, I) < static_cast<T>(0) ? N : -N;
		}
	};

	template<length_t L, typename T, precision P, template<length_t, typename, precision> class vecType, bool Aligned>
	struct compute_reflect
	{
		GLM_FUNC_QUALIFIER static vecType<L, T, P> call(vecType<L, T, P> const & I, vecType<L, T, P> const & N)
		{
			return I - N * dot(N, I) * static_cast<T>(2);
		}
	};

	template<length_t L, typename T, precision P, template<length_t, typename, precision> class vecType, bool Aligned>
	struct compute_refract
	{
		GLM_FUNC_QUALIFIER static vecType<L, T, P> call(vecType<L, T, P> const & I, vecType<L, T, P> const & N, T eta)
		{
			T const dotValue(dot(N, I));
			T const k(static_cast<T>(1) - eta * eta * (static_cast<T>(1) - dotValue * dotValue));
			return (eta * I - (eta * dotValue + std::sqrt(k)) * N) * static_cast<T>(k >= static_cast<T>(0));
		}
	};
}//namespace detail

	// length
	template<typename genType>
	GLM_FUNC_QUALIFIER genType length(genType x)
	{
		GLM_STATIC_ASSERT(std::numeric_limits<genType>::is_iec559, "'length' accepts only floating-point inputs");

		return abs(x);
	}

	template<length_t L, typename T, precision P, template<length_t, typename, precision> class vecType>
	GLM_FUNC_QUALIFIER T length(vecType<L, T, P> const & v)
	{
		GLM_STATIC_ASSERT(std::numeric_limits<T>::is_iec559, "'length' accepts only floating-point inputs");

		return detail::compute_length<vecType, L, T, P, detail::is_aligned<P>::value>::call(v);
	}

	// distance
	template<typename genType>
	GLM_FUNC_QUALIFIER genType distance(genType const & p0, genType const & p1)
	{
		GLM_STATIC_ASSERT(std::numeric_limits<genType>::is_iec559, "'distance' accepts only floating-point inputs");

		return length(p1 - p0);
	}

	template<length_t L, typename T, precision P, template<length_t, typename, precision> class vecType>
	GLM_FUNC_QUALIFIER T distance(vecType<L, T, P> const & p0, vecType<L, T, P> const & p1)
	{
		return detail::compute_distance<vecType, L, T, P, detail::is_aligned<P>::value>::call(p0, p1);
	}

	// dot
	template<typename T>
	GLM_FUNC_QUALIFIER T dot(T x, T y)
	{
		GLM_STATIC_ASSERT(std::numeric_limits<T>::is_iec559, "'dot' accepts only floating-point inputs");
		return x * y;
	}

	template<length_t L, typename T, precision P>
	GLM_FUNC_QUALIFIER T dot(vec<L, T, P> const & x, vec<L, T, P> const & y)
	{
		GLM_STATIC_ASSERT(std::numeric_limits<T>::is_iec559, "'dot' accepts only floating-point inputs");
		return detail::compute_dot<vec<L, T, P>, T, detail::is_aligned<P>::value>::call(x, y);
	}

	template<typename T, precision P>
	GLM_FUNC_QUALIFIER T dot(tquat<T, P> const & x, tquat<T, P> const & y)
	{
		GLM_STATIC_ASSERT(std::numeric_limits<T>::is_iec559, "'dot' accepts only floating-point inputs");
		return detail::compute_dot<tquat<T, P>, T, detail::is_aligned<P>::value>::call(x, y);
	}

	// cross
	template<typename T, precision P>
	GLM_FUNC_QUALIFIER vec<3, T, P> cross(vec<3, T, P> const & x, vec<3, T, P> const & y)
	{
		return detail::compute_cross<T, P, detail::is_aligned<P>::value>::call(x, y);
	}

	// normalize
	template<typename genType>
	GLM_FUNC_QUALIFIER genType normalize(genType const & x)
	{
		GLM_STATIC_ASSERT(std::numeric_limits<genType>::is_iec559, "'normalize' accepts only floating-point inputs");

		return x < genType(0) ? genType(-1) : genType(1);
	}

	template<length_t L, typename T, precision P, template<length_t, typename, precision> class vecType>
	GLM_FUNC_QUALIFIER vecType<L, T, P> normalize(vecType<L, T, P> const & x)
	{
		GLM_STATIC_ASSERT(std::numeric_limits<T>::is_iec559, "'normalize' accepts only floating-point inputs");

		return detail::compute_normalize<L, T, P, vecType, detail::is_aligned<P>::value>::call(x);
	}

	// faceforward
	template<typename genType>
	GLM_FUNC_QUALIFIER genType faceforward(genType const & N, genType const & I, genType const & Nref)
	{
		return dot(Nref, I) < static_cast<genType>(0) ? N : -N;
	}

	template<length_t L, typename T, precision P, template<length_t, typename, precision> class vecType>
	GLM_FUNC_QUALIFIER vecType<L, T, P> faceforward(vecType<L, T, P> const & N, vecType<L, T, P> const & I, vecType<L, T, P> const & Nref)
	{
		return detail::compute_faceforward<L, T, P, vecType, detail::is_aligned<P>::value>::call(N, I, Nref);
	}

	// reflect
	template<typename genType>
	GLM_FUNC_QUALIFIER genType reflect(genType const & I, genType const & N)
	{
		return I - N * dot(N, I) * genType(2);
	}

	template<length_t L, typename T, precision P, template<length_t, typename, precision> class vecType>
	GLM_FUNC_QUALIFIER vecType<L, T, P> reflect(vecType<L, T, P> const & I, vecType<L, T, P> const & N)
	{
		return detail::compute_reflect<L, T, P, vecType, detail::is_aligned<P>::value>::call(I, N);
	}

	// refract
	template<typename genType>
	GLM_FUNC_QUALIFIER genType refract(genType const & I, genType const & N, genType eta)
	{
		GLM_STATIC_ASSERT(std::numeric_limits<genType>::is_iec559, "'refract' accepts only floating-point inputs");
		genType const dotValue(dot(N, I));
		genType const k(static_cast<genType>(1) - eta * eta * (static_cast<genType>(1) - dotValue * dotValue));
		return (eta * I - (eta * dotValue + sqrt(k)) * N) * static_cast<genType>(k >= static_cast<genType>(0));
	}

	template<length_t L, typename T, precision P, template<length_t, typename, precision> class vecType>
	GLM_FUNC_QUALIFIER vecType<L, T, P> refract(vecType<L, T, P> const & I, vecType<L, T, P> const & N, T eta)
	{
		GLM_STATIC_ASSERT(std::numeric_limits<T>::is_iec559, "'refract' accepts only floating-point inputs");
		return detail::compute_refract<L, T, P, vecType, detail::is_aligned<P>::value>::call(I, N, eta);
	}
}//namespace glm

#if GLM_ARCH != GLM_ARCH_PURE && GLM_HAS_UNRESTRICTED_UNIONS
#	include "func_geometric_simd.inl"
#endif
