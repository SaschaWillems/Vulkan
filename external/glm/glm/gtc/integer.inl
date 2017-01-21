/// @ref gtc_integer
/// @file glm/gtc/integer.inl

namespace glm{
namespace detail
{
	template<length_t L, typename T, precision P, template<length_t, typename, precision> class vecType, bool Aligned>
	struct compute_log2<L, T, P, vecType, false, Aligned>
	{
		GLM_FUNC_QUALIFIER static vecType<L, T, P> call(vecType<L, T, P> const& v)
		{
			//Equivalent to return findMSB(vec); but save one function call in ASM with VC
			//return findMSB(vec);
			return vecType<L, T, P>(detail::compute_findMSB_vec<L, T, P, vecType, sizeof(T) * 8>::call(v));
		}
	};

#	if GLM_HAS_BITSCAN_WINDOWS
		template<precision P, bool Aligned>
		struct compute_log2<4, int, P, vec, false, Aligned>
		{
			GLM_FUNC_QUALIFIER static vec<4, int, P> call(vec<4, int, P> const& v)
			{
				vec<4, int, P> Result(glm::uninitialize);

				_BitScanReverse(reinterpret_cast<unsigned long*>(&Result.x), v.x);
				_BitScanReverse(reinterpret_cast<unsigned long*>(&Result.y), v.y);
				_BitScanReverse(reinterpret_cast<unsigned long*>(&Result.z), v.z);
				_BitScanReverse(reinterpret_cast<unsigned long*>(&Result.w), v.w);

				return Result;
			}
		};
#	endif//GLM_HAS_BITSCAN_WINDOWS
}//namespace detail
	template<typename genType>
	GLM_FUNC_QUALIFIER int iround(genType x)
	{
		GLM_STATIC_ASSERT(std::numeric_limits<genType>::is_iec559, "'iround' only accept floating-point inputs");
		assert(static_cast<genType>(0.0) <= x);

		return static_cast<int>(x + static_cast<genType>(0.5));
	}

	template<glm::length_t L, typename T, precision P, template<glm::length_t, typename, precision> class vecType>
	GLM_FUNC_QUALIFIER vecType<L, int, P> iround(vecType<L, T, P> const& x)
	{
		GLM_STATIC_ASSERT(std::numeric_limits<T>::is_iec559, "'iround' only accept floating-point inputs");
		assert(all(lessThanEqual(vecType<L, T, P>(0), x)));

		return vecType<L, int, P>(x + static_cast<T>(0.5));
	}

	template<typename genType>
	GLM_FUNC_QUALIFIER uint uround(genType x)
	{
		GLM_STATIC_ASSERT(std::numeric_limits<genType>::is_iec559, "'uround' only accept floating-point inputs");
		assert(static_cast<genType>(0.0) <= x);

		return static_cast<uint>(x + static_cast<genType>(0.5));
	}

	template<glm::length_t L, typename T, precision P, template<glm::length_t, typename, precision> class vecType>
	GLM_FUNC_QUALIFIER vecType<L, uint, P> uround(vecType<L, T, P> const& x)
	{
		GLM_STATIC_ASSERT(std::numeric_limits<T>::is_iec559, "'uround' only accept floating-point inputs");
		assert(all(lessThanEqual(vecType<L, T, P>(0), x)));

		return vecType<L, uint, P>(x + static_cast<T>(0.5));
	}
}//namespace glm
