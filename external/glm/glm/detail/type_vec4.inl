/// @ref core
/// @file glm/detail/type_tvec4.inl

namespace glm{
namespace detail
{
	template<typename T>
	struct is_int
	{
		enum test {value = 0};
	};

	template<>
	struct is_int<uint32>
	{
		enum test {value = ~0};
	};

	template<>
	struct is_int<int32>
	{
		enum test {value = ~0};
	};

	template<>
	struct is_int<uint64>
	{
		enum test {value = ~0};
	};

	template<>
	struct is_int<int64>
	{
		enum test {value = ~0};
	};

	template<typename T, precision P, bool Aligned>
	struct compute_vec4_add
	{
		GLM_FUNC_QUALIFIER static vec<4, T, P> call(vec<4, T, P> const & a, vec<4, T, P> const & b)
		{
			return vec<4, T, P>(a.x + b.x, a.y + b.y, a.z + b.z, a.w + b.w);
		}
	};

	template<typename T, precision P, bool Aligned>
	struct compute_vec4_sub
	{
		GLM_FUNC_QUALIFIER static vec<4, T, P> call(vec<4, T, P> const & a, vec<4, T, P> const & b)
		{
			return vec<4, T, P>(a.x - b.x, a.y - b.y, a.z - b.z, a.w - b.w);
		}
	};

	template<typename T, precision P, bool Aligned>
	struct compute_vec4_mul
	{
		GLM_FUNC_QUALIFIER static vec<4, T, P> call(vec<4, T, P> const & a, vec<4, T, P> const & b)
		{
			return vec<4, T, P>(a.x * b.x, a.y * b.y, a.z * b.z, a.w * b.w);
		}
	};

	template<typename T, precision P, bool Aligned>
	struct compute_vec4_div
	{
		GLM_FUNC_QUALIFIER static vec<4, T, P> call(vec<4, T, P> const & a, vec<4, T, P> const & b)
		{
			return vec<4, T, P>(a.x / b.x, a.y / b.y, a.z / b.z, a.w / b.w);
		}
	};

	template<typename T, precision P, bool Aligned>
	struct compute_vec4_mod
	{
		GLM_FUNC_QUALIFIER static vec<4, T, P> call(vec<4, T, P> const & a, vec<4, T, P> const & b)
		{
			return vec<4, T, P>(a.x % b.x, a.y % b.y, a.z % b.z, a.w % b.w);
		}
	};

	template<typename T, precision P, int IsInt, std::size_t Size, bool Aligned>
	struct compute_vec4_and
	{
		GLM_FUNC_QUALIFIER static vec<4, T, P> call(vec<4, T, P> const & a, vec<4, T, P> const & b)
		{
			return vec<4, T, P>(a.x & b.x, a.y & b.y, a.z & b.z, a.w & b.w);
		}
	};

	template<typename T, precision P, int IsInt, std::size_t Size, bool Aligned>
	struct compute_vec4_or
	{
		GLM_FUNC_QUALIFIER static vec<4, T, P> call(vec<4, T, P> const & a, vec<4, T, P> const & b)
		{
			return vec<4, T, P>(a.x | b.x, a.y | b.y, a.z | b.z, a.w | b.w);
		}
	};

	template<typename T, precision P, int IsInt, std::size_t Size, bool Aligned>
	struct compute_vec4_xor
	{
		GLM_FUNC_QUALIFIER static vec<4, T, P> call(vec<4, T, P> const & a, vec<4, T, P> const & b)
		{
			return vec<4, T, P>(a.x ^ b.x, a.y ^ b.y, a.z ^ b.z, a.w ^ b.w);
		}
	};

	template<typename T, precision P, int IsInt, std::size_t Size, bool Aligned>
	struct compute_vec4_shift_left
	{
		GLM_FUNC_QUALIFIER static vec<4, T, P> call(vec<4, T, P> const & a, vec<4, T, P> const & b)
		{
			return vec<4, T, P>(a.x << b.x, a.y << b.y, a.z << b.z, a.w << b.w);
		}
	};

	template<typename T, precision P, int IsInt, std::size_t Size, bool Aligned>
	struct compute_vec4_shift_right
	{
		GLM_FUNC_QUALIFIER static vec<4, T, P> call(vec<4, T, P> const & a, vec<4, T, P> const & b)
		{
			return vec<4, T, P>(a.x >> b.x, a.y >> b.y, a.z >> b.z, a.w >> b.w);
		}
	};

	template<typename T, precision P, int IsInt, std::size_t Size, bool Aligned>
	struct compute_vec4_equal
	{
		GLM_FUNC_QUALIFIER static bool call(vec<4, T, P> const & v1, vec<4, T, P> const & v2)
		{
			return (v1.x == v2.x) && (v1.y == v2.y) && (v1.z == v2.z) && (v1.w == v2.w);
		}
	};

	template<typename T, precision P, int IsInt, std::size_t Size, bool Aligned>
	struct compute_vec4_nequal
	{
		GLM_FUNC_QUALIFIER static bool call(vec<4, T, P> const & v1, vec<4, T, P> const & v2)
		{
			return (v1.x != v2.x) || (v1.y != v2.y) || (v1.z != v2.z) || (v1.w != v2.w);
		}
	};

	template<typename T, precision P, int IsInt, std::size_t Size, bool Aligned>
	struct compute_vec4_bitwise_not
	{
		GLM_FUNC_QUALIFIER static vec<4, T, P> call(vec<4, T, P> const & v)
		{
			return vec<4, T, P>(~v.x, ~v.y, ~v.z, ~v.w);
		}
	};
}//namespace detail

	// -- Implicit basic constructors --

#	if !GLM_HAS_DEFAULTED_FUNCTIONS || !defined(GLM_FORCE_NO_CTOR_INIT)
		template<typename T, precision P>
		GLM_FUNC_QUALIFIER GLM_CONSTEXPR_SIMD vec<4, T, P>::vec()
#			ifndef GLM_FORCE_NO_CTOR_INIT
				: x(0), y(0), z(0), w(0)
#			endif
		{}
#	endif//!GLM_HAS_DEFAULTED_FUNCTIONS

#	if !GLM_HAS_DEFAULTED_FUNCTIONS
		template<typename T, precision P>
		GLM_FUNC_QUALIFIER GLM_CONSTEXPR_SIMD vec<4, T, P>::vec(vec<4, T, P> const & v)
			: x(v.x), y(v.y), z(v.z), w(v.w)
		{}
#	endif//!GLM_HAS_DEFAULTED_FUNCTIONS

	template<typename T, precision P>
	template<precision Q>
	GLM_FUNC_QUALIFIER GLM_CONSTEXPR_SIMD vec<4, T, P>::vec(vec<4, T, Q> const & v)
		: x(v.x), y(v.y), z(v.z), w(v.w)
	{}

	// -- Explicit basic constructors --

	template<typename T, precision P>
	GLM_FUNC_QUALIFIER GLM_CONSTEXPR_SIMD vec<4, T, P>::vec(ctor)
	{}

	template<typename T, precision P>
	GLM_FUNC_QUALIFIER GLM_CONSTEXPR_SIMD vec<4, T, P>::vec(T scalar)
		: x(scalar), y(scalar), z(scalar), w(scalar)
	{}

	template <typename T, precision P>
	GLM_FUNC_QUALIFIER GLM_CONSTEXPR_SIMD vec<4, T, P>::vec(T _x, T _y, T _z, T _w)
		: x(_x), y(_y), z(_z), w(_w)
	{}

	// -- Conversion scalar constructors --

	template<typename T, precision P>
	template<typename X, typename Y, typename Z, typename W>
	GLM_FUNC_QUALIFIER GLM_CONSTEXPR_SIMD vec<4, T, P>::vec(X _x, Y _y, Z _z, W _w)
		: x(static_cast<T>(_x))
		, y(static_cast<T>(_y))
		, z(static_cast<T>(_z))
		, w(static_cast<T>(_w))
	{}

	template<typename T, precision P>
	template<typename X, typename Y, typename Z, typename W>
	GLM_FUNC_QUALIFIER GLM_CONSTEXPR_CTOR vec<4, T, P>::vec(vec<1, X, P> const& _x, vec<1, Y, P> const& _y, vec<1, Z, P> const& _z, vec<1, W, P> const& _w)
		: x(static_cast<T>(_x.x))
		, y(static_cast<T>(_y.x))
		, z(static_cast<T>(_z.x))
		, w(static_cast<T>(_w.x))
	{}

	// -- Conversion vector constructors --

	template<typename T, precision P>
	template<typename A, typename B, typename C, precision Q>
	GLM_FUNC_QUALIFIER GLM_CONSTEXPR_CTOR vec<4, T, P>::vec(vec<2, A, Q> const& _xy, B _z, C _w)
		: x(static_cast<T>(_xy.x))
		, y(static_cast<T>(_xy.y))
		, z(static_cast<T>(_z))
		, w(static_cast<T>(_w))
	{}

	template<typename T, precision P>
	template<typename A, typename B, typename C, precision Q>
	GLM_FUNC_QUALIFIER GLM_CONSTEXPR_CTOR vec<4, T, P>::vec(vec<2, A, Q> const& _xy, vec<1, B, Q> const& _z, vec<1, C, Q> const& _w)
		: x(static_cast<T>(_xy.x))
		, y(static_cast<T>(_xy.y))
		, z(static_cast<T>(_z.x))
		, w(static_cast<T>(_w.x))
	{}

	template<typename T, precision P>
	template<typename A, typename B, typename C, precision Q>
	GLM_FUNC_QUALIFIER GLM_CONSTEXPR_CTOR vec<4, T, P>::vec(A _x, vec<2, B, Q> const& _yz, C _w)
		: x(static_cast<T>(_x))
		, y(static_cast<T>(_yz.x))
		, z(static_cast<T>(_yz.y))
		, w(static_cast<T>(_w))
	{}

	template<typename T, precision P>
	template<typename A, typename B, typename C, precision Q>
	GLM_FUNC_QUALIFIER GLM_CONSTEXPR_CTOR vec<4, T, P>::vec(vec<1, A, Q> const& _x, vec<2, B, Q> const& _yz, vec<1, C, Q> const& _w)
		: x(static_cast<T>(_x.x))
		, y(static_cast<T>(_yz.x))
		, z(static_cast<T>(_yz.y))
		, w(static_cast<T>(_w.x))
	{}

	template<typename T, precision P>
	template<typename A, typename B, typename C, precision Q>
	GLM_FUNC_QUALIFIER GLM_CONSTEXPR_CTOR vec<4, T, P>::vec(A _x, B _y, vec<2, C, Q> const& _zw)
		: x(static_cast<T>(_x))
		, y(static_cast<T>(_y))
		, z(static_cast<T>(_zw.x))
		, w(static_cast<T>(_zw.y))
	{}

	template<typename T, precision P>
	template<typename A, typename B, typename C, precision Q>
	GLM_FUNC_QUALIFIER GLM_CONSTEXPR_CTOR vec<4, T, P>::vec(vec<1, A, Q> const& _x, vec<1, B, Q> const& _y, vec<2, C, Q> const& _zw)
		: x(static_cast<T>(_x.x))
		, y(static_cast<T>(_y.x))
		, z(static_cast<T>(_zw.x))
		, w(static_cast<T>(_zw.y))
	{}

	template<typename T, precision P>
	template<typename A, typename B, precision Q>
	GLM_FUNC_QUALIFIER GLM_CONSTEXPR_CTOR vec<4, T, P>::vec(vec<3, A, Q> const& _xyz, B _w) :
		x(static_cast<T>(_xyz.x)),
		y(static_cast<T>(_xyz.y)),
		z(static_cast<T>(_xyz.z)),
		w(static_cast<T>(_w))
	{}

	template<typename T, precision P>
	template<typename A, typename B, precision Q>
	GLM_FUNC_QUALIFIER GLM_CONSTEXPR_CTOR vec<4, T, P>::vec(vec<3, A, Q> const& _xyz, vec<1, B, Q> const& _w) :
		x(static_cast<T>(_xyz.x)),
		y(static_cast<T>(_xyz.y)),
		z(static_cast<T>(_xyz.z)),
		w(static_cast<T>(_w.x))
	{}

	template<typename T, precision P>
	template<typename A, typename B, precision Q>
	GLM_FUNC_QUALIFIER GLM_CONSTEXPR_CTOR vec<4, T, P>::vec(A _x, vec<3, B, Q> const& _yzw) :
		x(static_cast<T>(_x)),
		y(static_cast<T>(_yzw.x)),
		z(static_cast<T>(_yzw.y)),
		w(static_cast<T>(_yzw.z))
	{}

	template<typename T, precision P>
	template<typename A, typename B, precision Q>
	GLM_FUNC_QUALIFIER GLM_CONSTEXPR_CTOR vec<4, T, P>::vec(vec<1, A, Q> const& _x, vec<3, B, Q> const& _yzw) :
		x(static_cast<T>(_x.x)),
		y(static_cast<T>(_yzw.x)),
		z(static_cast<T>(_yzw.y)),
		w(static_cast<T>(_yzw.z))
	{}

	template<typename T, precision P>
	template<typename A, typename B, precision Q>
	GLM_FUNC_QUALIFIER GLM_CONSTEXPR_CTOR vec<4, T, P>::vec(vec<2, A, Q> const& _xy, vec<2, B, Q> const& _zw) :
		x(static_cast<T>(_xy.x)),
		y(static_cast<T>(_xy.y)),
		z(static_cast<T>(_zw.x)),
		w(static_cast<T>(_zw.y))
	{}

	template<typename T, precision P>
	template<typename U, precision Q>
	GLM_FUNC_QUALIFIER GLM_CONSTEXPR_CTOR vec<4, T, P>::vec(vec<4, U, Q> const& v) :
		x(static_cast<T>(v.x)),
		y(static_cast<T>(v.y)),
		z(static_cast<T>(v.z)),
		w(static_cast<T>(v.w))
	{}

	// -- Component accesses --

	template<typename T, precision P>
	GLM_FUNC_QUALIFIER T& vec<4, T, P>::operator[](typename vec<4, T, P>::length_type i)
	{
		assert(i >= 0 && i < this->length());
		return (&x)[i];
	}

	template<typename T, precision P>
	GLM_FUNC_QUALIFIER T const& vec<4, T, P>::operator[](typename vec<4, T, P>::length_type i) const
	{
		assert(i >= 0 && i < this->length());
		return (&x)[i];
	}

	// -- Unary arithmetic operators --

#	if !GLM_HAS_DEFAULTED_FUNCTIONS
		template<typename T, precision P>
		GLM_FUNC_QUALIFIER vec<4, T, P>& vec<4, T, P>::operator=(vec<4, T, P> const & v)
		{
			this->x = v.x;
			this->y = v.y;
			this->z = v.z;
			this->w = v.w;
			return *this;
		}
#	endif//!GLM_HAS_DEFAULTED_FUNCTIONS

	template<typename T, precision P>
	template<typename U>
	GLM_FUNC_QUALIFIER vec<4, T, P>& vec<4, T, P>::operator=(vec<4, U, P> const & v)
	{
		this->x = static_cast<T>(v.x);
		this->y = static_cast<T>(v.y);
		this->z = static_cast<T>(v.z);
		this->w = static_cast<T>(v.w);
		return *this;
	}

	template<typename T, precision P>
	template<typename U>
	GLM_FUNC_QUALIFIER vec<4, T, P> & vec<4, T, P>::operator+=(U scalar)
	{
		return (*this = detail::compute_vec4_add<T, P, detail::is_aligned<P>::value>::call(*this, vec<4, T, P>(scalar)));
	}

	template<typename T, precision P>
	template<typename U>
	GLM_FUNC_QUALIFIER vec<4, T, P> & vec<4, T, P>::operator+=(vec<1, U, P> const & v)
	{
		return (*this = detail::compute_vec4_add<T, P, detail::is_aligned<P>::value>::call(*this, vec<4, T, P>(v.x)));
	}

	template<typename T, precision P>
	template<typename U>
	GLM_FUNC_QUALIFIER vec<4, T, P> & vec<4, T, P>::operator+=(vec<4, U, P> const & v)
	{
		return (*this = detail::compute_vec4_add<T, P, detail::is_aligned<P>::value>::call(*this, vec<4, T, P>(v)));
	}

	template<typename T, precision P>
	template<typename U>
	GLM_FUNC_QUALIFIER vec<4, T, P> & vec<4, T, P>::operator-=(U scalar)
	{
		return (*this = detail::compute_vec4_sub<T, P, detail::is_aligned<P>::value>::call(*this, vec<4, T, P>(scalar)));
	}

	template<typename T, precision P>
	template<typename U>
	GLM_FUNC_QUALIFIER vec<4, T, P> & vec<4, T, P>::operator-=(vec<1, U, P> const & v)
	{
		return (*this = detail::compute_vec4_sub<T, P, detail::is_aligned<P>::value>::call(*this, vec<4, T, P>(v.x)));
	}

	template<typename T, precision P>
	template<typename U>
	GLM_FUNC_QUALIFIER vec<4, T, P> & vec<4, T, P>::operator-=(vec<4, U, P> const & v)
	{
		return (*this = detail::compute_vec4_sub<T, P, detail::is_aligned<P>::value>::call(*this, vec<4, T, P>(v)));
	}

	template<typename T, precision P>
	template<typename U>
	GLM_FUNC_QUALIFIER vec<4, T, P> & vec<4, T, P>::operator*=(U scalar)
	{
		return (*this = detail::compute_vec4_mul<T, P, detail::is_aligned<P>::value>::call(*this, vec<4, T, P>(scalar)));
	}

	template<typename T, precision P>
	template<typename U>
	GLM_FUNC_QUALIFIER vec<4, T, P> & vec<4, T, P>::operator*=(vec<1, U, P> const & v)
	{
		return (*this = detail::compute_vec4_mul<T, P, detail::is_aligned<P>::value>::call(*this, vec<4, T, P>(v.x)));
	}

	template<typename T, precision P>
	template<typename U>
	GLM_FUNC_QUALIFIER vec<4, T, P> & vec<4, T, P>::operator*=(vec<4, U, P> const & v)
	{
		return (*this = detail::compute_vec4_mul<T, P, detail::is_aligned<P>::value>::call(*this, vec<4, T, P>(v)));
	}

	template<typename T, precision P>
	template<typename U>
	GLM_FUNC_QUALIFIER vec<4, T, P> & vec<4, T, P>::operator/=(U scalar)
	{
		return (*this = detail::compute_vec4_div<T, P, detail::is_aligned<P>::value>::call(*this, vec<4, T, P>(scalar)));
	}

	template<typename T, precision P>
	template<typename U>
	GLM_FUNC_QUALIFIER vec<4, T, P> & vec<4, T, P>::operator/=(vec<1, U, P> const & v)
	{
		return (*this = detail::compute_vec4_div<T, P, detail::is_aligned<P>::value>::call(*this, vec<4, T, P>(v.x)));
	}

	template<typename T, precision P>
	template<typename U>
	GLM_FUNC_QUALIFIER vec<4, T, P> & vec<4, T, P>::operator/=(vec<4, U, P> const & v)
	{
		return (*this = detail::compute_vec4_div<T, P, detail::is_aligned<P>::value>::call(*this, vec<4, T, P>(v)));
	}

	// -- Increment and decrement operators --

	template<typename T, precision P>
	GLM_FUNC_QUALIFIER vec<4, T, P> & vec<4, T, P>::operator++()
	{
		++this->x;
		++this->y;
		++this->z;
		++this->w;
		return *this;
	}

	template<typename T, precision P>
	GLM_FUNC_QUALIFIER vec<4, T, P> & vec<4, T, P>::operator--()
	{
		--this->x;
		--this->y;
		--this->z;
		--this->w;
		return *this;
	}

	template<typename T, precision P>
	GLM_FUNC_QUALIFIER vec<4, T, P> vec<4, T, P>::operator++(int)
	{
		vec<4, T, P> Result(*this);
		++*this;
		return Result;
	}

	template<typename T, precision P>
	GLM_FUNC_QUALIFIER vec<4, T, P> vec<4, T, P>::operator--(int)
	{
		vec<4, T, P> Result(*this);
		--*this;
		return Result;
	}

	// -- Unary bit operators --

	template<typename T, precision P>
	template<typename U>
	GLM_FUNC_QUALIFIER vec<4, T, P> & vec<4, T, P>::operator%=(U scalar)
	{
		return (*this = detail::compute_vec4_mod<T, P, detail::is_aligned<P>::value>::call(*this, vec<4, T, P>(scalar)));
	}

	template<typename T, precision P>
	template<typename U>
	GLM_FUNC_QUALIFIER vec<4, T, P> & vec<4, T, P>::operator%=(vec<1, U, P> const& v)
	{
		return (*this = detail::compute_vec4_mod<T, P, detail::is_aligned<P>::value>::call(*this, vec<4, T, P>(v)));
	}

	template<typename T, precision P>
	template<typename U>
	GLM_FUNC_QUALIFIER vec<4, T, P> & vec<4, T, P>::operator%=(vec<4, U, P> const& v)
	{
		return (*this = detail::compute_vec4_mod<T, P, detail::is_aligned<P>::value>::call(*this, vec<4, T, P>(v)));
	}

	template<typename T, precision P>
	template<typename U>
	GLM_FUNC_QUALIFIER vec<4, T, P> & vec<4, T, P>::operator&=(U scalar)
	{
		return (*this = detail::compute_vec4_and<T, P, detail::is_int<T>::value, sizeof(T) * 8, detail::is_aligned<P>::value>::call(*this, vec<4, T, P>(scalar)));
	}

	template<typename T, precision P>
	template<typename U>
	GLM_FUNC_QUALIFIER vec<4, T, P> & vec<4, T, P>::operator&=(vec<1, U, P> const & v)
	{
		return (*this = detail::compute_vec4_and<T, P, detail::is_int<T>::value, sizeof(T) * 8, detail::is_aligned<P>::value>::call(*this, vec<4, T, P>(v)));
	}

	template<typename T, precision P>
	template<typename U>
	GLM_FUNC_QUALIFIER vec<4, T, P> & vec<4, T, P>::operator&=(vec<4, U, P> const & v)
	{
		return (*this = detail::compute_vec4_and<T, P, detail::is_int<T>::value, sizeof(T) * 8, detail::is_aligned<P>::value>::call(*this, vec<4, T, P>(v)));
	}

	template<typename T, precision P>
	template<typename U>
	GLM_FUNC_QUALIFIER vec<4, T, P> & vec<4, T, P>::operator|=(U scalar)
	{
		return (*this = detail::compute_vec4_or<T, P, detail::is_int<T>::value, sizeof(T) * 8, detail::is_aligned<P>::value>::call(*this, vec<4, T, P>(scalar)));
	}

	template<typename T, precision P>
	template<typename U>
	GLM_FUNC_QUALIFIER vec<4, T, P> & vec<4, T, P>::operator|=(vec<1, U, P> const & v)
	{
		return (*this = detail::compute_vec4_or<T, P, detail::is_int<T>::value, sizeof(T) * 8, detail::is_aligned<P>::value>::call(*this, vec<4, T, P>(v)));
	}

	template<typename T, precision P>
	template<typename U>
	GLM_FUNC_QUALIFIER vec<4, T, P> & vec<4, T, P>::operator|=(vec<4, U, P> const & v)
	{
		return (*this = detail::compute_vec4_or<T, P, detail::is_int<T>::value, sizeof(T) * 8, detail::is_aligned<P>::value>::call(*this, vec<4, T, P>(v)));
	}

	template<typename T, precision P>
	template<typename U>
	GLM_FUNC_QUALIFIER vec<4, T, P> & vec<4, T, P>::operator^=(U scalar)
	{
		return (*this = detail::compute_vec4_xor<T, P, detail::is_int<T>::value, sizeof(T) * 8, detail::is_aligned<P>::value>::call(*this, vec<4, T, P>(scalar)));
	}

	template<typename T, precision P>
	template<typename U>
	GLM_FUNC_QUALIFIER vec<4, T, P> & vec<4, T, P>::operator^=(vec<1, U, P> const & v)
	{
		return (*this = detail::compute_vec4_xor<T, P, detail::is_int<T>::value, sizeof(T) * 8, detail::is_aligned<P>::value>::call(*this, vec<4, T, P>(v)));
	}

	template<typename T, precision P>
	template<typename U>
	GLM_FUNC_QUALIFIER vec<4, T, P> & vec<4, T, P>::operator^=(vec<4, U, P> const & v)
	{
		return (*this = detail::compute_vec4_xor<T, P, detail::is_int<T>::value, sizeof(T) * 8, detail::is_aligned<P>::value>::call(*this, vec<4, T, P>(v)));
	}

	template<typename T, precision P>
	template<typename U>
	GLM_FUNC_QUALIFIER vec<4, T, P> & vec<4, T, P>::operator<<=(U scalar)
	{
		return (*this = detail::compute_vec4_shift_left<T, P, detail::is_int<T>::value, sizeof(T) * 8, detail::is_aligned<P>::value>::call(*this, vec<4, T, P>(scalar)));
	}

	template<typename T, precision P>
	template<typename U>
	GLM_FUNC_QUALIFIER vec<4, T, P> & vec<4, T, P>::operator<<=(vec<1, U, P> const & v)
	{
		return (*this = detail::compute_vec4_shift_left<T, P, detail::is_int<T>::value, sizeof(T) * 8, detail::is_aligned<P>::value>::call(*this, vec<4, T, P>(v)));
	}

	template<typename T, precision P>
	template<typename U>
	GLM_FUNC_QUALIFIER vec<4, T, P> & vec<4, T, P>::operator<<=(vec<4, U, P> const & v)
	{
		return (*this = detail::compute_vec4_shift_left<T, P, detail::is_int<T>::value, sizeof(T) * 8, detail::is_aligned<P>::value>::call(*this, vec<4, T, P>(v)));
	}

	template<typename T, precision P>
	template<typename U>
	GLM_FUNC_QUALIFIER vec<4, T, P> & vec<4, T, P>::operator>>=(U scalar)
	{
		return (*this = detail::compute_vec4_shift_right<T, P, detail::is_int<T>::value, sizeof(T) * 8, detail::is_aligned<P>::value>::call(*this, vec<4, T, P>(scalar)));
	}

	template<typename T, precision P>
	template<typename U>
	GLM_FUNC_QUALIFIER vec<4, T, P> & vec<4, T, P>::operator>>=(vec<1, U, P> const & v)
	{
		return (*this = detail::compute_vec4_shift_right<T, P, detail::is_int<T>::value, sizeof(T) * 8, detail::is_aligned<P>::value>::call(*this, vec<4, T, P>(v)));
	}

	template<typename T, precision P>
	template<typename U>
	GLM_FUNC_QUALIFIER vec<4, T, P> & vec<4, T, P>::operator>>=(vec<4, U, P> const & v)
	{
		return (*this = detail::compute_vec4_shift_right<T, P, detail::is_int<T>::value, sizeof(T) * 8, detail::is_aligned<P>::value>::call(*this, vec<4, T, P>(v)));
	}

	// -- Unary constant operators --

	template<typename T, precision P>
	GLM_FUNC_QUALIFIER vec<4, T, P> operator+(vec<4, T, P> const & v)
	{
		return v;
	}

	template<typename T, precision P>
	GLM_FUNC_QUALIFIER vec<4, T, P> operator-(vec<4, T, P> const & v)
	{
		return vec<4, T, P>(0) -= v;
	}

	// -- Binary arithmetic operators --

	template<typename T, precision P>
	GLM_FUNC_QUALIFIER vec<4, T, P> operator+(vec<4, T, P> const & v, T scalar)
	{
		return vec<4, T, P>(v) += scalar;
	}

	template<typename T, precision P>
	GLM_FUNC_QUALIFIER vec<4, T, P> operator+(vec<4, T, P> const & v1, vec<1, T, P> const & v2)
	{
		return vec<4, T, P>(v1) += v2;
	}

	template<typename T, precision P>
	GLM_FUNC_QUALIFIER vec<4, T, P> operator+(T scalar, vec<4, T, P> const & v)
	{
		return vec<4, T, P>(v) += scalar;
	}

	template<typename T, precision P>
	GLM_FUNC_QUALIFIER vec<4, T, P> operator+(vec<1, T, P> const & v1, vec<4, T, P> const & v2)
	{
		return vec<4, T, P>(v2) += v1;
	}

	template<typename T, precision P>
	GLM_FUNC_QUALIFIER vec<4, T, P> operator+(vec<4, T, P> const & v1, vec<4, T, P> const & v2)
	{
		return vec<4, T, P>(v1) += v2;
	}

	template<typename T, precision P>
	GLM_FUNC_QUALIFIER vec<4, T, P> operator-(vec<4, T, P> const & v, T scalar)
	{
		return vec<4, T, P>(v) -= scalar;
	}

	template<typename T, precision P>
	GLM_FUNC_QUALIFIER vec<4, T, P> operator-(vec<4, T, P> const & v1, vec<1, T, P> const & v2)
	{
		return vec<4, T, P>(v1) -= v2;
	}

	template<typename T, precision P>
	GLM_FUNC_QUALIFIER vec<4, T, P> operator-(T scalar, vec<4, T, P> const & v)
	{
		return vec<4, T, P>(scalar) -= v;
	}

	template<typename T, precision P>
	GLM_FUNC_QUALIFIER vec<4, T, P> operator-(vec<1, T, P> const & v1, vec<4, T, P> const & v2)
	{
		return vec<4, T, P>(v1.x) -= v2;
	}

	template<typename T, precision P>
	GLM_FUNC_QUALIFIER vec<4, T, P> operator-(vec<4, T, P> const & v1, vec<4, T, P> const & v2)
	{
		return vec<4, T, P>(v1) -= v2;
	}

	template<typename T, precision P>
	GLM_FUNC_QUALIFIER vec<4, T, P> operator*(vec<4, T, P> const & v, T scalar)
	{
		return vec<4, T, P>(v) *= scalar;
	}

	template<typename T, precision P>
	GLM_FUNC_QUALIFIER vec<4, T, P> operator*(vec<4, T, P> const & v1, vec<1, T, P> const & v2)
	{
		return vec<4, T, P>(v1) *= v2;
	}

	template<typename T, precision P>
	GLM_FUNC_QUALIFIER vec<4, T, P> operator*(T scalar, vec<4, T, P> const & v)
	{
		return vec<4, T, P>(v) *= scalar;
	}

	template<typename T, precision P>
	GLM_FUNC_QUALIFIER vec<4, T, P> operator*(vec<1, T, P> const & v1, vec<4, T, P> const & v2)
	{
		return vec<4, T, P>(v2) *= v1;
	}

	template<typename T, precision P>
	GLM_FUNC_QUALIFIER vec<4, T, P> operator*(vec<4, T, P> const & v1, vec<4, T, P> const & v2)
	{
		return vec<4, T, P>(v1) *= v2;
	}

	template<typename T, precision P>
	GLM_FUNC_QUALIFIER vec<4, T, P> operator/(vec<4, T, P> const & v, T scalar)
	{
		return vec<4, T, P>(v) /= scalar;
	}

	template<typename T, precision P>
	GLM_FUNC_QUALIFIER vec<4, T, P> operator/(vec<4, T, P> const & v1, vec<1, T, P> const & v2)
	{
		return vec<4, T, P>(v1) /= v2;
	}

	template<typename T, precision P>
	GLM_FUNC_QUALIFIER vec<4, T, P> operator/(T scalar, vec<4, T, P> const & v)
	{
		return vec<4, T, P>(scalar) /= v;
	}

	template<typename T, precision P>
	GLM_FUNC_QUALIFIER vec<4, T, P> operator/(vec<1, T, P> const & v1, vec<4, T, P> const & v2)
	{
		return vec<4, T, P>(v1.x) /= v2;
	}

	template<typename T, precision P>
	GLM_FUNC_QUALIFIER vec<4, T, P> operator/(vec<4, T, P> const & v1, vec<4, T, P> const & v2)
	{
		return vec<4, T, P>(v1) /= v2;
	}

	// -- Binary bit operators --

	template<typename T, precision P>
	GLM_FUNC_QUALIFIER vec<4, T, P> operator%(vec<4, T, P> const & v, T scalar)
	{
		return vec<4, T, P>(v) %= scalar;
	}

	template<typename T, precision P>
	GLM_FUNC_QUALIFIER vec<4, T, P> operator%(vec<4, T, P> const & v1, vec<1, T, P> const & v2)
	{
		return vec<4, T, P>(v1) %= v2.x;
	}

	template<typename T, precision P>
	GLM_FUNC_QUALIFIER vec<4, T, P> operator%(T scalar, vec<4, T, P> const & v)
	{
		return vec<4, T, P>(scalar) %= v;
	}

	template<typename T, precision P>
	GLM_FUNC_QUALIFIER vec<4, T, P> operator%(vec<1, T, P> const & scalar, vec<4, T, P> const & v)
	{
		return vec<4, T, P>(scalar.x) %= v;
	}

	template<typename T, precision P>
	GLM_FUNC_QUALIFIER vec<4, T, P> operator%(vec<4, T, P> const & v1, vec<4, T, P> const & v2)
	{
		return vec<4, T, P>(v1) %= v2;
	}

	template<typename T, precision P>
	GLM_FUNC_QUALIFIER vec<4, T, P> operator&(vec<4, T, P> const & v, T scalar)
	{
		return vec<4, T, P>(v) &= scalar;
	}

	template<typename T, precision P>
	GLM_FUNC_QUALIFIER vec<4, T, P> operator&(vec<4, T, P> const & v, vec<1, T, P> const & scalar)
	{
		return vec<4, T, P>(v) &= scalar;
	}

	template<typename T, precision P>
	GLM_FUNC_QUALIFIER vec<4, T, P> operator&(T scalar, vec<4, T, P> const & v)
	{
		return vec<4, T, P>(scalar) &= v;
	}

	template<typename T, precision P>
	GLM_FUNC_QUALIFIER vec<4, T, P> operator&(vec<1, T, P> const & v1, vec<4, T, P> const & v2)
	{
		return vec<4, T, P>(v1.x) &= v2;
	}

	template<typename T, precision P>
	GLM_FUNC_QUALIFIER vec<4, T, P> operator&(vec<4, T, P> const & v1, vec<4, T, P> const & v2)
	{
		return vec<4, T, P>(v1) &= v2;
	}

	template<typename T, precision P>
	GLM_FUNC_QUALIFIER vec<4, T, P> operator|(vec<4, T, P> const & v, T scalar)
	{
		return vec<4, T, P>(v) |= scalar;
	}

	template<typename T, precision P>
	GLM_FUNC_QUALIFIER vec<4, T, P> operator|(vec<4, T, P> const & v1, vec<1, T, P> const & v2)
	{
		return vec<4, T, P>(v1) |= v2.x;
	}

	template<typename T, precision P>
	GLM_FUNC_QUALIFIER vec<4, T, P> operator|(T scalar, vec<4, T, P> const & v)
	{
		return vec<4, T, P>(scalar) |= v;
	}

	template<typename T, precision P>
	GLM_FUNC_QUALIFIER vec<4, T, P> operator|(vec<1, T, P> const & v1, vec<4, T, P> const & v2)
	{
		return vec<4, T, P>(v1.x) |= v2;
	}

	template<typename T, precision P>
	GLM_FUNC_QUALIFIER vec<4, T, P> operator|(vec<4, T, P> const & v1, vec<4, T, P> const & v2)
	{
		return vec<4, T, P>(v1) |= v2;
	}

	template<typename T, precision P>
	GLM_FUNC_QUALIFIER vec<4, T, P> operator^(vec<4, T, P> const & v, T scalar)
	{
		return vec<4, T, P>(v) ^= scalar;
	}

	template<typename T, precision P>
	GLM_FUNC_QUALIFIER vec<4, T, P> operator^(vec<4, T, P> const & v1, vec<1, T, P> const & v2)
	{
		return vec<4, T, P>(v1) ^= v2.x;
	}

	template<typename T, precision P>
	GLM_FUNC_QUALIFIER vec<4, T, P> operator^(T scalar, vec<4, T, P> const & v)
	{
		return vec<4, T, P>(scalar) ^= v;
	}

	template<typename T, precision P>
	GLM_FUNC_QUALIFIER vec<4, T, P> operator^(vec<1, T, P> const & v1, vec<4, T, P> const & v2)
	{
		return vec<4, T, P>(v1.x) ^= v2;
	}

	template<typename T, precision P>
	GLM_FUNC_QUALIFIER vec<4, T, P> operator^(vec<4, T, P> const & v1, vec<4, T, P> const & v2)
	{
		return vec<4, T, P>(v1) ^= v2;
	}

	template<typename T, precision P>
	GLM_FUNC_QUALIFIER vec<4, T, P> operator<<(vec<4, T, P> const & v, T scalar)
	{
		return vec<4, T, P>(v) <<= scalar;
	}

	template<typename T, precision P>
	GLM_FUNC_QUALIFIER vec<4, T, P> operator<<(vec<4, T, P> const & v1, vec<1, T, P> const & v2)
	{
		return vec<4, T, P>(v1) <<= v2.x;
	}

	template<typename T, precision P>
	GLM_FUNC_QUALIFIER vec<4, T, P> operator<<(T scalar, vec<4, T, P> const & v)
	{
		return vec<4, T, P>(scalar) <<= v;
	}

	template<typename T, precision P>
	GLM_FUNC_QUALIFIER vec<4, T, P> operator<<(vec<1, T, P> const & v1, vec<4, T, P> const & v2)
	{
		return vec<4, T, P>(v1.x) <<= v2;
	}

	template<typename T, precision P>
	GLM_FUNC_QUALIFIER vec<4, T, P> operator<<(vec<4, T, P> const & v1, vec<4, T, P> const & v2)
	{
		return vec<4, T, P>(v1) <<= v2;
	}

	template<typename T, precision P>
	GLM_FUNC_QUALIFIER vec<4, T, P> operator>>(vec<4, T, P> const & v, T scalar)
	{
		return vec<4, T, P>(v) >>= scalar;
	}

	template<typename T, precision P>
	GLM_FUNC_QUALIFIER vec<4, T, P> operator>>(vec<4, T, P> const & v1, vec<1, T, P> const & v2)
	{
		return vec<4, T, P>(v1) >>= v2.x;
	}

	template<typename T, precision P>
	GLM_FUNC_QUALIFIER vec<4, T, P> operator>>(T scalar, vec<4, T, P> const & v)
	{
		return vec<4, T, P>(scalar) >>= v;
	}

	template<typename T, precision P>
	GLM_FUNC_QUALIFIER vec<4, T, P> operator>>(vec<1, T, P> const & v1, vec<4, T, P> const & v2)
	{
		return vec<4, T, P>(v1.x) >>= v2;
	}

	template<typename T, precision P>
	GLM_FUNC_QUALIFIER vec<4, T, P> operator>>(vec<4, T, P> const & v1, vec<4, T, P> const & v2)
	{
		return vec<4, T, P>(v1) >>= v2;
	}

	template<typename T, precision P>
	GLM_FUNC_QUALIFIER vec<4, T, P> operator~(vec<4, T, P> const & v)
	{
		return detail::compute_vec4_bitwise_not<T, P, detail::is_int<T>::value, sizeof(T) * 8, detail::is_aligned<P>::value>::call(v);
	}

	// -- Boolean operators --

	template<typename T, precision P>
	GLM_FUNC_QUALIFIER bool operator==(vec<4, T, P> const & v1, vec<4, T, P> const & v2)
	{
		return detail::compute_vec4_equal<T, P, detail::is_int<T>::value, sizeof(T) * 8, detail::is_aligned<P>::value>::call(v1, v2);
	}

	template<typename T, precision P>
	GLM_FUNC_QUALIFIER bool operator!=(vec<4, T, P> const & v1, vec<4, T, P> const & v2)
	{
		return detail::compute_vec4_nequal<T, P, detail::is_int<T>::value, sizeof(T) * 8, detail::is_aligned<P>::value>::call(v1, v2);
	}

	template<precision P>
	GLM_FUNC_QUALIFIER vec<4, bool, P> operator&&(vec<4, bool, P> const & v1, vec<4, bool, P> const & v2)
	{
		return vec<4, bool, P>(v1.x && v2.x, v1.y && v2.y, v1.z && v2.z, v1.w && v2.w);
	}

	template<precision P>
	GLM_FUNC_QUALIFIER vec<4, bool, P> operator||(vec<4, bool, P> const & v1, vec<4, bool, P> const & v2)
	{
		return vec<4, bool, P>(v1.x || v2.x, v1.y || v2.y, v1.z || v2.z, v1.w || v2.w);
	}
}//namespace glm

#if GLM_ARCH != GLM_ARCH_PURE && GLM_HAS_ALIGNED_TYPE
#	include "type_vec4_simd.inl"
#endif
