/// @ref core
/// @file glm/core/type_tvec2.inl

namespace glm
{
	// -- Implicit basic constructors --

#	if !GLM_HAS_DEFAULTED_FUNCTIONS || !defined(GLM_FORCE_NO_CTOR_INIT)
		template<typename T, precision P>
		GLM_FUNC_QUALIFIER GLM_CONSTEXPR_CTOR vec<2, T, P>::vec()
#			ifndef GLM_FORCE_NO_CTOR_INIT
				: x(0), y(0)
#			endif
		{}
#	endif//!GLM_HAS_DEFAULTED_FUNCTIONS

#	if !GLM_HAS_DEFAULTED_FUNCTIONS
		template<typename T, precision P>
		GLM_FUNC_QUALIFIER GLM_CONSTEXPR_CTOR vec<2, T, P>::vec(vec<2, T, P> const& v)
			: x(v.x), y(v.y)
		{}
#	endif//!GLM_HAS_DEFAULTED_FUNCTIONS

	template<typename T, precision P>
	template<precision Q>
	GLM_FUNC_QUALIFIER GLM_CONSTEXPR_CTOR vec<2, T, P>::vec(vec<2, T, Q> const& v)
		: x(v.x), y(v.y)
	{}

	// -- Explicit basic constructors --

	template<typename T, precision P>
	GLM_FUNC_QUALIFIER GLM_CONSTEXPR_CTOR vec<2, T, P>::vec(ctor)
	{}

	template<typename T, precision P>
	GLM_FUNC_QUALIFIER GLM_CONSTEXPR_CTOR vec<2, T, P>::vec(T scalar)
		: x(scalar), y(scalar)
	{}

	template<typename T, precision P>
	GLM_FUNC_QUALIFIER GLM_CONSTEXPR_CTOR vec<2, T, P>::vec(T _x, T _y)
		: x(_x), y(_y)
	{}

	// -- Conversion scalar constructors --

	template<typename T, precision P>
	template<typename A, typename B>
	GLM_FUNC_QUALIFIER GLM_CONSTEXPR_CTOR vec<2, T, P>::vec(A _x, B _y)
		: x(static_cast<T>(_x))
		, y(static_cast<T>(_y))
	{}

	template<typename T, precision P>
	template<typename A, typename B>
	GLM_FUNC_QUALIFIER GLM_CONSTEXPR_CTOR vec<2, T, P>::vec(vec<1, A, P> const& _x, vec<1, B, P> const& _y)
		: x(static_cast<T>(_x.x))
		, y(static_cast<T>(_y.x))
	{}

	// -- Conversion vector constructors --

	template<typename T, precision P>
	template<typename U, precision Q>
	GLM_FUNC_QUALIFIER GLM_CONSTEXPR_CTOR vec<2, T, P>::vec(vec<2, U, Q> const& v)
		: x(static_cast<T>(v.x))
		, y(static_cast<T>(v.y))
	{}

	template<typename T, precision P>
	template<typename U, precision Q>
	GLM_FUNC_QUALIFIER GLM_CONSTEXPR_CTOR vec<2, T, P>::vec(vec<3, U, Q> const& v)
		: x(static_cast<T>(v.x))
		, y(static_cast<T>(v.y))
	{}

	template<typename T, precision P>
	template<typename U, precision Q>
	GLM_FUNC_QUALIFIER GLM_CONSTEXPR_CTOR vec<2, T, P>::vec(vec<4, U, Q> const& v)
		: x(static_cast<T>(v.x))
		, y(static_cast<T>(v.y))
	{}

	// -- Component accesses --

	template<typename T, precision P>
	GLM_FUNC_QUALIFIER T & vec<2, T, P>::operator[](typename vec<2, T, P>::length_type i)
	{
		assert(i >= 0 && i < this->length());
		return (&x)[i];
	}

	template<typename T, precision P>
	GLM_FUNC_QUALIFIER T const & vec<2, T, P>::operator[](typename vec<2, T, P>::length_type i) const
	{
		assert(i >= 0 && i < this->length());
		return (&x)[i];
	}

	// -- Unary arithmetic operators --

#	if !GLM_HAS_DEFAULTED_FUNCTIONS
		template<typename T, precision P>
		GLM_FUNC_QUALIFIER vec<2, T, P> & vec<2, T, P>::operator=(vec<2, T, P> const& v)
		{
			this->x = v.x;
			this->y = v.y;
			return *this;
		}
#	endif//!GLM_HAS_DEFAULTED_FUNCTIONS

	template<typename T, precision P>
	template<typename U>
	GLM_FUNC_QUALIFIER vec<2, T, P> & vec<2, T, P>::operator=(vec<2, U, P> const& v)
	{
		this->x = static_cast<T>(v.x);
		this->y = static_cast<T>(v.y);
		return *this;
	}

	template<typename T, precision P>
	template<typename U>
	GLM_FUNC_QUALIFIER vec<2, T, P> & vec<2, T, P>::operator+=(U scalar)
	{
		this->x += static_cast<T>(scalar);
		this->y += static_cast<T>(scalar);
		return *this;
	}

	template<typename T, precision P>
	template<typename U>
	GLM_FUNC_QUALIFIER vec<2, T, P> & vec<2, T, P>::operator+=(vec<1, U, P> const& v)
	{
		this->x += static_cast<T>(v.x);
		this->y += static_cast<T>(v.x);
		return *this;
	}

	template<typename T, precision P>
	template<typename U>
	GLM_FUNC_QUALIFIER vec<2, T, P> & vec<2, T, P>::operator+=(vec<2, U, P> const& v)
	{
		this->x += static_cast<T>(v.x);
		this->y += static_cast<T>(v.y);
		return *this;
	}

	template<typename T, precision P>
	template<typename U>
	GLM_FUNC_QUALIFIER vec<2, T, P> & vec<2, T, P>::operator-=(U scalar)
	{
		this->x -= static_cast<T>(scalar);
		this->y -= static_cast<T>(scalar);
		return *this;
	}

	template<typename T, precision P>
	template<typename U>
	GLM_FUNC_QUALIFIER vec<2, T, P> & vec<2, T, P>::operator-=(vec<1, U, P> const& v)
	{
		this->x -= static_cast<T>(v.x);
		this->y -= static_cast<T>(v.x);
		return *this;
	}

	template<typename T, precision P>
	template<typename U>
	GLM_FUNC_QUALIFIER vec<2, T, P> & vec<2, T, P>::operator-=(vec<2, U, P> const& v)
	{
		this->x -= static_cast<T>(v.x);
		this->y -= static_cast<T>(v.y);
		return *this;
	}

	template<typename T, precision P>
	template<typename U>
	GLM_FUNC_QUALIFIER vec<2, T, P> & vec<2, T, P>::operator*=(U scalar)
	{
		this->x *= static_cast<T>(scalar);
		this->y *= static_cast<T>(scalar);
		return *this;
	}

	template<typename T, precision P>
	template<typename U>
	GLM_FUNC_QUALIFIER vec<2, T, P> & vec<2, T, P>::operator*=(vec<1, U, P> const& v)
	{
		this->x *= static_cast<T>(v.x);
		this->y *= static_cast<T>(v.x);
		return *this;
	}

	template<typename T, precision P>
	template<typename U>
	GLM_FUNC_QUALIFIER vec<2, T, P> & vec<2, T, P>::operator*=(vec<2, U, P> const& v)
	{
		this->x *= static_cast<T>(v.x);
		this->y *= static_cast<T>(v.y);
		return *this;
	}

	template<typename T, precision P>
	template<typename U>
	GLM_FUNC_QUALIFIER vec<2, T, P> & vec<2, T, P>::operator/=(U scalar)
	{
		this->x /= static_cast<T>(scalar);
		this->y /= static_cast<T>(scalar);
		return *this;
	}

	template<typename T, precision P>
	template<typename U>
	GLM_FUNC_QUALIFIER vec<2, T, P> & vec<2, T, P>::operator/=(vec<1, U, P> const& v)
	{
		this->x /= static_cast<T>(v.x);
		this->y /= static_cast<T>(v.x);
		return *this;
	}

	template<typename T, precision P>
	template<typename U>
	GLM_FUNC_QUALIFIER vec<2, T, P> & vec<2, T, P>::operator/=(vec<2, U, P> const& v)
	{
		this->x /= static_cast<T>(v.x);
		this->y /= static_cast<T>(v.y);
		return *this;
	}

	// -- Increment and decrement operators --

	template<typename T, precision P>
	GLM_FUNC_QUALIFIER vec<2, T, P> & vec<2, T, P>::operator++()
	{
		++this->x;
		++this->y;
		return *this;
	}

	template<typename T, precision P>
	GLM_FUNC_QUALIFIER vec<2, T, P> & vec<2, T, P>::operator--()
	{
		--this->x;
		--this->y;
		return *this;
	}

	template<typename T, precision P> 
	GLM_FUNC_QUALIFIER vec<2, T, P> vec<2, T, P>::operator++(int)
	{
		vec<2, T, P> Result(*this);
		++*this;
		return Result;
	}

	template<typename T, precision P> 
	GLM_FUNC_QUALIFIER vec<2, T, P> vec<2, T, P>::operator--(int)
	{
		vec<2, T, P> Result(*this);
		--*this;
		return Result;
	}

	// -- Unary bit operators --

	template<typename T, precision P>
	template<typename U>
	GLM_FUNC_QUALIFIER vec<2, T, P> & vec<2, T, P>::operator%=(U scalar)
	{
		this->x %= static_cast<T>(scalar);
		this->y %= static_cast<T>(scalar);
		return *this;
	}

	template<typename T, precision P>
	template<typename U>
	GLM_FUNC_QUALIFIER vec<2, T, P> & vec<2, T, P>::operator%=(vec<1, U, P> const& v)
	{
		this->x %= static_cast<T>(v.x);
		this->y %= static_cast<T>(v.x);
		return *this;
	}

	template<typename T, precision P>
	template<typename U>
	GLM_FUNC_QUALIFIER vec<2, T, P> & vec<2, T, P>::operator%=(vec<2, U, P> const& v)
	{
		this->x %= static_cast<T>(v.x);
		this->y %= static_cast<T>(v.y);
		return *this;
	}

	template<typename T, precision P>
	template<typename U>
	GLM_FUNC_QUALIFIER vec<2, T, P> & vec<2, T, P>::operator&=(U scalar)
	{
		this->x &= static_cast<T>(scalar);
		this->y &= static_cast<T>(scalar);
		return *this;
	}

	template<typename T, precision P>
	template<typename U>
	GLM_FUNC_QUALIFIER vec<2, T, P> & vec<2, T, P>::operator&=(vec<1, U, P> const& v)
	{
		this->x &= static_cast<T>(v.x);
		this->y &= static_cast<T>(v.x);
		return *this;
	}

	template<typename T, precision P>
	template<typename U>
	GLM_FUNC_QUALIFIER vec<2, T, P> & vec<2, T, P>::operator&=(vec<2, U, P> const& v)
	{
		this->x &= static_cast<T>(v.x);
		this->y &= static_cast<T>(v.y);
		return *this;
	}

	template<typename T, precision P>
	template<typename U>
	GLM_FUNC_QUALIFIER vec<2, T, P> & vec<2, T, P>::operator|=(U scalar)
	{
		this->x |= static_cast<T>(scalar);
		this->y |= static_cast<T>(scalar);
		return *this;
	}

	template<typename T, precision P>
	template<typename U>
	GLM_FUNC_QUALIFIER vec<2, T, P> & vec<2, T, P>::operator|=(vec<1, U, P> const& v)
	{
		this->x |= static_cast<T>(v.x);
		this->y |= static_cast<T>(v.x);
		return *this;
	}

	template<typename T, precision P>
	template<typename U>
	GLM_FUNC_QUALIFIER vec<2, T, P> & vec<2, T, P>::operator|=(vec<2, U, P> const& v)
	{
		this->x |= static_cast<T>(v.x);
		this->y |= static_cast<T>(v.y);
		return *this;
	}

	template<typename T, precision P>
	template<typename U>
	GLM_FUNC_QUALIFIER vec<2, T, P> & vec<2, T, P>::operator^=(U scalar)
	{
		this->x ^= static_cast<T>(scalar);
		this->y ^= static_cast<T>(scalar);
		return *this;
	}

	template<typename T, precision P>
	template<typename U>
	GLM_FUNC_QUALIFIER vec<2, T, P> & vec<2, T, P>::operator^=(vec<1, U, P> const& v)
	{
		this->x ^= static_cast<T>(v.x);
		this->y ^= static_cast<T>(v.x);
		return *this;
	}

	template<typename T, precision P>
	template<typename U>
	GLM_FUNC_QUALIFIER vec<2, T, P> & vec<2, T, P>::operator^=(vec<2, U, P> const& v)
	{
		this->x ^= static_cast<T>(v.x);
		this->y ^= static_cast<T>(v.y);
		return *this;
	}

	template<typename T, precision P>
	template<typename U>
	GLM_FUNC_QUALIFIER vec<2, T, P> & vec<2, T, P>::operator<<=(U scalar)
	{
		this->x <<= static_cast<T>(scalar);
		this->y <<= static_cast<T>(scalar);
		return *this;
	}

	template<typename T, precision P>
	template<typename U>
	GLM_FUNC_QUALIFIER vec<2, T, P> & vec<2, T, P>::operator<<=(vec<1, U, P> const& v)
	{
		this->x <<= static_cast<T>(v.x);
		this->y <<= static_cast<T>(v.x);
		return *this;
	}

	template<typename T, precision P>
	template<typename U>
	GLM_FUNC_QUALIFIER vec<2, T, P> & vec<2, T, P>::operator<<=(vec<2, U, P> const& v)
	{
		this->x <<= static_cast<T>(v.x);
		this->y <<= static_cast<T>(v.y);
		return *this;
	}

	template<typename T, precision P>
	template<typename U>
	GLM_FUNC_QUALIFIER vec<2, T, P> & vec<2, T, P>::operator>>=(U scalar)
	{
		this->x >>= static_cast<T>(scalar);
		this->y >>= static_cast<T>(scalar);
		return *this;
	}

	template<typename T, precision P>
	template<typename U>
	GLM_FUNC_QUALIFIER vec<2, T, P> & vec<2, T, P>::operator>>=(vec<1, U, P> const& v)
	{
		this->x >>= static_cast<T>(v.x);
		this->y >>= static_cast<T>(v.x);
		return *this;
	}

	template<typename T, precision P>
	template<typename U>
	GLM_FUNC_QUALIFIER vec<2, T, P> & vec<2, T, P>::operator>>=(vec<2, U, P> const& v)
	{
		this->x >>= static_cast<T>(v.x);
		this->y >>= static_cast<T>(v.y);
		return *this;
	}

	// -- Unary arithmetic operators --

	template<typename T, precision P>
	GLM_FUNC_QUALIFIER vec<2, T, P> operator+(vec<2, T, P> const& v)
	{
		return v;
	}

	template<typename T, precision P>
	GLM_FUNC_QUALIFIER vec<2, T, P> operator-(vec<2, T, P> const& v)
	{
		return vec<2, T, P>(
			-v.x, 
			-v.y);
	}

	// -- Binary arithmetic operators --

	template<typename T, precision P>
	GLM_FUNC_QUALIFIER vec<2, T, P> operator+(vec<2, T, P> const& v, T scalar)
	{
		return vec<2, T, P>(
			v.x + scalar,
			v.y + scalar);
	}

	template<typename T, precision P>
	GLM_FUNC_QUALIFIER vec<2, T, P> operator+(vec<2, T, P> const& v1, vec<1, T, P> const& v2)
	{
		return vec<2, T, P>(
			v1.x + v2.x,
			v1.y + v2.x);
	}

	template<typename T, precision P>
	GLM_FUNC_QUALIFIER vec<2, T, P> operator+(T scalar, vec<2, T, P> const& v)
	{
		return vec<2, T, P>(
			scalar + v.x,
			scalar + v.y);
	}

	template<typename T, precision P>
	GLM_FUNC_QUALIFIER vec<2, T, P> operator+(vec<1, T, P> const& v1, vec<2, T, P> const& v2)
	{
		return vec<2, T, P>(
			v1.x + v2.x,
			v1.x + v2.y);
	}

	template<typename T, precision P>
	GLM_FUNC_QUALIFIER vec<2, T, P> operator+(vec<2, T, P> const & v1, vec<2, T, P> const & v2)
	{
		return vec<2, T, P>(
			v1.x + v2.x,
			v1.y + v2.y);
	}

	template<typename T, precision P>
	GLM_FUNC_QUALIFIER vec<2, T, P> operator-(vec<2, T, P> const & v, T scalar)
	{
		return vec<2, T, P>(
			v.x - scalar,
			v.y - scalar);
	}

	template<typename T, precision P>
	GLM_FUNC_QUALIFIER vec<2, T, P> operator-(vec<2, T, P> const & v1, vec<1, T, P> const & v2)
	{
		return vec<2, T, P>(
			v1.x - v2.x,
			v1.y - v2.x);
	}

	template<typename T, precision P>
	GLM_FUNC_QUALIFIER vec<2, T, P> operator-(T scalar, vec<2, T, P> const & v)
	{
		return vec<2, T, P>(
			scalar - v.x,
			scalar - v.y);
	}

	template<typename T, precision P>
	GLM_FUNC_QUALIFIER vec<2, T, P> operator-(vec<1, T, P> const & v1, vec<2, T, P> const & v2)
	{
		return vec<2, T, P>(
			v1.x - v2.x,
			v1.x - v2.y);
	}

	template<typename T, precision P>
	GLM_FUNC_QUALIFIER vec<2, T, P> operator-(vec<2, T, P> const & v1, vec<2, T, P> const & v2)
	{
		return vec<2, T, P>(
			v1.x - v2.x,
			v1.y - v2.y);
	}

	template<typename T, precision P>
	GLM_FUNC_QUALIFIER vec<2, T, P> operator*(vec<2, T, P> const & v, T scalar)
	{
		return vec<2, T, P>(
			v.x * scalar,
			v.y * scalar);
	}

	template<typename T, precision P>
	GLM_FUNC_QUALIFIER vec<2, T, P> operator*(vec<2, T, P> const & v1, vec<1, T, P> const & v2)
	{
		return vec<2, T, P>(
			v1.x * v2.x,
			v1.y * v2.x);
	}

	template<typename T, precision P>
	GLM_FUNC_QUALIFIER vec<2, T, P> operator*(T scalar, vec<2, T, P> const & v)
	{
		return vec<2, T, P>(
			scalar * v.x,
			scalar * v.y);
	}

	template<typename T, precision P>
	GLM_FUNC_QUALIFIER vec<2, T, P> operator*(vec<1, T, P> const & v1, vec<2, T, P> const & v2)
	{
		return vec<2, T, P>(
			v1.x * v2.x,
			v1.x * v2.y);
	}

	template<typename T, precision P>
	GLM_FUNC_QUALIFIER vec<2, T, P> operator*(vec<2, T, P> const & v1, vec<2, T, P> const & v2)
	{
		return vec<2, T, P>(
			v1.x * v2.x,
			v1.y * v2.y);
	}

	template<typename T, precision P>
	GLM_FUNC_QUALIFIER vec<2, T, P> operator/(vec<2, T, P> const & v, T scalar)
	{
		return vec<2, T, P>(
			v.x / scalar,
			v.y / scalar);
	}

	template<typename T, precision P>
	GLM_FUNC_QUALIFIER vec<2, T, P> operator/(vec<2, T, P> const & v1, vec<1, T, P> const & v2)
	{
		return vec<2, T, P>(
			v1.x / v2.x,
			v1.y / v2.x);
	}

	template<typename T, precision P>
	GLM_FUNC_QUALIFIER vec<2, T, P> operator/(T scalar, vec<2, T, P> const & v)
	{
		return vec<2, T, P>(
			scalar / v.x,
			scalar / v.y);
	}

	template<typename T, precision P>
	GLM_FUNC_QUALIFIER vec<2, T, P> operator/(vec<1, T, P> const & v1, vec<2, T, P> const & v2)
	{
		return vec<2, T, P>(
			v1.x / v2.x,
			v1.x / v2.y);
	}

	template<typename T, precision P>
	GLM_FUNC_QUALIFIER vec<2, T, P> operator/(vec<2, T, P> const & v1, vec<2, T, P> const & v2)
	{
		return vec<2, T, P>(
			v1.x / v2.x,
			v1.y / v2.y);
	}

	// -- Binary bit operators --

	template<typename T, precision P>
	GLM_FUNC_QUALIFIER vec<2, T, P> operator%(vec<2, T, P> const & v, T scalar)
	{
		return vec<2, T, P>(
			v.x % scalar,
			v.y % scalar);
	}

	template<typename T, precision P>
	GLM_FUNC_QUALIFIER vec<2, T, P> operator%(vec<2, T, P> const & v1, vec<1, T, P> const & v2)
	{
		return vec<2, T, P>(
			v1.x % v2.x,
			v1.y % v2.x);
	}

	template<typename T, precision P>
	GLM_FUNC_QUALIFIER vec<2, T, P> operator%(T scalar, vec<2, T, P> const & v)
	{
		return vec<2, T, P>(
			scalar % v.x,
			scalar % v.y);
	}

	template<typename T, precision P>
	GLM_FUNC_QUALIFIER vec<2, T, P> operator%(vec<1, T, P> const & v1, vec<2, T, P> const & v2)
	{
		return vec<2, T, P>(
			v1.x % v2.x,
			v1.x % v2.y);
	}

	template<typename T, precision P>
	GLM_FUNC_QUALIFIER vec<2, T, P> operator%(vec<2, T, P> const & v1, vec<2, T, P> const & v2)
	{
		return vec<2, T, P>(
			v1.x % v2.x,
			v1.y % v2.y);
	}

	template<typename T, precision P>
	GLM_FUNC_QUALIFIER vec<2, T, P> operator&(vec<2, T, P> const & v, T scalar)
	{
		return vec<2, T, P>(
			v.x & scalar,
			v.y & scalar);
	}

	template<typename T, precision P>
	GLM_FUNC_QUALIFIER vec<2, T, P> operator&(vec<2, T, P> const & v1, vec<1, T, P> const & v2)
	{
		return vec<2, T, P>(
			v1.x & v2.x,
			v1.y & v2.x);
	}

	template<typename T, precision P>
	GLM_FUNC_QUALIFIER vec<2, T, P> operator&(T scalar, vec<2, T, P> const & v)
	{
		return vec<2, T, P>(
			scalar & v.x,
			scalar & v.y);
	}

	template<typename T, precision P>
	GLM_FUNC_QUALIFIER vec<2, T, P> operator&(vec<1, T, P> const & v1, vec<2, T, P> const & v2)
	{
		return vec<2, T, P>(
			v1.x & v2.x,
			v1.x & v2.y);
	}

	template<typename T, precision P>
	GLM_FUNC_QUALIFIER vec<2, T, P> operator&(vec<2, T, P> const & v1, vec<2, T, P> const & v2)
	{
		return vec<2, T, P>(
			v1.x & v2.x,
			v1.y & v2.y);
	}

	template<typename T, precision P>
	GLM_FUNC_QUALIFIER vec<2, T, P> operator|(vec<2, T, P> const & v, T scalar)
	{
		return vec<2, T, P>(
			v.x | scalar,
			v.y | scalar);
	}

	template<typename T, precision P>
	GLM_FUNC_QUALIFIER vec<2, T, P> operator|(vec<2, T, P> const & v1, vec<1, T, P> const & v2)
	{
		return vec<2, T, P>(
			v1.x | v2.x,
			v1.y | v2.x);
	}

	template<typename T, precision P>
	GLM_FUNC_QUALIFIER vec<2, T, P> operator|(T scalar, vec<2, T, P> const & v)
	{
		return vec<2, T, P>(
			scalar | v.x,
			scalar | v.y);
	}

	template<typename T, precision P>
	GLM_FUNC_QUALIFIER vec<2, T, P> operator|(vec<1, T, P> const & v1, vec<2, T, P> const & v2)
	{
		return vec<2, T, P>(
			v1.x | v2.x,
			v1.x | v2.y);
	}

	template<typename T, precision P>
	GLM_FUNC_QUALIFIER vec<2, T, P> operator|(vec<2, T, P> const & v1, vec<2, T, P> const & v2)
	{
		return vec<2, T, P>(
			v1.x | v2.x,
			v1.y | v2.y);
	}

	template<typename T, precision P>
	GLM_FUNC_QUALIFIER vec<2, T, P> operator^(vec<2, T, P> const & v, T scalar)
	{
		return vec<2, T, P>(
			v.x ^ scalar,
			v.y ^ scalar);
	}

	template<typename T, precision P>
	GLM_FUNC_QUALIFIER vec<2, T, P> operator^(vec<2, T, P> const & v1, vec<1, T, P> const & v2)
	{
		return vec<2, T, P>(
			v1.x ^ v2.x,
			v1.y ^ v2.x);
	}

	template<typename T, precision P>
	GLM_FUNC_QUALIFIER vec<2, T, P> operator^(T scalar, vec<2, T, P> const & v)
	{
		return vec<2, T, P>(
			scalar ^ v.x,
			scalar ^ v.y);
	}

	template<typename T, precision P>
	GLM_FUNC_QUALIFIER vec<2, T, P> operator^(vec<1, T, P> const & v1, vec<2, T, P> const & v2)
	{
		return vec<2, T, P>(
			v1.x ^ v2.x,
			v1.x ^ v2.y);
	}

	template<typename T, precision P>
	GLM_FUNC_QUALIFIER vec<2, T, P> operator^(vec<2, T, P> const & v1, vec<2, T, P> const & v2)
	{
		return vec<2, T, P>(
			v1.x ^ v2.x,
			v1.y ^ v2.y);
	}

	template<typename T, precision P>
	GLM_FUNC_QUALIFIER vec<2, T, P> operator<<(vec<2, T, P> const & v, T scalar)
	{
		return vec<2, T, P>(
			v.x << scalar,
			v.y << scalar);
	}

	template<typename T, precision P>
	GLM_FUNC_QUALIFIER vec<2, T, P> operator<<(vec<2, T, P> const & v1, vec<1, T, P> const & v2)
	{
		return vec<2, T, P>(
			v1.x << v2.x,
			v1.y << v2.x);
	}

	template<typename T, precision P>
	GLM_FUNC_QUALIFIER vec<2, T, P> operator<<(T scalar, vec<2, T, P> const & v)
	{
		return vec<2, T, P>(
			scalar << v.x,
			scalar << v.y);
	}

	template<typename T, precision P>
	GLM_FUNC_QUALIFIER vec<2, T, P> operator<<(vec<1, T, P> const & v1, vec<2, T, P> const & v2)
	{
		return vec<2, T, P>(
			v1.x << v2.x,
			v1.x << v2.y);
	}

	template<typename T, precision P>
	GLM_FUNC_QUALIFIER vec<2, T, P> operator<<(vec<2, T, P> const & v1, vec<2, T, P> const & v2)
	{
		return vec<2, T, P>(
			v1.x << v2.x,
			v1.y << v2.y);
	}

	template<typename T, precision P>
	GLM_FUNC_QUALIFIER vec<2, T, P> operator>>(vec<2, T, P> const & v, T scalar)
	{
		return vec<2, T, P>(
			v.x >> scalar,
			v.y >> scalar);
	}

	template<typename T, precision P>
	GLM_FUNC_QUALIFIER vec<2, T, P> operator>>(vec<2, T, P> const & v1, vec<1, T, P> const & v2)
	{
		return vec<2, T, P>(
			v1.x >> v2.x,
			v1.y >> v2.x);
	}

	template<typename T, precision P>
	GLM_FUNC_QUALIFIER vec<2, T, P> operator>>(T scalar, vec<2, T, P> const & v)
	{
		return vec<2, T, P>(
			scalar >> v.x,
			scalar >> v.y);
	}

	template<typename T, precision P>
	GLM_FUNC_QUALIFIER vec<2, T, P> operator>>(vec<1, T, P> const & v1, vec<2, T, P> const & v2)
	{
		return vec<2, T, P>(
			v1.x >> v2.x,
			v1.x >> v2.y);
	}

	template<typename T, precision P>
	GLM_FUNC_QUALIFIER vec<2, T, P> operator>>(vec<2, T, P> const & v1, vec<2, T, P> const & v2)
	{
		return vec<2, T, P>(
			v1.x >> v2.x,
			v1.y >> v2.y);
	}

	template<typename T, precision P>
	GLM_FUNC_QUALIFIER vec<2, T, P> operator~(vec<2, T, P> const & v)
	{
		return vec<2, T, P>(
			~v.x,
			~v.y);
	}

	// -- Boolean operators --

	template<typename T, precision P>
	GLM_FUNC_QUALIFIER bool operator==(vec<2, T, P> const & v1, vec<2, T, P> const & v2)
	{
		return (v1.x == v2.x) && (v1.y == v2.y);
	}

	template<typename T, precision P>
	GLM_FUNC_QUALIFIER bool operator!=(vec<2, T, P> const & v1, vec<2, T, P> const & v2)
	{
		return (v1.x != v2.x) || (v1.y != v2.y);
	}

	template<precision P>
	GLM_FUNC_QUALIFIER vec<2, bool, P> operator&&(vec<2, bool, P> const & v1, vec<2, bool, P> const & v2)
	{
		return vec<2, bool, P>(v1.x && v2.x, v1.y && v2.y);
	}

	template<precision P>
	GLM_FUNC_QUALIFIER vec<2, bool, P> operator||(vec<2, bool, P> const & v1, vec<2, bool, P> const & v2)
	{
		return vec<2, bool, P>(v1.x || v2.x, v1.y || v2.y);
	}
}//namespace glm
