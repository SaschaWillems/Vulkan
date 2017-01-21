/// @ref gtc_random
/// @file glm/gtc/random.inl

#include "../geometric.hpp"
#include "../exponential.hpp"
#include <cstdlib>
#include <ctime>
#include <cassert>

namespace glm{
namespace detail
{
	template<length_t L, typename T, precision P, template<int, class, precision> class vecType>
	struct compute_rand
	{
		GLM_FUNC_QUALIFIER static vecType<L, T, P> call();
	};

	template<precision P>
	struct compute_rand<1, uint8, P, vec>
	{
		GLM_FUNC_QUALIFIER static vec<1, uint8, P> call()
		{
			return vec<1, uint8, P>(
				std::rand() % std::numeric_limits<uint8>::max());
		}
	};

	template<precision P>
	struct compute_rand<2, uint8, P, vec>
	{
		GLM_FUNC_QUALIFIER static vec<2, uint8, P> call()
		{
			return vec<2, uint8, P>(
				std::rand() % std::numeric_limits<uint8>::max(),
				std::rand() % std::numeric_limits<uint8>::max());
		}
	};

	template<precision P>
	struct compute_rand<3, uint8, P, vec>
	{
		GLM_FUNC_QUALIFIER static vec<3, uint8, P> call()
		{
			return vec<3, uint8, P>(
				std::rand() % std::numeric_limits<uint8>::max(),
				std::rand() % std::numeric_limits<uint8>::max(),
				std::rand() % std::numeric_limits<uint8>::max());
		}
	};

	template<precision P>
	struct compute_rand<4, uint8, P, vec>
	{
		GLM_FUNC_QUALIFIER static vec<4, uint8, P> call()
		{
			return vec<4, uint8, P>(
				std::rand() % std::numeric_limits<uint8>::max(),
				std::rand() % std::numeric_limits<uint8>::max(),
				std::rand() % std::numeric_limits<uint8>::max(),
				std::rand() % std::numeric_limits<uint8>::max());
		}
	};

	template<length_t L, precision P, template<length_t, typename, precision> class vecType>
	struct compute_rand<L, uint16, P, vecType>
	{
		GLM_FUNC_QUALIFIER static vecType<L, uint16, P> call()
		{
			return
				(vecType<L, uint16, P>(compute_rand<L, uint8, P, vecType>::call()) << static_cast<uint16>(8)) |
				(vecType<L, uint16, P>(compute_rand<L, uint8, P, vecType>::call()) << static_cast<uint16>(0));
		}
	};

	template<length_t L, precision P, template<length_t, typename, precision> class vecType>
	struct compute_rand<L, uint32, P, vecType>
	{
		GLM_FUNC_QUALIFIER static vecType<L, uint32, P> call()
		{
			return
				(vecType<L, uint32, P>(compute_rand<L, uint16, P, vecType>::call()) << static_cast<uint32>(16)) |
				(vecType<L, uint32, P>(compute_rand<L, uint16, P, vecType>::call()) << static_cast<uint32>(0));
		}
	};

	template<length_t L, precision P, template<length_t, typename, precision> class vecType>
	struct compute_rand<L, uint64, P, vecType>
	{
		GLM_FUNC_QUALIFIER static vecType<L, uint64, P> call()
		{
			return
				(vecType<L, uint64, P>(compute_rand<L, uint32, P, vecType>::call()) << static_cast<uint64>(32)) |
				(vecType<L, uint64, P>(compute_rand<L, uint32, P, vecType>::call()) << static_cast<uint64>(0));
		}
	};

	template<length_t L, typename T, precision P, template<length_t, typename, precision> class vecType>
	struct compute_linearRand
	{
		GLM_FUNC_QUALIFIER static vecType<L, T, P> call(vecType<L, T, P> const & Min, vecType<L, T, P> const & Max);
	};

	template<length_t L, precision P, template<length_t, typename, precision> class vecType>
	struct compute_linearRand<L, int8, P, vecType>
	{
		GLM_FUNC_QUALIFIER static vecType<L, int8, P> call(vecType<L, int8, P> const & Min, vecType<L, int8, P> const & Max)
		{
			return (vecType<L, int8, P>(compute_rand<L, uint8, P, vecType>::call() % vecType<L, uint8, P>(Max + static_cast<int8>(1) - Min))) + Min;
		}
	};

	template<length_t L, precision P, template<length_t, typename, precision> class vecType>
	struct compute_linearRand<L, uint8, P, vecType>
	{
		GLM_FUNC_QUALIFIER static vecType<L, uint8, P> call(vecType<L, uint8, P> const & Min, vecType<L, uint8, P> const & Max)
		{
			return (compute_rand<L, uint8, P, vecType>::call() % (Max + static_cast<uint8>(1) - Min)) + Min;
		}
	};

	template<length_t L, precision P, template<length_t, typename, precision> class vecType>
	struct compute_linearRand<L, int16, P, vecType>
	{
		GLM_FUNC_QUALIFIER static vecType<L, int16, P> call(vecType<L, int16, P> const & Min, vecType<L, int16, P> const & Max)
		{
			return (vecType<L, int16, P>(compute_rand<L, uint16, P, vecType>::call() % vecType<L, uint16, P>(Max + static_cast<int16>(1) - Min))) + Min;
		}
	};

	template<length_t L, precision P, template<length_t, typename, precision> class vecType>
	struct compute_linearRand<L, uint16, P, vecType>
	{
		GLM_FUNC_QUALIFIER static vecType<L, uint16, P> call(vecType<L, uint16, P> const & Min, vecType<L, uint16, P> const & Max)
		{
			return (compute_rand<L, uint16, P, vecType>::call() % (Max + static_cast<uint16>(1) - Min)) + Min;
		}
	};

	template<length_t L, precision P, template<length_t, typename, precision> class vecType>
	struct compute_linearRand<L, int32, P, vecType>
	{
		GLM_FUNC_QUALIFIER static vecType<L, int32, P> call(vecType<L, int32, P> const & Min, vecType<L, int32, P> const & Max)
		{
			return (vecType<L, int32, P>(compute_rand<L, uint32, P, vecType>::call() % vecType<L, uint32, P>(Max + static_cast<int32>(1) - Min))) + Min;
		}
	};

	template<length_t L, precision P, template<length_t, typename, precision> class vecType>
	struct compute_linearRand<L, uint32, P, vecType>
	{
		GLM_FUNC_QUALIFIER static vecType<L, uint32, P> call(vecType<L, uint32, P> const & Min, vecType<L, uint32, P> const & Max)
		{
			return (compute_rand<L, uint32, P, vecType>::call() % (Max + static_cast<uint32>(1) - Min)) + Min;
		}
	};
 
	template<length_t L, precision P, template<length_t, typename, precision> class vecType>
	struct compute_linearRand<L, int64, P, vecType>
	{
		GLM_FUNC_QUALIFIER static vecType<L, int64, P> call(vecType<L, int64, P> const & Min, vecType<L, int64, P> const & Max)
		{
			return (vecType<L, int64, P>(compute_rand<L, uint64, P, vecType>::call() % vecType<L, uint64, P>(Max + static_cast<int64>(1) - Min))) + Min;
		}
	};

	template<length_t L, precision P, template<length_t, typename, precision> class vecType>
	struct compute_linearRand<L, uint64, P, vecType>
	{
		GLM_FUNC_QUALIFIER static vecType<L, uint64, P> call(vecType<L, uint64, P> const & Min, vecType<L, uint64, P> const & Max)
		{
			return (compute_rand<L, uint64, P, vecType>::call() % (Max + static_cast<uint64>(1) - Min)) + Min;
		}
	};

	template<length_t L, template<length_t, typename, precision> class vecType>
	struct compute_linearRand<L, float, lowp, vecType>
	{
		GLM_FUNC_QUALIFIER static vecType<L, float, lowp> call(vecType<L, float, lowp> const & Min, vecType<L, float, lowp> const & Max)
		{
			return vecType<L, float, lowp>(compute_rand<L, uint8, lowp, vecType>::call()) / static_cast<float>(std::numeric_limits<uint8>::max()) * (Max - Min) + Min;
		}
	};

	template<length_t L, template<length_t, typename, precision> class vecType>
	struct compute_linearRand<L, float, mediump, vecType>
	{
		GLM_FUNC_QUALIFIER static vecType<L, float, mediump> call(vecType<L, float, mediump> const & Min, vecType<L, float, mediump> const & Max)
		{
			return vecType<L, float, mediump>(compute_rand<L, uint16, mediump, vecType>::call()) / static_cast<float>(std::numeric_limits<uint16>::max()) * (Max - Min) + Min;
		}
	};

	template<length_t L, template<length_t, typename, precision> class vecType>
	struct compute_linearRand<L, float, highp, vecType>
	{
		GLM_FUNC_QUALIFIER static vecType<L, float, highp> call(vecType<L, float, highp> const & Min, vecType<L, float, highp> const & Max)
		{
			return vecType<L, float, highp>(compute_rand<L, uint32, highp, vecType>::call()) / static_cast<float>(std::numeric_limits<uint32>::max()) * (Max - Min) + Min;
		}
	};

	template<length_t L, template<length_t, typename, precision> class vecType>
	struct compute_linearRand<L, double, lowp, vecType>
	{
		GLM_FUNC_QUALIFIER static vecType<L, double, lowp> call(vecType<L, double, lowp> const & Min, vecType<L, double, lowp> const & Max)
		{
			return vecType<L, double, lowp>(compute_rand<L, uint16, lowp, vecType>::call()) / static_cast<double>(std::numeric_limits<uint16>::max()) * (Max - Min) + Min;
		}
	};

	template<length_t L, template<length_t, typename, precision> class vecType>
	struct compute_linearRand<L, double, mediump, vecType>
	{
		GLM_FUNC_QUALIFIER static vecType<L, double, mediump> call(vecType<L, double, mediump> const & Min, vecType<L, double, mediump> const & Max)
		{
			return vecType<L, double, mediump>(compute_rand<L, uint32, mediump, vecType>::call()) / static_cast<double>(std::numeric_limits<uint32>::max()) * (Max - Min) + Min;
		}
	};

	template<length_t L, template<length_t, typename, precision> class vecType>
	struct compute_linearRand<L, double, highp, vecType>
	{
		GLM_FUNC_QUALIFIER static vecType<L, double, highp> call(vecType<L, double, highp> const & Min, vecType<L, double, highp> const & Max)
		{
			return vecType<L, double, highp>(compute_rand<L, uint64, highp, vecType>::call()) / static_cast<double>(std::numeric_limits<uint64>::max()) * (Max - Min) + Min;
		}
	};

	template<length_t L, template<length_t, typename, precision> class vecType>
	struct compute_linearRand<L, long double, lowp, vecType>
	{
		GLM_FUNC_QUALIFIER static vecType<L, long double, lowp> call(vecType<L, long double, lowp> const & Min, vecType<L, long double, lowp> const & Max)
		{
			return vecType<L, long double, lowp>(compute_rand<L, uint32, lowp, vecType>::call()) / static_cast<long double>(std::numeric_limits<uint32>::max()) * (Max - Min) + Min;
		}
	};

	template<length_t L, template<length_t, typename, precision> class vecType>
	struct compute_linearRand<L, long double, mediump, vecType>
	{
		GLM_FUNC_QUALIFIER static vecType<L, long double, mediump> call(vecType<L, long double, mediump> const & Min, vecType<L, long double, mediump> const & Max)
		{
			return vecType<L, long double, mediump>(compute_rand<L, uint64, mediump, vecType>::call()) / static_cast<long double>(std::numeric_limits<uint64>::max()) * (Max - Min) + Min;
		}
	};

	template<length_t L, template<length_t, typename, precision> class vecType>
	struct compute_linearRand<L, long double, highp, vecType>
	{
		GLM_FUNC_QUALIFIER static vecType<L, long double, highp> call(vecType<L, long double, highp> const & Min, vecType<L, long double, highp> const & Max)
		{
			return vecType<L, long double, highp>(compute_rand<L, uint64, highp, vecType>::call()) / static_cast<long double>(std::numeric_limits<uint64>::max()) * (Max - Min) + Min;
		}
	};
}//namespace detail

	template<typename genType>
	GLM_FUNC_QUALIFIER genType linearRand(genType Min, genType Max)
	{
		return detail::compute_linearRand<1, genType, highp, vec>::call(
			vec<1, genType, highp>(Min),
			vec<1, genType, highp>(Max)).x;
	}

	template<length_t L, typename T, precision P, template<length_t, typename, precision> class vecType>
	GLM_FUNC_QUALIFIER vecType<L, T, P> linearRand(vecType<L, T, P> const & Min, vecType<L, T, P> const & Max)
	{
		return detail::compute_linearRand<L, T, P, vecType>::call(Min, Max);
	}

	template<typename genType>
	GLM_FUNC_QUALIFIER genType gaussRand(genType Mean, genType Deviation)
	{
		genType w, x1, x2;
	
		do
		{
			x1 = linearRand(genType(-1), genType(1));
			x2 = linearRand(genType(-1), genType(1));
		
			w = x1 * x1 + x2 * x2;
		} while(w > genType(1));
	
		return x2 * Deviation * Deviation * sqrt((genType(-2) * log(w)) / w) + Mean;
	}

	template<length_t L, typename T, precision P, template<length_t, typename, precision> class vecType>
	GLM_FUNC_QUALIFIER vecType<L, T, P> gaussRand(vecType<L, T, P> const & Mean, vecType<L, T, P> const & Deviation)
	{
		return detail::functor2<L, T, P>::call(gaussRand, Mean, Deviation);
	}

	template<typename T>
	GLM_FUNC_QUALIFIER vec<2, T, defaultp> diskRand(T Radius)
	{		
		vec<2, T, defaultp> Result(T(0));
		T LenRadius(T(0));
		
		do
		{
			Result = linearRand(
				vec<2, T, defaultp>(-Radius),
				vec<2, T, defaultp>(Radius));
			LenRadius = length(Result);
		}
		while(LenRadius > Radius);
		
		return Result;
	}
	
	template<typename T>
	GLM_FUNC_QUALIFIER vec<3, T, defaultp> ballRand(T Radius)
	{		
		vec<3, T, defaultp> Result(T(0));
		T LenRadius(T(0));
		
		do
		{
			Result = linearRand(
				vec<3, T, defaultp>(-Radius),
				vec<3, T, defaultp>(Radius));
			LenRadius = length(Result);
		}
		while(LenRadius > Radius);
		
		return Result;
	}
	
	template<typename T>
	GLM_FUNC_QUALIFIER vec<2, T, defaultp> circularRand(T Radius)
	{
		T a = linearRand(T(0), T(6.283185307179586476925286766559f));
		return vec<2, T, defaultp>(cos(a), sin(a)) * Radius;		
	}
	
	template<typename T>
	GLM_FUNC_QUALIFIER vec<3, T, defaultp> sphericalRand(T Radius)
	{
		T z = linearRand(T(-1), T(1));
		T a = linearRand(T(0), T(6.283185307179586476925286766559f));
	
		T r = sqrt(T(1) - z * z);
	
		T x = r * cos(a);
		T y = r * sin(a);
	
		return vec<3, T, defaultp>(x, y, z) * Radius;	
	}
}//namespace glm
