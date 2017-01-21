/// @ref core
/// @file glm/detail/type_vec1.inl

namespace glm
{
	// -- Implicit basic constructors --

#	if !GLM_HAS_DEFAULTED_FUNCTIONS || !defined(GLM_FORCE_NO_CTOR_INIT)
		template<typename T, precision P>
		GLM_FUNC_QUALIFIER GLM_CONSTEXPR_CTOR vec<1, T, P>::vec()
#			ifndef GLM_FORCE_NO_CTOR_INIT
				: x(0)
#			endif
		{}
#	endif//!GLM_HAS_DEFAULTED_FUNCTIONS

#	if !GLM_HAS_DEFAULTED_FUNCTIONS
		template<typename T, precision P>
		GLM_FUNC_QUALIFIER GLM_CONSTEXPR_CTOR vec<1, T, P>::vec(vec<1, T, P> const & v)
			: x(v.x)
		{}
#	endif//!GLM_HAS_DEFAULTED_FUNCTIONS

	template<typename T, precision P>
	template<precision Q>
	GLM_FUNC_QUALIFIER GLM_CONSTEXPR_CTOR vec<1, T, P>::vec(vec<1, T, Q> const& v)
		: x(v.x)
	{}

	// -- Explicit basic constructors --

	template<typename T, precision P>
	GLM_FUNC_QUALIFIER GLM_CONSTEXPR_CTOR vec<1, T, P>::vec(ctor)
	{}

	template<typename T, precision P>
	GLM_FUNC_QUALIFIER GLM_CONSTEXPR_CTOR vec<1, T, P>::vec(T scalar)
		: x(scalar)
	{}

	// -- Conversion vector constructors --

	template<typename T, precision P>
	template<typename U, precision Q>
	GLM_FUNC_QUALIFIER GLM_CONSTEXPR_CTOR vec<1, T, P>::vec(vec<1, U, Q> const & v)
		: x(static_cast<T>(v.x))
	{}

	template<typename T, precision P>
	template<typename U, precision Q>
	GLM_FUNC_QUALIFIER GLM_CONSTEXPR_CTOR vec<1, T, P>::vec(vec<2, U, Q> const & v)
		: x(static_cast<T>(v.x))
	{}

	template<typename T, precision P>
	template<typename U, precision Q>
	GLM_FUNC_QUALIFIER GLM_CONSTEXPR_CTOR vec<1, T, P>::vec(vec<3, U, Q> const & v)
		: x(static_cast<T>(v.x))
	{}

	template<typename T, precision P>
	template<typename U, precision Q>
	GLM_FUNC_QUALIFIER GLM_CONSTEXPR_CTOR vec<1, T, P>::vec(vec<4, U, Q> const & v)
		: x(static_cast<T>(v.x))
	{}

	// -- Component accesses --

	template<typename T, precision P>
	GLM_FUNC_QUALIFIER T & vec<1, T, P>::operator[](typename vec<1, T, P>::length_type i)
	{
		assert(i >= 0 && i < this->length());
		return (&x)[i];
	}

	template<typename T, precision P>
	GLM_FUNC_QUALIFIER T const & vec<1, T, P>::operator[](typename vec<1, T, P>::length_type i) const
	{
		assert(i >= 0 && i < this->length());
		return (&x)[i];
	}

	// -- Unary arithmetic operators --

#	if !GLM_HAS_DEFAULTED_FUNCTIONS
		template<typename T, precision P>
		GLM_FUNC_QUALIFIER vec<1, T, P> & vec<1, T, P>::operator=(vec<1, T, P> const & v)
		{
			this->x = v.x;
			return *this;
		}
#	endif//!GLM_HAS_DEFAULTED_FUNCTIONS

	template<typename T, precision P>
	template<typename U> 
	GLM_FUNC_QUALIFIER vec<1, T, P> & vec<1, T, P>::operator=(vec<1, U, P> const & v)
	{
		this->x = static_cast<T>(v.x);
		return *this;
	}

	template<typename T, precision P>
	template<typename U> 
	GLM_FUNC_QUALIFIER vec<1, T, P> & vec<1, T, P>::operator+=(U scalar)
	{
		this->x += static_cast<T>(scalar);
		return *this;
	}

	template<typename T, precision P>
	template<typename U> 
	GLM_FUNC_QUALIFIER vec<1, T, P> & vec<1, T, P>::operator+=(vec<1, U, P> const & v)
	{
		this->x += static_cast<T>(v.x);
		return *this;
	}

	template<typename T, precision P>
	template<typename U> 
	GLM_FUNC_QUALIFIER vec<1, T, P> & vec<1, T, P>::operator-=(U scalar)
	{
		this->x -= static_cast<T>(scalar);
		return *this;
	}

	template<typename T, precision P>
	template<typename U> 
	GLM_FUNC_QUALIFIER vec<1, T, P> & vec<1, T, P>::operator-=(vec<1, U, P> const & v)
	{
		this->x -= static_cast<T>(v.x);
		return *this;
	}

	template<typename T, precision P>
	template<typename U> 
	GLM_FUNC_QUALIFIER vec<1, T, P> & vec<1, T, P>::operator*=(U scalar)
	{
		this->x *= static_cast<T>(scalar);
		return *this;
	}

	template<typename T, precision P>
	template<typename U> 
	GLM_FUNC_QUALIFIER vec<1, T, P> & vec<1, T, P>::operator*=(vec<1, U, P> const & v)
	{
		this->x *= static_cast<T>(v.x);
		return *this;
	}

	template<typename T, precision P>
	template<typename U> 
	GLM_FUNC_QUALIFIER vec<1, T, P> & vec<1, T, P>::operator/=(U scalar)
	{
		this->x /= static_cast<T>(scalar);
		return *this;
	}

	template<typename T, precision P>
	template<typename U> 
	GLM_FUNC_QUALIFIER vec<1, T, P> & vec<1, T, P>::operator/=(vec<1, U, P> const & v)
	{
		this->x /= static_cast<T>(v.x);
		return *this;
	}

	// -- Increment and decrement operators --

	template<typename T, precision P>
	GLM_FUNC_QUALIFIER vec<1, T, P> & vec<1, T, P>::operator++()
	{
		++this->x;
		return *this;
	}

	template<typename T, precision P>
	GLM_FUNC_QUALIFIER vec<1, T, P> & vec<1, T, P>::operator--()
	{
		--this->x;
		return *this;
	}

	template<typename T, precision P> 
	GLM_FUNC_QUALIFIER vec<1, T, P> vec<1, T, P>::operator++(int)
	{
		vec<1, T, P> Result(*this);
		++*this;
		return Result;
	}

	template<typename T, precision P> 
	GLM_FUNC_QUALIFIER vec<1, T, P> vec<1, T, P>::operator--(int)
	{
		vec<1, T, P> Result(*this);
		--*this;
		return Result;
	}

	// -- Unary bit operators --

	template<typename T, precision P>
	template<typename U> 
	GLM_FUNC_QUALIFIER vec<1, T, P> & vec<1, T, P>::operator%=(U scalar)
	{
		this->x %= static_cast<T>(scalar);
		return *this;
	}

	template<typename T, precision P>
	template<typename U> 
	GLM_FUNC_QUALIFIER vec<1, T, P> & vec<1, T, P>::operator%=(vec<1, U, P> const & v)
	{
		this->x %= static_cast<T>(v.x);
		return *this;
	}

	template<typename T, precision P>
	template<typename U> 
	GLM_FUNC_QUALIFIER vec<1, T, P> & vec<1, T, P>::operator&=(U scalar)
	{
		this->x &= static_cast<T>(scalar);
		return *this;
	}

	template<typename T, precision P>
	template<typename U> 
	GLM_FUNC_QUALIFIER vec<1, T, P> & vec<1, T, P>::operator&=(vec<1, U, P> const & v)
	{
		this->x &= static_cast<T>(v.x);
		return *this;
	}

	template<typename T, precision P>
	template<typename U> 
	GLM_FUNC_QUALIFIER vec<1, T, P> & vec<1, T, P>::operator|=(U scalar)
	{
		this->x |= static_cast<T>(scalar);
		return *this;
	}

	template<typename T, precision P>
	template<typename U> 
	GLM_FUNC_QUALIFIER vec<1, T, P> & vec<1, T, P>::operator|=(vec<1, U, P> const & v)
	{
		this->x |= U(v.x);
		return *this;
	}

	template<typename T, precision P>
	template<typename U> 
	GLM_FUNC_QUALIFIER vec<1, T, P> & vec<1, T, P>::operator^=(U scalar)
	{
		this->x ^= static_cast<T>(scalar);
		return *this;
	}

	template<typename T, precision P>
	template<typename U> 
	GLM_FUNC_QUALIFIER vec<1, T, P> & vec<1, T, P>::operator^=(vec<1, U, P> const & v)
	{
		this->x ^= static_cast<T>(v.x);
		return *this;
	}

	template<typename T, precision P>
	template<typename U> 
	GLM_FUNC_QUALIFIER vec<1, T, P> & vec<1, T, P>::operator<<=(U scalar)
	{
		this->x <<= static_cast<T>(scalar);
		return *this;
	}

	template<typename T, precision P>
	template<typename U> 
	GLM_FUNC_QUALIFIER vec<1, T, P> & vec<1, T, P>::operator<<=(vec<1, U, P> const & v)
	{
		this->x <<= static_cast<T>(v.x);
		return *this;
	}

	template<typename T, precision P>
	template<typename U> 
	GLM_FUNC_QUALIFIER vec<1, T, P> & vec<1, T, P>::operator>>=(U scalar)
	{
		this->x >>= static_cast<T>(scalar);
		return *this;
	}

	template<typename T, precision P>
	template<typename U> 
	GLM_FUNC_QUALIFIER vec<1, T, P> & vec<1, T, P>::operator>>=(vec<1, U, P> const & v)
	{
		this->x >>= static_cast<T>(v.x);
		return *this;
	}

	// -- Unary constant operators --

	template<typename T, precision P>
	GLM_FUNC_QUALIFIER vec<1, T, P> operator+(vec<1, T, P> const & v)
	{
		return v;
	}

	template<typename T, precision P>
	GLM_FUNC_QUALIFIER vec<1, T, P> operator-(vec<1, T, P> const & v)
	{
		return vec<1, T, P>(
			-v.x);
	}

	// -- Binary arithmetic operators --

	template<typename T, precision P> 
	GLM_FUNC_QUALIFIER vec<1, T, P> operator+(vec<1, T, P> const & v, T scalar)
	{
		return vec<1, T, P>(
			v.x + scalar);
	}

	template<typename T, precision P> 
	GLM_FUNC_QUALIFIER vec<1, T, P> operator+(T scalar, vec<1, T, P> const & v)
	{
		return vec<1, T, P>(
			scalar + v.x);
	}

	template<typename T, precision P>
	GLM_FUNC_QUALIFIER vec<1, T, P> operator+(vec<1, T, P> const & v1, vec<1, T, P> const & v2)
	{
		return vec<1, T, P>(
			v1.x + v2.x);
	}

	//operator-
	template<typename T, precision P> 
	GLM_FUNC_QUALIFIER vec<1, T, P> operator-(vec<1, T, P> const & v, T scalar)
	{
		return vec<1, T, P>(
			v.x - scalar);
	}

	template<typename T, precision P>
	GLM_FUNC_QUALIFIER vec<1, T, P> operator-(T scalar, vec<1, T, P> const & v)
	{
		return vec<1, T, P>(
			scalar - v.x);
	}

	template<typename T, precision P>
	GLM_FUNC_QUALIFIER vec<1, T, P> operator-(vec<1, T, P> const & v1, vec<1, T, P> const & v2)
	{
		return vec<1, T, P>(
			v1.x - v2.x);
	}

	template<typename T, precision P>
	GLM_FUNC_QUALIFIER vec<1, T, P> operator*(vec<1, T, P> const & v, T scalar)
	{
		return vec<1, T, P>(
			v.x * scalar);
	}

	template<typename T, precision P>
	GLM_FUNC_QUALIFIER vec<1, T, P> operator*(T scalar, vec<1, T, P> const & v)
	{
		return vec<1, T, P>(
			scalar * v.x);
	}

	template<typename T, precision P>
	GLM_FUNC_QUALIFIER vec<1, T, P> operator*(vec<1, T, P> const & v1, vec<1, T, P> const & v2)
	{
		return vec<1, T, P>(
			v1.x * v2.x);
	}

	template<typename T, precision P>
	GLM_FUNC_QUALIFIER vec<1, T, P> operator/(vec<1, T, P> const & v, T scalar)
	{
		return vec<1, T, P>(
			v.x / scalar);
	}

	template<typename T, precision P>
	GLM_FUNC_QUALIFIER vec<1, T, P> operator/(T scalar, vec<1, T, P> const & v)
	{
		return vec<1, T, P>(
			scalar / v.x);
	}

	template<typename T, precision P>
	GLM_FUNC_QUALIFIER vec<1, T, P> operator/(vec<1, T, P> const & v1, vec<1, T, P> const & v2)
	{
		return vec<1, T, P>(
			v1.x / v2.x);
	}

	// -- Binary bit operators --

	template<typename T, precision P>
	GLM_FUNC_QUALIFIER vec<1, T, P> operator%(vec<1, T, P> const & v, T scalar)
	{
		return vec<1, T, P>(
			v.x % scalar);
	}

	template<typename T, precision P>
	GLM_FUNC_QUALIFIER vec<1, T, P> operator%(T scalar, vec<1, T, P> const & v)
	{
		return vec<1, T, P>(
			scalar % v.x);
	}

	template<typename T, precision P>
	GLM_FUNC_QUALIFIER vec<1, T, P> operator%(vec<1, T, P> const & v1, vec<1, T, P> const & v2)
	{
		return vec<1, T, P>(
			v1.x % v2.x);
	}

	template<typename T, precision P>
	GLM_FUNC_QUALIFIER vec<1, T, P> operator&(vec<1, T, P> const & v, T scalar)
	{
		return vec<1, T, P>(
			v.x & scalar);
	}

	template<typename T, precision P>
	GLM_FUNC_QUALIFIER vec<1, T, P> operator&(T scalar, vec<1, T, P> const & v)
	{
		return vec<1, T, P>(
			scalar & v.x);
	}

	template<typename T, precision P>
	GLM_FUNC_QUALIFIER vec<1, T, P> operator&(vec<1, T, P> const & v1, vec<1, T, P> const & v2)
	{
		return vec<1, T, P>(
			v1.x & v2.x);
	}

	template<typename T, precision P>
	GLM_FUNC_QUALIFIER vec<1, T, P> operator|(vec<1, T, P> const & v, T scalar)
	{
		return vec<1, T, P>(
			v.x | scalar);
	}

	template<typename T, precision P>
	GLM_FUNC_QUALIFIER vec<1, T, P> operator|(T scalar, vec<1, T, P> const & v)
	{
		return vec<1, T, P>(
			scalar | v.x);
	}

	template<typename T, precision P>
	GLM_FUNC_QUALIFIER vec<1, T, P> operator|(vec<1, T, P> const & v1, vec<1, T, P> const & v2)
	{
		return vec<1, T, P>(
			v1.x | v2.x);
	}
		
	template<typename T, precision P>
	GLM_FUNC_QUALIFIER vec<1, T, P> operator^(vec<1, T, P> const & v, T scalar)
	{
		return vec<1, T, P>(
			v.x ^ scalar);
	}

	template<typename T, precision P>
	GLM_FUNC_QUALIFIER vec<1, T, P> operator^(T scalar, vec<1, T, P> const & v)
	{
		return vec<1, T, P>(
			scalar ^ v.x);
	}

	template<typename T, precision P>
	GLM_FUNC_QUALIFIER vec<1, T, P> operator^(vec<1, T, P> const & v1, vec<1, T, P> const & v2)
	{
		return vec<1, T, P>(
			v1.x ^ v2.x);
	}

	template<typename T, precision P>
	GLM_FUNC_QUALIFIER vec<1, T, P> operator<<(vec<1, T, P> const & v, T scalar)
	{
		return vec<1, T, P>(
			v.x << scalar);
	}

	template<typename T, precision P>
	GLM_FUNC_QUALIFIER vec<1, T, P> operator<<(T scalar, vec<1, T, P> const & v)
	{
		return vec<1, T, P>(
			scalar << v.x);
	}

	template<typename T, precision P>
	GLM_FUNC_QUALIFIER vec<1, T, P> operator<<(vec<1, T, P> const & v1, vec<1, T, P> const & v2)
	{
		return vec<1, T, P>(
			v1.x << v2.x);
	}

	template<typename T, precision P>
	GLM_FUNC_QUALIFIER vec<1, T, P> operator>>(vec<1, T, P> const & v, T scalar)
	{
		return vec<1, T, P>(
			v.x >> scalar);
	}

	template<typename T, precision P>
	GLM_FUNC_QUALIFIER vec<1, T, P> operator>>(T scalar, vec<1, T, P> const & v)
	{
		return vec<1, T, P>(
			scalar >> v.x);
	}

	template<typename T, precision P>
	GLM_FUNC_QUALIFIER vec<1, T, P> operator>>(vec<1, T, P> const & v1, vec<1, T, P> const & v2)
	{
		return vec<1, T, P>(
			v1.x >> v2.x);
	}

	template<typename T, precision P>
	GLM_FUNC_QUALIFIER vec<1, T, P> operator~(vec<1, T, P> const & v)
	{
		return vec<1, T, P>(
			~v.x);
	}

	// -- Boolean operators --

	template<typename T, precision P> 
	GLM_FUNC_QUALIFIER bool operator==(vec<1, T, P> const & v1, vec<1, T, P> const & v2)
	{
		return (v1.x == v2.x);
	}

	template<typename T, precision P> 
	GLM_FUNC_QUALIFIER bool operator!=(vec<1, T, P> const & v1, vec<1, T, P> const & v2)
	{
		return (v1.x != v2.x);
	}

	template<precision P>
	GLM_FUNC_QUALIFIER vec<1, bool, P> operator&&(vec<1, bool, P> const & v1, vec<1, bool, P> const & v2)
	{
		return vec<1, bool, P>(v1.x && v2.x);
	}

	template<precision P>
	GLM_FUNC_QUALIFIER vec<1, bool, P> operator||(vec<1, bool, P> const & v1, vec<1, bool, P> const & v2)
	{
		return vec<1, bool, P>(v1.x || v2.x);
	}
}//namespace glm
