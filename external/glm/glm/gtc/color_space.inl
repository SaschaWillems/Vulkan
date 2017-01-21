/// @ref gtc_color_space
/// @file glm/gtc/color_space.inl

namespace glm{
namespace detail
{
	template<length_t L, typename T, precision P, template<length_t, typename, precision> class vecType>
	struct compute_rgbToSrgb
	{
		GLM_FUNC_QUALIFIER static vecType<L, T, P> call(vecType<L, T, P> const& ColorRGB, T GammaCorrection)
		{
			vecType<L, T, P> const ClampedColor(clamp(ColorRGB, static_cast<T>(0), static_cast<T>(1)));

			return mix(
				pow(ClampedColor, vecType<L, T, P>(GammaCorrection)) * static_cast<T>(1.055) - static_cast<T>(0.055),
				ClampedColor * static_cast<T>(12.92),
				lessThan(ClampedColor, vecType<L, T, P>(static_cast<T>(0.0031308))));
		}
	};

	template<typename T, precision P>
	struct compute_rgbToSrgb<4, T, P, vec>
	{
		GLM_FUNC_QUALIFIER static vec<4, T, P> call(vec<4, T, P> const& ColorRGB, T GammaCorrection)
		{
			return vec<4, T, P>(compute_rgbToSrgb<3, T, P, vec>::call(vec<3, T, P>(ColorRGB), GammaCorrection), ColorRGB.w);
		}
	};

	template<length_t L, typename T, precision P, template<length_t, typename, precision> class vecType>
	struct compute_srgbToRgb
	{
		GLM_FUNC_QUALIFIER static vecType<L, T, P> call(vecType<L, T, P> const& ColorSRGB, T Gamma)
		{
			return mix(
				pow((ColorSRGB + static_cast<T>(0.055)) * static_cast<T>(0.94786729857819905213270142180095), vecType<L, T, P>(Gamma)),
				ColorSRGB * static_cast<T>(0.07739938080495356037151702786378),
				lessThanEqual(ColorSRGB, vecType<L, T, P>(static_cast<T>(0.04045))));
		}
	};

	template<typename T, precision P>
	struct compute_srgbToRgb<4, T, P, vec>
	{
		GLM_FUNC_QUALIFIER static vec<4, T, P> call(vec<4, T, P> const& ColorSRGB, T Gamma)
		{
			return vec<4, T, P>(compute_srgbToRgb<3, T, P, vec>::call(vec<3, T, P>(ColorSRGB), Gamma), ColorSRGB.w);
		}
	};
}//namespace detail

	template<length_t L, typename T, precision P, template<length_t, typename, precision> class vecType>
	GLM_FUNC_QUALIFIER vecType<L, T, P> convertLinearToSRGB(vecType<L, T, P> const& ColorLinear)
	{
		return detail::compute_rgbToSrgb<L, T, P, vecType>::call(ColorLinear, static_cast<T>(0.41666));
	}

	// Based on Ian Taylor http://chilliant.blogspot.fr/2012/08/srgb-approximations-for-hlsl.html
	template<>
	GLM_FUNC_QUALIFIER vec<3, float, lowp> convertLinearToSRGB(vec<3, float, lowp> const& ColorLinear)
	{
		vec<3, float, lowp> S1 = sqrt(ColorLinear);
		vec<3, float, lowp> S2 = sqrt(S1);
		vec<3, float, lowp> S3 = sqrt(S2);
		return 0.662002687f * S1 + 0.684122060f * S2 - 0.323583601f * S3 - 0.0225411470f * ColorLinear;
	}

	template<length_t L, typename T, precision P, template<length_t, typename, precision> class vecType>
	GLM_FUNC_QUALIFIER vecType<L, T, P> convertLinearToSRGB(vecType<L, T, P> const& ColorLinear, T Gamma)
	{
		return detail::compute_rgbToSrgb<L, T, P, vecType>::call(ColorLinear, static_cast<T>(1) / Gamma);
	}

	template<length_t L, typename T, precision P, template<length_t, typename, precision> class vecType>
	GLM_FUNC_QUALIFIER vecType<L, T, P> convertSRGBToLinear(vecType<L, T, P> const& ColorSRGB)
	{
		return detail::compute_srgbToRgb<L, T, P, vecType>::call(ColorSRGB, static_cast<T>(2.4));
	}

	template<length_t L, typename T, precision P, template<length_t, typename, precision> class vecType>
	GLM_FUNC_QUALIFIER vecType<L, T, P> convertSRGBToLinear(vecType<L, T, P> const& ColorSRGB, T Gamma)
	{
		return detail::compute_srgbToRgb<L, T, P, vecType>::call(ColorSRGB, Gamma);
	}
}//namespace glm
