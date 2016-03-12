/////////////////////////////////////////////////////////////////////////////////////////
/// OpenGL Mathematics (glm.g-truc.net)
///
/// Copyright (c) 2005 - 2015 G-Truc Creation (www.g-truc.net)
/// Permission is hereby granted, free of charge, to any person obtaining a copy
/// of this software and associated documentation files (the "Software"), to deal
/// in the Software without restriction, including without limitation the rights
/// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
/// copies of the Software, and to permit persons to whom the Software is
/// furnished to do so, subject to the following conditions:
/// 
/// The above copyright notice and this permission notice shall be included in
/// all copies or substantial portions of the Software.
/// 
/// Restrictions:
///		By making use of the Software for military purposes, you choose to make
///		a Bunny unhappy.
/// 
/// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
/// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
/// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
/// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
/// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
/// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
/// THE SOFTWARE.
///
/// @ref gtc_color_space
/// @file glm/gtc/color_space.inl
/// @date 2015-02-10 / 2015-08-02
/// @author Christophe Riccio
///////////////////////////////////////////////////////////////////////////////////////////////////

namespace glm{
namespace detail
{
	template <typename T, precision P, template <typename, precision> class vecType>
	struct compute_rgbToSrgb
	{
		GLM_FUNC_QUALIFIER static vecType<T, P> call(vecType<T, P> const & ColorRGB, T GammaCorrection)
		{
			vecType<T, P> const ClampedColor(clamp(ColorRGB, static_cast<T>(0), static_cast<T>(1)));

			return mix(
				pow(ClampedColor, vecType<T, P>(GammaCorrection)) * static_cast<T>(1.055) - static_cast<T>(0.055),
				ClampedColor * static_cast<T>(12.92),
				lessThan(ClampedColor, vecType<T, P>(static_cast<T>(0.0031308))));
		}
	};

	template <typename T, precision P>
	struct compute_rgbToSrgb<T, P, tvec4>
	{
		GLM_FUNC_QUALIFIER static tvec4<T, P> call(tvec4<T, P> const & ColorRGB, T GammaCorrection)
		{
			return tvec4<T, P>(compute_rgbToSrgb<T, P, tvec3>::call(tvec3<T, P>(ColorRGB), GammaCorrection), ColorRGB.a);
		}
	};

	template <typename T, precision P, template <typename, precision> class vecType>
	struct compute_srgbToRgb
	{
		GLM_FUNC_QUALIFIER static vecType<T, P> call(vecType<T, P> const & ColorSRGB, T Gamma)
		{
			return mix(
				pow((ColorSRGB + static_cast<T>(0.055)) * static_cast<T>(0.94786729857819905213270142180095), vecType<T, P>(Gamma)),
				ColorSRGB * static_cast<T>(0.07739938080495356037151702786378),
				lessThanEqual(ColorSRGB, vecType<T, P>(static_cast<T>(0.04045))));
		}
	};

	template <typename T, precision P>
	struct compute_srgbToRgb<T, P, tvec4>
	{
		GLM_FUNC_QUALIFIER static tvec4<T, P> call(tvec4<T, P> const & ColorSRGB, T Gamma)
		{
			return tvec4<T, P>(compute_srgbToRgb<T, P, tvec3>::call(tvec3<T, P>(ColorSRGB), Gamma), ColorSRGB.a);
		}
	};
}//namespace detail

	template <typename T, precision P, template <typename, precision> class vecType>
	GLM_FUNC_QUALIFIER vecType<T, P> convertLinearToSRGB(vecType<T, P> const & ColorLinear)
	{
		return detail::compute_rgbToSrgb<T, P, vecType>::call(ColorLinear, static_cast<T>(0.41666));
	}

	template <typename T, precision P, template <typename, precision> class vecType>
	GLM_FUNC_QUALIFIER vecType<T, P> convertLinearToSRGB(vecType<T, P> const & ColorLinear, T Gamma)
	{
		return detail::compute_rgbToSrgb<T, P, vecType>::call(ColorLinear, static_cast<T>(1) / Gamma);
	}

	template <typename T, precision P, template <typename, precision> class vecType>
	GLM_FUNC_QUALIFIER vecType<T, P> convertSRGBToLinear(vecType<T, P> const & ColorSRGB)
	{
		return detail::compute_srgbToRgb<T, P, vecType>::call(ColorSRGB, static_cast<T>(2.4));
	}
	
	template <typename T, precision P, template <typename, precision> class vecType>
	GLM_FUNC_QUALIFIER vecType<T, P> convertSRGBToLinear(vecType<T, P> const & ColorSRGB, T Gamma)
	{
		return detail::compute_srgbToRgb<T, P, vecType>::call(ColorSRGB, Gamma);
	}
}//namespace glm
