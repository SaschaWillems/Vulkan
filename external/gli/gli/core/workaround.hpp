#pragma once

// Removed after upgrading to GLM 0.9.8
namespace gli{
namespace workaround{
namespace detail
{
	union u3u3u2
	{
		struct
		{
			uint x : 3;
			uint y : 3;
			uint z : 2;
		} data;
		uint8 pack;
	};

	union u4u4
	{
		struct
		{
			uint x : 4;
			uint y : 4;
		} data;
		uint8 pack;
	};

	union u4u4u4u4
	{
		struct
		{
			uint x : 4;
			uint y : 4;
			uint z : 4;
			uint w : 4;
		} data;
		uint16 pack;
	};

	union u5u6u5
	{
		struct
		{
			uint x : 5;
			uint y : 6;
			uint z : 5;
		} data;
		uint16 pack;
	};

	union u5u5u5u1
	{
		struct
		{
			uint x : 5;
			uint y : 5;
			uint z : 5;
			uint w : 1;
		} data;
		uint16 pack;
	};

	union u9u9u9e5
	{
		struct
		{
			uint x : 9;
			uint y : 9;
			uint z : 9;
			uint w : 5;
		} data;
		uint32 pack;
	};

	template <typename T, typename floatType, precision P, template <typename, precision> class vecType, bool isInteger, bool signedType>
	struct compute_compNormalize
	{};

	template <typename T, typename floatType, precision P, template <typename, precision> class vecType>
	struct compute_compNormalize<T, floatType, P, vecType, true, true>
	{
		GLM_FUNC_QUALIFIER static vecType<floatType, P> call(vecType<T, P> const & v)
		{
			floatType const Min = static_cast<floatType>(std::numeric_limits<T>::min());
			floatType const Max = static_cast<floatType>(std::numeric_limits<T>::max());
			return (vecType<floatType, P>(v) - Min) / (Max - Min) * static_cast<floatType>(2) - static_cast<floatType>(1);
		}
	};

	template <typename T, typename floatType, precision P, template <typename, precision> class vecType>
	struct compute_compNormalize<T, floatType, P, vecType, true, false>
	{
		GLM_FUNC_QUALIFIER static vecType<floatType, P> call(vecType<T, P> const & v)
		{
			return vecType<floatType, P>(v) / static_cast<floatType>(std::numeric_limits<T>::max());
		}
	};

	template <typename T, typename floatType, precision P, template <typename, precision> class vecType>
	struct compute_compNormalize<T, floatType, P, vecType, false, true>
	{
		GLM_FUNC_QUALIFIER static vecType<floatType, P> call(vecType<T, P> const & v)
		{
			return v;
		}
	};

	template <typename T, typename floatType, precision P, template <typename, precision> class vecType, bool isInteger, bool signedType>
	struct compute_compScale
	{};

	template <typename T, typename floatType, precision P, template <typename, precision> class vecType>
	struct compute_compScale<T, floatType, P, vecType, true, true>
	{
		GLM_FUNC_QUALIFIER static vecType<T, P> call(vecType<floatType, P> const & v)
		{
			floatType const Max = static_cast<floatType>(std::numeric_limits<T>::max()) + static_cast<floatType>(0.5);
			vecType<floatType, P> const Scaled(v * Max);
			vecType<T, P> const Result(Scaled - static_cast<floatType>(0.5));
			return Result;
		}
	};

	template <typename T, typename floatType, precision P, template <typename, precision> class vecType>
	struct compute_compScale<T, floatType, P, vecType, true, false>
	{
		GLM_FUNC_QUALIFIER static vecType<T, P> call(vecType<floatType, P> const & v)
		{
			return vecType<T, P>(vecType<floatType, P>(v) * static_cast<floatType>(std::numeric_limits<T>::max()));
		}
	};

	template <typename T, typename floatType, precision P, template <typename, precision> class vecType>
	struct compute_compScale<T, floatType, P, vecType, false, true>
	{
		GLM_FUNC_QUALIFIER static vecType<T, P> call(vecType<floatType, P> const & v)
		{
			return v;
		}
	};

	template <precision P, template <typename, precision> class vecType>
	struct compute_half
	{};

	template <precision P>
	struct compute_half<P, tvec1>
	{
		GLM_FUNC_QUALIFIER static tvec1<uint16, P> pack(tvec1<float, P> const & v)
		{
			int16 const Unpacked(glm::detail::toFloat16(v.x));
			return tvec1<uint16, P>(reinterpret_cast<uint16 const &>(Unpacked));
		}

		GLM_FUNC_QUALIFIER static tvec1<float, P> unpack(tvec1<uint16, P> const & v)
		{
			return tvec1<float, P>(glm::detail::toFloat32(reinterpret_cast<int16 const &>(v.x)));
		}
	};

	template <precision P>
	struct compute_half<P, tvec2>
	{
		GLM_FUNC_QUALIFIER static tvec2<uint16, P> pack(tvec2<float, P> const & v)
		{
			tvec2<int16, P> const Unpacked(glm::detail::toFloat16(v.x), glm::detail::toFloat16(v.y));
			return tvec2<uint16, P>(
				reinterpret_cast<uint16 const &>(Unpacked.x),
				reinterpret_cast<uint16 const &>(Unpacked.y));
		}

		GLM_FUNC_QUALIFIER static tvec2<float, P> unpack(tvec2<uint16, P> const & v)
		{
			return tvec2<float, P>(
				glm::detail::toFloat32(reinterpret_cast<int16 const &>(v.x)),
				glm::detail::toFloat32(reinterpret_cast<int16 const &>(v.y)));
		}
	};

	template <precision P>
	struct compute_half<P, tvec3>
	{
		GLM_FUNC_QUALIFIER static tvec3<uint16, P> pack(tvec3<float, P> const & v)
		{
			tvec3<int16, P> const Unpacked(glm::detail::toFloat16(v.x), glm::detail::toFloat16(v.y), glm::detail::toFloat16(v.z));
			return tvec3<uint16, P>(
				reinterpret_cast<uint16 const &>(Unpacked.x),
				reinterpret_cast<uint16 const &>(Unpacked.y),
				reinterpret_cast<uint16 const &>(Unpacked.z));
		}

		GLM_FUNC_QUALIFIER static tvec3<float, P> unpack(tvec3<uint16, P> const & v)
		{
			return tvec3<float, P>(
				glm::detail::toFloat32(reinterpret_cast<int16 const &>(v.x)),
				glm::detail::toFloat32(reinterpret_cast<int16 const &>(v.y)),
				glm::detail::toFloat32(reinterpret_cast<int16 const &>(v.z)));
		}
	};

	template <precision P>
	struct compute_half<P, tvec4>
	{
		GLM_FUNC_QUALIFIER static vec<4, uint16, P> pack(vec<4, float, P> const & v)
		{
			vec<4, int16, P> const Unpacked(glm::detail::toFloat16(v.x), glm::detail::toFloat16(v.y), glm::detail::toFloat16(v.z), glm::detail::toFloat16(v.w));
			return vec<4, uint16, P>(
				reinterpret_cast<uint16 const &>(Unpacked.x),
				reinterpret_cast<uint16 const &>(Unpacked.y),
				reinterpret_cast<uint16 const &>(Unpacked.z),
				reinterpret_cast<uint16 const &>(Unpacked.w));
		}

		GLM_FUNC_QUALIFIER static vec<4, float, P> unpack(vec<4, uint16, P> const & v)
		{
			return vec<4, float, P>(
				glm::detail::toFloat32(reinterpret_cast<int16 const &>(v.x)),
				glm::detail::toFloat32(reinterpret_cast<int16 const &>(v.y)),
				glm::detail::toFloat32(reinterpret_cast<int16 const &>(v.z)),
				glm::detail::toFloat32(reinterpret_cast<int16 const &>(v.w)));
		}
	};
}//namespace detail

	template <typename floatType, typename T, precision P, template <typename, precision> class vecType>
	GLM_FUNC_QUALIFIER vecType<floatType, P> compNormalize(vecType<T, P> const & v)
	{
		GLM_STATIC_ASSERT(std::numeric_limits<floatType>::is_iec559, "'compNormalize' accepts only floating-point types for 'floatType' template parameter");

		return detail::compute_compNormalize<T, floatType, P, vecType, std::numeric_limits<T>::is_integer, std::numeric_limits<T>::is_signed>::call(v);
	}

	template <typename T, typename floatType, precision P, template <typename, precision> class vecType>
	GLM_FUNC_QUALIFIER vecType<T, P> compScale(vecType<floatType, P> const & v)
	{
		GLM_STATIC_ASSERT(std::numeric_limits<floatType>::is_iec559, "'compScale' accepts only floating-point types for 'floatType' template parameter");

		return detail::compute_compScale<T, floatType, P, vecType, std::numeric_limits<T>::is_integer, std::numeric_limits<T>::is_signed>::call(v);
	}

	template <typename uintType, typename floatType, precision P, template <typename, precision> class vecType>
	GLM_FUNC_QUALIFIER vecType<uintType, P> packUnorm(vecType<floatType, P> const & v)
	{
		GLM_STATIC_ASSERT(std::numeric_limits<uintType>::is_integer, "uintType must be an integer type");
		GLM_STATIC_ASSERT(std::numeric_limits<floatType>::is_iec559, "floatType must be a floating point type");

		return vecType<uintType, P>(round(clamp(v, static_cast<floatType>(0), static_cast<floatType>(1)) * static_cast<floatType>(std::numeric_limits<uintType>::max())));
	}

	template <typename uintType, typename floatType, precision P, template <typename, precision> class vecType>
	GLM_FUNC_QUALIFIER vecType<floatType, P> unpackUnorm(vecType<uintType, P> const & v)
	{
		GLM_STATIC_ASSERT(std::numeric_limits<uintType>::is_integer, "uintType must be an integer type");
		GLM_STATIC_ASSERT(std::numeric_limits<floatType>::is_iec559, "floatType must be a floating point type");

		return vecType<float, P>(v) * (static_cast<floatType>(1) / static_cast<floatType>(std::numeric_limits<uintType>::max()));
	}

	GLM_FUNC_QUALIFIER uint8 packUnorm2x3_1x2(vec3 const & v)
	{
		u32vec3 const Unpack(round(clamp(v, 0.0f, 1.0f) * vec3(7.f, 7.f, 3.f)));
		detail::u3u3u2 Result;
		Result.data.x = Unpack.x;
		Result.data.y = Unpack.y;
		Result.data.z = Unpack.z;
		return Result.pack;
	}

	GLM_FUNC_QUALIFIER vec3 unpackUnorm2x3_1x2(uint8 v)
	{
		vec3 const ScaleFactor(1.f / 7.f, 1.f / 7.f, 1.f / 3.f);
		detail::u3u3u2 Unpack;
		Unpack.pack = v;
		return vec3(Unpack.data.x, Unpack.data.y, Unpack.data.z) * ScaleFactor;
	}

	GLM_FUNC_QUALIFIER uint8 packUnorm2x4(vec2 const & v)
	{
		u32vec2 const Unpack(round(clamp(v, 0.0f, 1.0f) * 15.0f));
		detail::u4u4 Result;
		Result.data.x = Unpack.x;
		Result.data.y = Unpack.y;
		return Result.pack;
	}

	GLM_FUNC_QUALIFIER vec2 unpackUnorm2x4(uint8 v)
	{
		float const ScaleFactor(1.f / 15.f);
		detail::u4u4 Unpack;
		Unpack.pack = v;
		return vec2(Unpack.data.x, Unpack.data.y) * ScaleFactor;
	}

	GLM_FUNC_QUALIFIER uint16 packUnorm4x4(vec4 const & v)
	{
		u32vec4 const Unpack(round(clamp(v, 0.0f, 1.0f) * 15.0f));
		detail::u4u4u4u4 Result;
		Result.data.x = Unpack.x;
		Result.data.y = Unpack.y;
		Result.data.z = Unpack.z;
		Result.data.w = Unpack.w;
		return Result.pack;
	}

	GLM_FUNC_QUALIFIER vec4 unpackUnorm4x4(uint16 v)
	{
		float const ScaleFactor(1.f / 15.f);
		detail::u4u4u4u4 Unpack;
		Unpack.pack = v;
		return vec4(Unpack.data.x, Unpack.data.y, Unpack.data.z, Unpack.data.w) * ScaleFactor;
	}

	GLM_FUNC_QUALIFIER uint16 packUnorm3x5_1x1(vec4 const & v)
	{
		u32vec4 const Unpack(round(clamp(v, 0.0f, 1.0f) * vec4(15.f, 15.f, 15.f, 1.f)));
		detail::u5u5u5u1 Result;
		Result.data.x = Unpack.x;
		Result.data.y = Unpack.y;
		Result.data.z = Unpack.z;
		Result.data.w = Unpack.w;
		return Result.pack;
	}

	GLM_FUNC_QUALIFIER vec4 unpackUnorm3x5_1x1(uint16 v)
	{
		vec4 const ScaleFactor(1.f / 15.f, 1.f / 15.f, 1.f / 15.f, 1.f);
		detail::u5u5u5u1 Unpack;
		Unpack.pack = v;
		return vec4(Unpack.data.x, Unpack.data.y, Unpack.data.z, Unpack.data.w) * ScaleFactor;
	}

	GLM_FUNC_QUALIFIER uint16 packUnorm1x5_1x6_1x5(vec3 const & v)
	{
		u32vec3 const Unpack(round(clamp(v, 0.0f, 1.0f) * vec3(15.f, 31.f, 15.f)));
		detail::u5u6u5 Result;
		Result.data.x = Unpack.x;
		Result.data.y = Unpack.y;
		Result.data.z = Unpack.z;
		return Result.pack;
	}

	GLM_FUNC_QUALIFIER vec3 unpackUnorm1x5_1x6_1x5(uint16 v)
	{
		vec3 const ScaleFactor(1.f / 15.f, 1.f / 31.f, 1.f / 15.f);
		detail::u5u6u5 Unpack;
		Unpack.pack = v;
		return vec3(Unpack.data.x, Unpack.data.y, Unpack.data.z) * ScaleFactor;
	}

	GLM_FUNC_QUALIFIER uint32 packF3x9_E1x5(vec3 const & v)
	{
		float const SharedExpMax = (pow(2.0f, 9.0f - 1.0f) / pow(2.0f, 9.0f)) * pow(2.0f, 31.f - 15.f);
		vec3 const Color = clamp(v, 0.0f, SharedExpMax);
		float const MaxColor = max(Color.x, max(Color.y, Color.z));

		float const ExpSharedP = max(-15.f - 1.f, floor(log2(MaxColor))) + 1.0f + 15.f;
		float const MaxShared = floor(MaxColor / pow(2.0f, (ExpSharedP - 16.f - 9.f)) + 0.5f);
		float const ExpShared = MaxShared == pow(2.0f, 9.0f) ? ExpSharedP + 1.0f : ExpSharedP;

		uvec3 const ColorComp(floor(Color / pow(2.f, (ExpShared - 15.f - 9.f)) + 0.5f));

		detail::u9u9u9e5 Unpack;
		Unpack.data.x = ColorComp.x;
		Unpack.data.y = ColorComp.y;
		Unpack.data.z = ColorComp.z;
		Unpack.data.w = uint(ExpShared);
		return Unpack.pack;
	}

	GLM_FUNC_QUALIFIER vec3 unpackF3x9_E1x5(uint32 v)
	{
		detail::u9u9u9e5 Unpack;
		Unpack.pack = v;

		return vec3(Unpack.data.x, Unpack.data.y, Unpack.data.z) * pow(2.0f, Unpack.data.w - 15.f - 9.f);
	}

	template <precision P, template <typename, precision> class vecType>
	GLM_FUNC_QUALIFIER vecType<uint16, P> packHalf(vecType<float, P> const & v)
	{
		return detail::compute_half<P, vecType>::pack(v);
	}

	template <precision P, template <typename, precision> class vecType>
	GLM_FUNC_QUALIFIER vecType<float, P> unpackHalf(vecType<uint16, P> const & v)
	{
		return detail::compute_half<P, vecType>::unpack(v);
	}
}//namespace workaround
}//namespace gli

