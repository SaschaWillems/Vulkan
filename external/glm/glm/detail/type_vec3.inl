///////////////////////////////////////////////////////////////////////////////////
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
/// @ref core
/// @file glm/detail/type_tvec3.inl
/// @date 2008-08-22 / 2011-06-15
/// @author Christophe Riccio
///////////////////////////////////////////////////////////////////////////////////

namespace glm
{

#	ifdef GLM_STATIC_CONST_MEMBERS
	template <typename T, precision P>
	const tvec3<T, P> tvec3<T, P>::ZERO(static_cast<T>(0), static_cast<T>(0), static_cast<T>(0));

	template <typename T, precision P>
	const tvec3<T, P> tvec3<T, P>::X(static_cast<T>(1), static_cast<T>(0), static_cast<T>(0));

	template <typename T, precision P>
	const tvec3<T, P> tvec3<T, P>::Y(static_cast<T>(0), static_cast<T>(1), static_cast<T>(0));

	template <typename T, precision P>
	const tvec3<T, P> tvec3<T, P>::Z(static_cast<T>(0), static_cast<T>(0), static_cast<T>(1));

	template <typename T, precision P>
	const tvec3<T, P> tvec3<T, P>::XY(static_cast<T>(1), static_cast<T>(1), static_cast<T>(0));

	template <typename T, precision P>
	const tvec3<T, P> tvec3<T, P>::XZ(static_cast<T>(1), static_cast<T>(0), static_cast<T>(1));

	template <typename T, precision P>
	const tvec3<T, P> tvec3<T, P>::YZ(static_cast<T>(0), static_cast<T>(1), static_cast<T>(1));

	template <typename T, precision P>
	const tvec3<T, P> tvec3<T, P>::XYZ(static_cast<T>(1), static_cast<T>(1), static_cast<T>(1));
#	endif
	// -- Implicit basic constructors --

#	if !GLM_HAS_DEFAULTED_FUNCTIONS || !defined(GLM_FORCE_NO_CTOR_INIT)
		template <typename T, precision P>
		GLM_FUNC_QUALIFIER tvec3<T, P>::tvec3()
#			ifndef GLM_FORCE_NO_CTOR_INIT 
				: x(0), y(0), z(0)
#			endif
		{}
#	endif//!GLM_HAS_DEFAULTED_FUNCTIONS

#	if !GLM_HAS_DEFAULTED_FUNCTIONS
		template <typename T, precision P>
		GLM_FUNC_QUALIFIER tvec3<T, P>::tvec3(tvec3<T, P> const & v)
			: x(v.x), y(v.y), z(v.z)
		{}
#	endif//!GLM_HAS_DEFAULTED_FUNCTIONS

	template <typename T, precision P>
	template <precision Q>
	GLM_FUNC_QUALIFIER tvec3<T, P>::tvec3(tvec3<T, Q> const & v)
		: x(v.x), y(v.y), z(v.z)
	{}

	// -- Explicit basic constructors --

	template <typename T, precision P>
	GLM_FUNC_QUALIFIER tvec3<T, P>::tvec3(ctor)
	{}

	template <typename T, precision P>
	GLM_FUNC_QUALIFIER tvec3<T, P>::tvec3(T const & scalar)
		: x(scalar), y(scalar), z(scalar)
	{}

	template <typename T, precision P>
	GLM_FUNC_QUALIFIER tvec3<T, P>::tvec3(T const & a, T const & b, T const & c)
		: x(a), y(b), z(c)
	{}

	// -- Conversion scalar constructors --

	template <typename T, precision P>
	template <typename A, typename B, typename C>
	GLM_FUNC_QUALIFIER tvec3<T, P>::tvec3(A const & a, B const & b, C const & c) :
		x(static_cast<T>(a)),
		y(static_cast<T>(b)),
		z(static_cast<T>(c))
	{}

	template <typename T, precision P>
	template <typename A, typename B, typename C>
	GLM_FUNC_QUALIFIER tvec3<T, P>::tvec3(tvec1<A, P> const & a, tvec1<B, P> const & b, tvec1<C, P> const & c) :
		x(static_cast<T>(a)),
		y(static_cast<T>(b)),
		z(static_cast<T>(c))
	{}

	// -- Conversion vector constructors --

	template <typename T, precision P>
	template <typename A, typename B, precision Q>
	GLM_FUNC_QUALIFIER tvec3<T, P>::tvec3(tvec2<A, Q> const & a, B const & b) :
		x(static_cast<T>(a.x)),
		y(static_cast<T>(a.y)),
		z(static_cast<T>(b))
	{}

	template <typename T, precision P>
	template <typename A, typename B, precision Q>
	GLM_FUNC_QUALIFIER tvec3<T, P>::tvec3(tvec2<A, Q> const & a, tvec1<B, Q> const & b) :
		x(static_cast<T>(a.x)),
		y(static_cast<T>(a.y)),
		z(static_cast<T>(b.x))
	{}

	template <typename T, precision P>
	template <typename A, typename B, precision Q>
	GLM_FUNC_QUALIFIER tvec3<T, P>::tvec3(A const & a, tvec2<B, Q> const & b) :
		x(static_cast<T>(a)),
		y(static_cast<T>(b.x)),
		z(static_cast<T>(b.y))
	{}

	template <typename T, precision P>
	template <typename A, typename B, precision Q>
	GLM_FUNC_QUALIFIER tvec3<T, P>::tvec3(tvec1<A, Q> const & a, tvec2<B, Q> const & b) :
		x(static_cast<T>(a.x)),
		y(static_cast<T>(b.x)),
		z(static_cast<T>(b.y))
	{}

	template <typename T, precision P>
	template <typename U, precision Q>
	GLM_FUNC_QUALIFIER tvec3<T, P>::tvec3(tvec3<U, Q> const & v) :
		x(static_cast<T>(v.x)),
		y(static_cast<T>(v.y)),
		z(static_cast<T>(v.z))
	{}

	template <typename T, precision P>
	template <typename U, precision Q>
	GLM_FUNC_QUALIFIER tvec3<T, P>::tvec3(tvec4<U, Q> const & v) :
		x(static_cast<T>(v.x)),
		y(static_cast<T>(v.y)),
		z(static_cast<T>(v.z))
	{}

	// -- Component accesses --

#	ifdef GLM_FORCE_SIZE_FUNC
		template <typename T, precision P>
		GLM_FUNC_QUALIFIER GLM_CONSTEXPR typename tvec3<T, P>::size_type tvec3<T, P>::size() const
		{
			return 3;
		}

		template <typename T, precision P>
		GLM_FUNC_QUALIFIER T & tvec3<T, P>::operator[](typename tvec3<T, P>::size_type i)
		{
			assert(i >= 0 && static_cast<detail::component_count_t>(i) < detail::component_count(*this));
			return (&x)[i];
		}

		template <typename T, precision P>
		GLM_FUNC_QUALIFIER T const & tvec3<T, P>::operator[](typename tvec3<T, P>::size_type i) const
		{
			assert(i >= 0 && static_cast<detail::component_count_t>(i) < detail::component_count(*this));
			return (&x)[i];
		}
#	else
		template <typename T, precision P>
		GLM_FUNC_QUALIFIER GLM_CONSTEXPR typename tvec3<T, P>::length_type tvec3<T, P>::length() const
		{
			return 3;
		}

		template <typename T, precision P>
		GLM_FUNC_QUALIFIER T & tvec3<T, P>::operator[](typename tvec3<T, P>::length_type i)
		{
			assert(i >= 0 && static_cast<detail::component_count_t>(i) < detail::component_count(*this));
			return (&x)[i];
		}

		template <typename T, precision P>
		GLM_FUNC_QUALIFIER T const & tvec3<T, P>::operator[](typename tvec3<T, P>::length_type i) const
		{
			assert(i >= 0 && static_cast<detail::component_count_t>(i) < detail::component_count(*this));
			return (&x)[i];
		}
#	endif//GLM_FORCE_SIZE_FUNC

	// -- Unary arithmetic operators --

#	if !GLM_HAS_DEFAULTED_FUNCTIONS
		template <typename T, precision P>
		GLM_FUNC_QUALIFIER tvec3<T, P>& tvec3<T, P>::operator=(tvec3<T, P> const & v)
		{
			this->x = v.x;
			this->y = v.y;
			this->z = v.z;
			return *this;
		}
#	endif//!GLM_HAS_DEFAULTED_FUNCTIONS

	template <typename T, precision P>
	template <typename U>
	GLM_FUNC_QUALIFIER tvec3<T, P>& tvec3<T, P>::operator=(tvec3<U, P> const & v)
	{
		this->x = static_cast<T>(v.x);
		this->y = static_cast<T>(v.y);
		this->z = static_cast<T>(v.z);
		return *this;
	}

	template <typename T, precision P>
	template <typename U>
	GLM_FUNC_QUALIFIER tvec3<T, P> & tvec3<T, P>::operator+=(U scalar)
	{
		this->x += static_cast<T>(scalar);
		this->y += static_cast<T>(scalar);
		this->z += static_cast<T>(scalar);
		return *this;
	}

	template <typename T, precision P>
	template <typename U>
	GLM_FUNC_QUALIFIER tvec3<T, P> & tvec3<T, P>::operator+=(tvec1<U, P> const & v)
	{
		this->x += static_cast<T>(v.x);
		this->y += static_cast<T>(v.x);
		this->z += static_cast<T>(v.x);
		return *this;
	}

	template <typename T, precision P>
	template <typename U>
	GLM_FUNC_QUALIFIER tvec3<T, P> & tvec3<T, P>::operator+=(tvec3<U, P> const & v)
	{
		this->x += static_cast<T>(v.x);
		this->y += static_cast<T>(v.y);
		this->z += static_cast<T>(v.z);
		return *this;
	}

	template <typename T, precision P>
	template <typename U>
	GLM_FUNC_QUALIFIER tvec3<T, P> & tvec3<T, P>::operator-=(U scalar)
	{
		this->x -= static_cast<T>(scalar);
		this->y -= static_cast<T>(scalar);
		this->z -= static_cast<T>(scalar);
		return *this;
	}

	template <typename T, precision P>
	template <typename U>
	GLM_FUNC_QUALIFIER tvec3<T, P> & tvec3<T, P>::operator-=(tvec1<U, P> const & v)
	{
		this->x -= static_cast<T>(v.x);
		this->y -= static_cast<T>(v.x);
		this->z -= static_cast<T>(v.x);
		return *this;
	}

	template <typename T, precision P>
	template <typename U>
	GLM_FUNC_QUALIFIER tvec3<T, P> & tvec3<T, P>::operator-=(tvec3<U, P> const & v)
	{
		this->x -= static_cast<T>(v.x);
		this->y -= static_cast<T>(v.y);
		this->z -= static_cast<T>(v.z);
		return *this;
	}

	template <typename T, precision P>
	template <typename U>
	GLM_FUNC_QUALIFIER tvec3<T, P> & tvec3<T, P>::operator*=(U scalar)
	{
		this->x *= static_cast<T>(scalar);
		this->y *= static_cast<T>(scalar);
		this->z *= static_cast<T>(scalar);
		return *this;
	}

	template <typename T, precision P>
	template <typename U>
	GLM_FUNC_QUALIFIER tvec3<T, P> & tvec3<T, P>::operator*=(tvec1<U, P> const & v)
	{
		this->x *= static_cast<T>(v.x);
		this->y *= static_cast<T>(v.x);
		this->z *= static_cast<T>(v.x);
		return *this;
	}

	template <typename T, precision P>
	template <typename U>
	GLM_FUNC_QUALIFIER tvec3<T, P> & tvec3<T, P>::operator*=(tvec3<U, P> const & v)
	{
		this->x *= static_cast<T>(v.x);
		this->y *= static_cast<T>(v.y);
		this->z *= static_cast<T>(v.z);
		return *this;
	}

	template <typename T, precision P>
	template <typename U>
	GLM_FUNC_QUALIFIER tvec3<T, P> & tvec3<T, P>::operator/=(U v)
	{
		this->x /= static_cast<T>(v);
		this->y /= static_cast<T>(v);
		this->z /= static_cast<T>(v);
		return *this;
	}

	template <typename T, precision P>
	template <typename U>
	GLM_FUNC_QUALIFIER tvec3<T, P> & tvec3<T, P>::operator/=(tvec1<U, P> const & v)
	{
		this->x /= static_cast<T>(v.x);
		this->y /= static_cast<T>(v.x);
		this->z /= static_cast<T>(v.x);
		return *this;
	}

	template <typename T, precision P>
	template <typename U>
	GLM_FUNC_QUALIFIER tvec3<T, P> & tvec3<T, P>::operator/=(tvec3<U, P> const & v)
	{
		this->x /= static_cast<T>(v.x);
		this->y /= static_cast<T>(v.y);
		this->z /= static_cast<T>(v.z);
		return *this;
	}

	// -- Increment and decrement operators --

	template <typename T, precision P>
	GLM_FUNC_QUALIFIER tvec3<T, P> & tvec3<T, P>::operator++()
	{
		++this->x;
		++this->y;
		++this->z;
		return *this;
	}

	template <typename T, precision P>
	GLM_FUNC_QUALIFIER tvec3<T, P> & tvec3<T, P>::operator--()
	{
		--this->x;
		--this->y;
		--this->z;
		return *this;
	}

	template <typename T, precision P> 
	GLM_FUNC_QUALIFIER tvec3<T, P> tvec3<T, P>::operator++(int)
	{
		tvec3<T, P> Result(*this);
		++*this;
		return Result;
	}

	template <typename T, precision P> 
	GLM_FUNC_QUALIFIER tvec3<T, P> tvec3<T, P>::operator--(int)
	{
		tvec3<T, P> Result(*this);
		--*this;
		return Result;
	}

	// -- Unary bit operators --

	template <typename T, precision P>
	template <typename U>
	GLM_FUNC_QUALIFIER tvec3<T, P> & tvec3<T, P>::operator%=(U scalar)
	{
		this->x %= scalar;
		this->y %= scalar;
		this->z %= scalar;
		return *this;
	}

	template <typename T, precision P>
	template <typename U>
	GLM_FUNC_QUALIFIER tvec3<T, P> & tvec3<T, P>::operator%=(tvec1<U, P> const & v)
	{
		this->x %= v.x;
		this->y %= v.x;
		this->z %= v.x;
		return *this;
	}

	template <typename T, precision P>
	template <typename U>
	GLM_FUNC_QUALIFIER tvec3<T, P> & tvec3<T, P>::operator%=(tvec3<U, P> const & v)
	{
		this->x %= v.x;
		this->y %= v.y;
		this->z %= v.z;
		return *this;
	}

	template <typename T, precision P>
	template <typename U>
	GLM_FUNC_QUALIFIER tvec3<T, P> & tvec3<T, P>::operator&=(U scalar)
	{
		this->x &= scalar;
		this->y &= scalar;
		this->z &= scalar;
		return *this;
	}

	template <typename T, precision P>
	template <typename U>
	GLM_FUNC_QUALIFIER tvec3<T, P> & tvec3<T, P>::operator&=(tvec1<U, P> const & v)
	{
		this->x &= v.x;
		this->y &= v.x;
		this->z &= v.x;
		return *this;
	}

	template <typename T, precision P>
	template <typename U>
	GLM_FUNC_QUALIFIER tvec3<T, P> & tvec3<T, P>::operator&=(tvec3<U, P> const & v)
	{
		this->x &= v.x;
		this->y &= v.y;
		this->z &= v.z;
		return *this;
	}

	template <typename T, precision P>
	template <typename U>
	GLM_FUNC_QUALIFIER tvec3<T, P> & tvec3<T, P>::operator|=(U scalar)
	{
		this->x |= scalar;
		this->y |= scalar;
		this->z |= scalar;
		return *this;
	}

	template <typename T, precision P>
	template <typename U> 
	GLM_FUNC_QUALIFIER tvec3<T, P> & tvec3<T, P>::operator|=(tvec1<U, P> const & v)
	{
		this->x |= v.x;
		this->y |= v.x;
		this->z |= v.x;
		return *this;
	}

	template <typename T, precision P>
	template <typename U> 
	GLM_FUNC_QUALIFIER tvec3<T, P> & tvec3<T, P>::operator|=(tvec3<U, P> const & v)
	{
		this->x |= v.x;
		this->y |= v.y;
		this->z |= v.z;
		return *this;
	}

	template <typename T, precision P>
	template <typename U> 
	GLM_FUNC_QUALIFIER tvec3<T, P> & tvec3<T, P>::operator^=(U scalar)
	{
		this->x ^= scalar;
		this->y ^= scalar;
		this->z ^= scalar;
		return *this;
	}

	template <typename T, precision P>
	template <typename U> 
	GLM_FUNC_QUALIFIER tvec3<T, P> & tvec3<T, P>::operator^=(tvec1<U, P> const & v)
	{
		this->x ^= v.x;
		this->y ^= v.x;
		this->z ^= v.x;
		return *this;
	}

	template <typename T, precision P>
	template <typename U> 
	GLM_FUNC_QUALIFIER tvec3<T, P> & tvec3<T, P>::operator^=(tvec3<U, P> const & v)
	{
		this->x ^= v.x;
		this->y ^= v.y;
		this->z ^= v.z;
		return *this;
	}

	template <typename T, precision P>
	template <typename U> 
	GLM_FUNC_QUALIFIER tvec3<T, P> & tvec3<T, P>::operator<<=(U scalar)
	{
		this->x <<= scalar;
		this->y <<= scalar;
		this->z <<= scalar;
		return *this;
	}

	template <typename T, precision P>
	template <typename U> 
	GLM_FUNC_QUALIFIER tvec3<T, P> & tvec3<T, P>::operator<<=(tvec1<U, P> const & v)
	{
		this->x <<= static_cast<T>(v.x);
		this->y <<= static_cast<T>(v.x);
		this->z <<= static_cast<T>(v.x);
		return *this;
	}

	template <typename T, precision P>
	template <typename U> 
	GLM_FUNC_QUALIFIER tvec3<T, P> & tvec3<T, P>::operator<<=(tvec3<U, P> const & v)
	{
		this->x <<= static_cast<T>(v.x);
		this->y <<= static_cast<T>(v.y);
		this->z <<= static_cast<T>(v.z);
		return *this;
	}

	template <typename T, precision P>
	template <typename U> 
	GLM_FUNC_QUALIFIER tvec3<T, P> & tvec3<T, P>::operator>>=(U scalar)
	{
		this->x >>= static_cast<T>(scalar);
		this->y >>= static_cast<T>(scalar);
		this->z >>= static_cast<T>(scalar);
		return *this;
	}

	template <typename T, precision P>
	template <typename U> 
	GLM_FUNC_QUALIFIER tvec3<T, P> & tvec3<T, P>::operator>>=(tvec1<U, P> const & v)
	{
		this->x >>= static_cast<T>(v.x);
		this->y >>= static_cast<T>(v.x);
		this->z >>= static_cast<T>(v.x);
		return *this;
	}

	template <typename T, precision P>
	template <typename U> 
	GLM_FUNC_QUALIFIER tvec3<T, P> & tvec3<T, P>::operator>>=(tvec3<U, P> const & v)
	{
		this->x >>= static_cast<T>(v.x);
		this->y >>= static_cast<T>(v.y);
		this->z >>= static_cast<T>(v.z);
		return *this;
	}

	// -- Unary arithmetic operators --

	template <typename T, precision P>
	GLM_FUNC_QUALIFIER tvec3<T, P> operator+(tvec3<T, P> const & v)
	{
		return v;
	}

	template <typename T, precision P>
	GLM_FUNC_QUALIFIER tvec3<T, P> operator-(tvec3<T, P> const & v)
	{
		return tvec3<T, P>(
			-v.x, 
			-v.y, 
			-v.z);
	}

	// -- Binary arithmetic operators --

	template <typename T, precision P>
	GLM_FUNC_QUALIFIER tvec3<T, P> operator+(tvec3<T, P> const & v, T const & scalar)
	{
		return tvec3<T, P>(
			v.x + scalar,
			v.y + scalar,
			v.z + scalar);
	}

	template <typename T, precision P>
	GLM_FUNC_QUALIFIER tvec3<T, P> operator+(tvec3<T, P> const & v, tvec1<T, P> const & scalar)
	{
		return tvec3<T, P>(
			v.x + scalar.x,
			v.y + scalar.x,
			v.z + scalar.x);
	}

	template <typename T, precision P>
	GLM_FUNC_QUALIFIER tvec3<T, P> operator+(T const & scalar, tvec3<T, P> const & v)
	{
		return tvec3<T, P>(
			scalar + v.x,
			scalar + v.y,
			scalar + v.z);
	}

	template <typename T, precision P>
	GLM_FUNC_QUALIFIER tvec3<T, P> operator+(tvec1<T, P> const & scalar, tvec3<T, P> const & v)
	{
		return tvec3<T, P>(
			scalar.x + v.x,
			scalar.x + v.y,
			scalar.x + v.z);
	}

	template <typename T, precision P>
	GLM_FUNC_QUALIFIER tvec3<T, P> operator+(tvec3<T, P> const & v1, tvec3<T, P> const & v2)
	{
		return tvec3<T, P>(
			v1.x + v2.x,
			v1.y + v2.y,
			v1.z + v2.z);
	}

	template <typename T, precision P>
	GLM_FUNC_QUALIFIER tvec3<T, P> operator-(tvec3<T, P> const & v, T const & scalar)
	{
		return tvec3<T, P>(
			v.x - scalar,
			v.y - scalar,
			v.z - scalar);
	}

	template <typename T, precision P>
	GLM_FUNC_QUALIFIER tvec3<T, P> operator-(tvec3<T, P> const & v, tvec1<T, P> const & scalar)
	{
		return tvec3<T, P>(
			v.x - scalar.x,
			v.y - scalar.x,
			v.z - scalar.x);
	}

	template <typename T, precision P> 
	GLM_FUNC_QUALIFIER tvec3<T, P> operator-(T const & scalar, tvec3<T, P> const & v)
	{
		return tvec3<T, P>(
			scalar - v.x,
			scalar - v.y,
			scalar - v.z);
	}

	template <typename T, precision P> 
	GLM_FUNC_QUALIFIER tvec3<T, P> operator-(tvec1<T, P> const & scalar, tvec3<T, P> const & v)
	{
		return tvec3<T, P>(
			scalar.x - v.x,
			scalar.x - v.y,
			scalar.x - v.z);
	}

	template <typename T, precision P>
	GLM_FUNC_QUALIFIER tvec3<T, P> operator-(tvec3<T, P> const & v1, tvec3<T, P> const & v2)
	{
		return tvec3<T, P>(
			v1.x - v2.x,
			v1.y - v2.y,
			v1.z - v2.z);
	}

	template <typename T, precision P>
	GLM_FUNC_QUALIFIER tvec3<T, P> operator*(tvec3<T, P> const & v, T const & scalar)
	{
		return tvec3<T, P>(
			v.x * scalar,
			v.y * scalar,
			v.z * scalar);
	}

	template <typename T, precision P>
	GLM_FUNC_QUALIFIER tvec3<T, P> operator*(tvec3<T, P> const & v, tvec1<T, P> const & scalar)
	{
		return tvec3<T, P>(
			v.x * scalar.x,
			v.y * scalar.x,
			v.z * scalar.x);
	}

	template <typename T, precision P>
	GLM_FUNC_QUALIFIER tvec3<T, P> operator*(T const & scalar, tvec3<T, P> const & v)
	{
		return tvec3<T, P>(
			scalar * v.x,
			scalar * v.y,
			scalar * v.z);
	}

	template <typename T, precision P>
	GLM_FUNC_QUALIFIER tvec3<T, P> operator*(tvec1<T, P> const & scalar, tvec3<T, P> const & v)
	{
		return tvec3<T, P>(
			scalar.x * v.x,
			scalar.x * v.y,
			scalar.x * v.z);
	}

	template <typename T, precision P>
	GLM_FUNC_QUALIFIER tvec3<T, P> operator*(tvec3<T, P> const & v1, tvec3<T, P> const & v2)
	{
		return tvec3<T, P>(
			v1.x * v2.x,
			v1.y * v2.y,
			v1.z * v2.z);
	}

	template <typename T, precision P>
	GLM_FUNC_QUALIFIER tvec3<T, P> operator/(tvec3<T, P> const & v, T const & scalar)
	{
		return tvec3<T, P>(
			v.x / scalar,
			v.y / scalar,
			v.z / scalar);
	}

	template <typename T, precision P>
	GLM_FUNC_QUALIFIER tvec3<T, P> operator/(tvec3<T, P> const & v, tvec1<T, P> const & scalar)
	{
		return tvec3<T, P>(
			v.x / scalar.x,
			v.y / scalar.x,
			v.z / scalar.x);
	}

	template <typename T, precision P>
	GLM_FUNC_QUALIFIER tvec3<T, P> operator/(T const & scalar, tvec3<T, P> const & v)
	{
		return tvec3<T, P>(
			scalar / v.x,
			scalar / v.y,
			scalar / v.z);
	}

	template <typename T, precision P>
	GLM_FUNC_QUALIFIER tvec3<T, P> operator/(tvec1<T, P> const & scalar, tvec3<T, P> const & v)
	{
		return tvec3<T, P>(
			scalar.x / v.x,
			scalar.x / v.y,
			scalar.x / v.z);
	}

	template <typename T, precision P>
	GLM_FUNC_QUALIFIER tvec3<T, P> operator/(tvec3<T, P> const & v1, tvec3<T, P> const & v2)
	{
		return tvec3<T, P>(
			v1.x / v2.x,
			v1.y / v2.y,
			v1.z / v2.z);
	}

	// -- Binary bit operators --

	template <typename T, precision P>
	GLM_FUNC_QUALIFIER tvec3<T, P> operator%(tvec3<T, P> const & v, T const & scalar)
	{
		return tvec3<T, P>(
			v.x % scalar,
			v.y % scalar,
			v.z % scalar);
	}

	template <typename T, precision P>
	GLM_FUNC_QUALIFIER tvec3<T, P> operator%(tvec3<T, P> const & v, tvec1<T, P> const & scalar)
	{
		return tvec3<T, P>(
			v.x % scalar.x,
			v.y % scalar.x,
			v.z % scalar.x);
	}

	template <typename T, precision P>
	GLM_FUNC_QUALIFIER tvec3<T, P> operator%(T const & scalar, tvec3<T, P> const & v)
	{
		return tvec3<T, P>(
			scalar % v.x,
			scalar % v.y,
			scalar % v.z);
	}

	template <typename T, precision P>
	GLM_FUNC_QUALIFIER tvec3<T, P> operator%(tvec1<T, P> const & scalar, tvec3<T, P> const & v)
	{
		return tvec3<T, P>(
			scalar.x % v.x,
			scalar.x % v.y,
			scalar.x % v.z);
	}

	template <typename T, precision P>
	GLM_FUNC_QUALIFIER tvec3<T, P> operator%(tvec3<T, P> const & v1, tvec3<T, P> const & v2)
	{
		return tvec3<T, P>(
			v1.x % v2.x,
			v1.y % v2.y,
			v1.z % v2.z);
	}

	template <typename T, precision P>
	GLM_FUNC_QUALIFIER tvec3<T, P> operator&(tvec3<T, P> const & v, T const & scalar)
	{
		return tvec3<T, P>(
			v.x & scalar,
			v.y & scalar,
			v.z & scalar);
	}

	template <typename T, precision P>
	GLM_FUNC_QUALIFIER tvec3<T, P> operator&(tvec3<T, P> const & v, tvec1<T, P> const & scalar)
	{
		return tvec3<T, P>(
			v.x & scalar.x,
			v.y & scalar.x,
			v.z & scalar.x);
	}

	template <typename T, precision P>
	GLM_FUNC_QUALIFIER tvec3<T, P> operator&(T const & scalar, tvec3<T, P> const & v)
	{
		return tvec3<T, P>(
			scalar & v.x,
			scalar & v.y,
			scalar & v.z);
	}

	template <typename T, precision P>
	GLM_FUNC_QUALIFIER tvec3<T, P> operator&(tvec1<T, P> const & scalar, tvec3<T, P> const & v)
	{
		return tvec3<T, P>(
			scalar.x & v.x,
			scalar.x & v.y,
			scalar.x & v.z);
	}

	template <typename T, precision P>
	GLM_FUNC_QUALIFIER tvec3<T, P> operator&(tvec3<T, P> const & v1, tvec3<T, P> const & v2)
	{
		return tvec3<T, P>(
			v1.x & v2.x,
			v1.y & v2.y,
			v1.z & v2.z);
	}

	template <typename T, precision P>
	GLM_FUNC_QUALIFIER tvec3<T, P> operator|(tvec3<T, P> const & v, T const & scalar)
	{
		return tvec3<T, P>(
			v.x | scalar,
			v.y | scalar,
			v.z | scalar);
	}

	template <typename T, precision P>
	GLM_FUNC_QUALIFIER tvec3<T, P> operator|(tvec3<T, P> const & v, tvec1<T, P> const & scalar)
	{
		return tvec3<T, P>(
			v.x | scalar.x,
			v.y | scalar.x,
			v.z | scalar.x);
	}

	template <typename T, precision P>
	GLM_FUNC_QUALIFIER tvec3<T, P> operator|(T const & scalar, tvec3<T, P> const & v)
	{
		return tvec3<T, P>(
			scalar | v.x,
			scalar | v.y,
			scalar | v.z);
	}

	template <typename T, precision P>
	GLM_FUNC_QUALIFIER tvec3<T, P> operator|(tvec1<T, P> const & scalar, tvec3<T, P> const & v)
	{
		return tvec3<T, P>(
			scalar.x | v.x,
			scalar.x | v.y,
			scalar.x | v.z);
	}

	template <typename T, precision P>
	GLM_FUNC_QUALIFIER tvec3<T, P> operator|(tvec3<T, P> const & v1, tvec3<T, P> const & v2)
	{
		return tvec3<T, P>(
			v1.x | v2.x,
			v1.y | v2.y,
			v1.z | v2.z);
	}

	template <typename T, precision P>
	GLM_FUNC_QUALIFIER tvec3<T, P> operator^(tvec3<T, P> const & v, T const & scalar)
	{
		return tvec3<T, P>(
			v.x ^ scalar,
			v.y ^ scalar,
			v.z ^ scalar);
	}

	template <typename T, precision P>
	GLM_FUNC_QUALIFIER tvec3<T, P> operator^(tvec3<T, P> const & v, tvec1<T, P> const & scalar)
	{
		return tvec3<T, P>(
			v.x ^ scalar.x,
			v.y ^ scalar.x,
			v.z ^ scalar.x);
	}

	template <typename T, precision P>
	GLM_FUNC_QUALIFIER tvec3<T, P> operator^(T const & scalar, tvec3<T, P> const & v)
	{
		return tvec3<T, P>(
			scalar ^ v.x,
			scalar ^ v.y,
			scalar ^ v.z);
	}

	template <typename T, precision P>
	GLM_FUNC_QUALIFIER tvec3<T, P> operator^(tvec1<T, P> const & scalar, tvec3<T, P> const & v)
	{
		return tvec3<T, P>(
			scalar.x ^ v.x,
			scalar.x ^ v.y,
			scalar.x ^ v.z);
	}

	template <typename T, precision P>
	GLM_FUNC_QUALIFIER tvec3<T, P> operator^(tvec3<T, P> const & v1, tvec3<T, P> const & v2)
	{
		return tvec3<T, P>(
			v1.x ^ v2.x,
			v1.y ^ v2.y,
			v1.z ^ v2.z);
	}

	template <typename T, precision P>
	GLM_FUNC_QUALIFIER tvec3<T, P> operator<<(tvec3<T, P> const & v, T const & scalar)
	{
		return tvec3<T, P>(
			v.x << scalar,
			v.y << scalar,
			v.z << scalar);
	}

	template <typename T, precision P>
	GLM_FUNC_QUALIFIER tvec3<T, P> operator<<(tvec3<T, P> const & v, tvec1<T, P> const & scalar)
	{
		return tvec3<T, P>(
			v.x << scalar.x,
			v.y << scalar.x,
			v.z << scalar.x);
	}

	template <typename T, precision P>
	GLM_FUNC_QUALIFIER tvec3<T, P> operator<<(T const & scalar, tvec3<T, P> const & v)
	{
		return tvec3<T, P>(
			scalar << v.x,
			scalar << v.y,
			scalar << v.z);
	}

	template <typename T, precision P>
	GLM_FUNC_QUALIFIER tvec3<T, P> operator<<(tvec1<T, P> const & scalar, tvec3<T, P> const & v)
	{
		return tvec3<T, P>(
			scalar.x << v.x,
			scalar.x << v.y,
			scalar.x << v.z);
	}

	template <typename T, precision P>
	GLM_FUNC_QUALIFIER tvec3<T, P> operator<<(tvec3<T, P> const & v1, tvec3<T, P> const & v2)
	{
		return tvec3<T, P>(
			v1.x << v2.x,
			v1.y << v2.y,
			v1.z << v2.z);
	}

	template <typename T, precision P>
	GLM_FUNC_QUALIFIER tvec3<T, P> operator>>(tvec3<T, P> const & v, T const & scalar)
	{
		return tvec3<T, P>(
			v.x >> scalar,
			v.y >> scalar,
			v.z >> scalar);
	}

	template <typename T, precision P>
	GLM_FUNC_QUALIFIER tvec3<T, P> operator>>(tvec3<T, P> const & v, tvec1<T, P> const & scalar)
	{
		return tvec3<T, P>(
			v.x >> scalar.x,
			v.y >> scalar.x,
			v.z >> scalar.x);
	}

	template <typename T, precision P>
	GLM_FUNC_QUALIFIER tvec3<T, P> operator>>(T const & scalar, tvec3<T, P> const & v)
	{
		return tvec3<T, P>(
			scalar >> v.x,
			scalar >> v.y,
			scalar >> v.z);
	}

	template <typename T, precision P>
	GLM_FUNC_QUALIFIER tvec3<T, P> operator>>(tvec1<T, P> const & scalar, tvec3<T, P> const & v)
	{
		return tvec3<T, P>(
			scalar.x >> v.x,
			scalar.x >> v.y,
			scalar.x >> v.z);
	}

	template <typename T, precision P>
	GLM_FUNC_QUALIFIER tvec3<T, P> operator>>(tvec3<T, P> const & v1, tvec3<T, P> const & v2)
	{
		return tvec3<T, P>(
			v1.x >> v2.x,
			v1.y >> v2.y,
			v1.z >> v2.z);
	}

	template <typename T, precision P> 
	GLM_FUNC_QUALIFIER tvec3<T, P> operator~(tvec3<T, P> const & v)
	{
		return tvec3<T, P>(
			~v.x,
			~v.y,
			~v.z);
	}

	// -- Boolean operators --

	template <typename T, precision P>
	GLM_FUNC_QUALIFIER bool operator==(tvec3<T, P> const & v1, tvec3<T, P> const & v2)
	{
		return (v1.x == v2.x) && (v1.y == v2.y) && (v1.z == v2.z);
	}

	template <typename T, precision P>
	GLM_FUNC_QUALIFIER bool operator!=(tvec3<T, P> const & v1, tvec3<T, P> const & v2)
	{
		return (v1.x != v2.x) || (v1.y != v2.y) || (v1.z != v2.z);
	}

	template <precision P>
	GLM_FUNC_QUALIFIER tvec3<bool, P> operator&&(tvec3<bool, P> const & v1, tvec3<bool, P> const & v2)
	{
		return tvec3<bool, P>(v1.x && v2.x, v1.y && v2.y, v1.z && v2.z);
	}

	template <precision P>
	GLM_FUNC_QUALIFIER tvec3<bool, P> operator||(tvec3<bool, P> const & v1, tvec3<bool, P> const & v2)
	{
		return tvec3<bool, P>(v1.x || v2.x, v1.y || v2.y, v1.z || v2.z);
	}
}//namespace glm
