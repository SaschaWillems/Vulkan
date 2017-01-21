/// @ref gtc_type_ptr
/// @file glm/gtc/type_ptr.inl

#include <cstring>

namespace glm
{
	/// @addtogroup gtc_type_ptr
	/// @{

	/// Return the constant address to the data of the vector input.
	/// @see gtc_type_ptr
	template<typename T, precision P>
	GLM_FUNC_QUALIFIER T const* value_ptr(vec<2, T, P> const& v)
	{
		return &(v.x);
	}

	//! Return the address to the data of the vector input.
	/// @see gtc_type_ptr
	template<typename T, precision P>
	GLM_FUNC_QUALIFIER T* value_ptr(vec<2, T, P>& v)
	{
		return &(v.x);
	}

	/// Return the constant address to the data of the vector input.
	/// @see gtc_type_ptr
	template<typename T, precision P>
	GLM_FUNC_QUALIFIER T const * value_ptr(vec<3, T, P> const& v)
	{
		return &(v.x);
	}

	//! Return the address to the data of the vector input.
	/// @see gtc_type_ptr
	template<typename T, precision P>
	GLM_FUNC_QUALIFIER T* value_ptr(vec<3, T, P>& v)
	{
		return &(v.x);
	}
		
	/// Return the constant address to the data of the vector input.
	/// @see gtc_type_ptr
	template<typename T, precision P>
	GLM_FUNC_QUALIFIER T const* value_ptr(vec<4, T, P> const& v)
	{
		return &(v.x);
	}

	//! Return the address to the data of the vector input.
	//! From GLM_GTC_type_ptr extension.
	template<typename T, precision P>
	GLM_FUNC_QUALIFIER T* value_ptr(vec<4, T, P>& v)
	{
		return &(v.x);
	}

	/// Return the constant address to the data of the matrix input.
	/// @see gtc_type_ptr
	template<typename T, precision P>
	GLM_FUNC_QUALIFIER T const* value_ptr(mat<2, 2, T, P> const& m)
	{
		return &(m[0].x);
	}

	//! Return the address to the data of the matrix input.
	/// @see gtc_type_ptr
	template<typename T, precision P>
	GLM_FUNC_QUALIFIER T* value_ptr(mat<2, 2, T, P>& m)
	{
		return &(m[0].x);
	}

	/// Return the constant address to the data of the matrix input.
	/// @see gtc_type_ptr
	template<typename T, precision P>
	GLM_FUNC_QUALIFIER T const* value_ptr(mat<3, 3, T, P> const& m)
	{
		return &(m[0].x);
	}

	//! Return the address to the data of the matrix input.
	/// @see gtc_type_ptr
	template<typename T, precision P>
	GLM_FUNC_QUALIFIER T* value_ptr(mat<3, 3, T, P>& m)
	{
		return &(m[0].x);
	}
		
	/// Return the constant address to the data of the matrix input.
	/// @see gtc_type_ptr
	template<typename T, precision P>
	GLM_FUNC_QUALIFIER T const* value_ptr(mat<4, 4, T, P> const& m)
	{
		return &(m[0].x);
	}

	//! Return the address to the data of the matrix input.
	//! From GLM_GTC_type_ptr extension.
	template<typename T, precision P>
	GLM_FUNC_QUALIFIER T* value_ptr(mat<4, 4, T, P>& m)
	{
		return &(m[0].x);
	}

	/// Return the constant address to the data of the matrix input.
	/// @see gtc_type_ptr
	template<typename T, precision P>
	GLM_FUNC_QUALIFIER T const* value_ptr(mat<2, 3, T, P> const& m)
	{
		return &(m[0].x);
	}

	//! Return the address to the data of the matrix input.
	/// @see gtc_type_ptr
	template<typename T, precision P>
	GLM_FUNC_QUALIFIER T* value_ptr(mat<2, 3, T, P>& m)
	{
		return &(m[0].x);
	}
		
	/// Return the constant address to the data of the matrix input.
	/// @see gtc_type_ptr
	template<typename T, precision P>
	GLM_FUNC_QUALIFIER T const* value_ptr(mat<3, 2, T, P> const& m)
	{
		return &(m[0].x);
	}

	//! Return the address to the data of the matrix input.
	/// @see gtc_type_ptr
	template<typename T, precision P>
	GLM_FUNC_QUALIFIER T* value_ptr(mat<3, 2, T, P>& m)
	{
		return &(m[0].x);
	}
		
	/// Return the constant address to the data of the matrix input.
	/// @see gtc_type_ptr
	template<typename T, precision P>
	GLM_FUNC_QUALIFIER T const* value_ptr(mat<2, 4, T, P> const& m)
	{
		return &(m[0].x);
	}

	//! Return the address to the data of the matrix input.
	/// @see gtc_type_ptr
	template<typename T, precision P>
	GLM_FUNC_QUALIFIER T* value_ptr(mat<2, 4, T, P>& m)
	{
		return &(m[0].x);
	}
		
	/// Return the constant address to the data of the matrix input.
	/// @see gtc_type_ptr
	template<typename T, precision P>
	GLM_FUNC_QUALIFIER T const* value_ptr(mat<4, 2, T, P> const& m)
	{
		return &(m[0].x);
	}

	//! Return the address to the data of the matrix input.
	/// @see gtc_type_ptr
	template<typename T, precision P>
	GLM_FUNC_QUALIFIER T* value_ptr(mat<4, 2, T, P>& m)
	{
		return &(m[0].x);
	}
		
	/// Return the constant address to the data of the matrix input.
	/// @see gtc_type_ptr
	template<typename T, precision P>
	GLM_FUNC_QUALIFIER T const* value_ptr(mat<3, 4, T, P> const& m)
	{
		return &(m[0].x);
	}

	//! Return the address to the data of the matrix input.
	/// @see gtc_type_ptr
	template<typename T, precision P>
	GLM_FUNC_QUALIFIER T* value_ptr(mat<3, 4, T, P>& m)
	{
		return &(m[0].x);
	}
		
	/// Return the constant address to the data of the matrix input.
	/// @see gtc_type_ptr
	template<typename T, precision P>
	GLM_FUNC_QUALIFIER T const* value_ptr(mat<4, 3, T, P> const& m)
	{
		return &(m[0].x);
	}

	/// Return the address to the data of the matrix input.
	/// @see gtc_type_ptr
	template<typename T, precision P>
	GLM_FUNC_QUALIFIER T * value_ptr(mat<4, 3, T, P>& m)
	{
		return &(m[0].x);
	}

	/// Return the constant address to the data of the input parameter.
	/// @see gtc_type_ptr
	template<typename T, precision P>
	GLM_FUNC_QUALIFIER T const * value_ptr(tquat<T, P> const& q)
	{
		return &(q[0]);
	}

	/// Return the address to the data of the quaternion input.
	/// @see gtc_type_ptr
	template<typename T, precision P>
	GLM_FUNC_QUALIFIER T* value_ptr(tquat<T, P>& q)
	{
		return &(q[0]);
	}

	/// Build a vector from a pointer.
	/// @see gtc_type_ptr
	template<typename T>
	GLM_FUNC_QUALIFIER vec<2, T, defaultp> make_vec2(T const *const ptr)
	{
		vec<2, T, defaultp> Result;
		memcpy(value_ptr(Result), ptr, sizeof(vec<2, T, defaultp>));
		return Result;
	}

	/// Build a vector from a pointer.
	/// @see gtc_type_ptr
	template<typename T>
	GLM_FUNC_QUALIFIER vec<3, T, defaultp> make_vec3(T const *const ptr)
	{
		vec<3, T, defaultp> Result;
		memcpy(value_ptr(Result), ptr, sizeof(vec<3, T, defaultp>));
		return Result;
	}

	/// Build a vector from a pointer.
	/// @see gtc_type_ptr
	template<typename T>
	GLM_FUNC_QUALIFIER vec<4, T, defaultp> make_vec4(T const *const ptr)
	{
		vec<4, T, defaultp> Result;
		memcpy(value_ptr(Result), ptr, sizeof(vec<4, T, defaultp>));
		return Result;
	}

	/// Build a matrix from a pointer.
	/// @see gtc_type_ptr
	template<typename T>
	GLM_FUNC_QUALIFIER mat<2, 2, T, defaultp> make_mat2x2(T const *const ptr)
	{
		mat<2, 2, T, defaultp> Result;
		memcpy(value_ptr(Result), ptr, sizeof(mat<2, 2, T, defaultp>));
		return Result;
	}

	/// Build a matrix from a pointer.
	/// @see gtc_type_ptr
	template<typename T>
	GLM_FUNC_QUALIFIER mat<2, 3, T, defaultp> make_mat2x3(T const *const ptr)
	{
		mat<2, 3, T, defaultp> Result;
		memcpy(value_ptr(Result), ptr, sizeof(mat<2, 3, T, defaultp>));
		return Result;
	}

	/// Build a matrix from a pointer.
	/// @see gtc_type_ptr
	template<typename T>
	GLM_FUNC_QUALIFIER mat<2, 4, T, defaultp> make_mat2x4(T const *const ptr)
	{
		mat<2, 4, T, defaultp> Result;
		memcpy(value_ptr(Result), ptr, sizeof(mat<2, 4, T, defaultp>));
		return Result;
	}

	/// Build a matrix from a pointer.
	/// @see gtc_type_ptr
	template<typename T>
	GLM_FUNC_QUALIFIER mat<3, 2, T, defaultp> make_mat3x2(T const *const ptr)
	{
		mat<3, 2, T, defaultp> Result;
		memcpy(value_ptr(Result), ptr, sizeof(mat<3, 2, T, defaultp>));
		return Result;
	}

	//! Build a matrix from a pointer.
	/// @see gtc_type_ptr
	template<typename T>
	GLM_FUNC_QUALIFIER mat<3, 3, T, defaultp> make_mat3x3(T const *const ptr)
	{
		mat<3, 3, T, defaultp> Result;
		memcpy(value_ptr(Result), ptr, sizeof(mat<3, 3, T, defaultp>));
		return Result;
	}

	//! Build a matrix from a pointer.
	/// @see gtc_type_ptr
	template<typename T>
	GLM_FUNC_QUALIFIER mat<3, 4, T, defaultp> make_mat3x4(T const *const ptr)
	{
		mat<3, 4, T, defaultp> Result;
		memcpy(value_ptr(Result), ptr, sizeof(mat<3, 4, T, defaultp>));
		return Result;
	}

	//! Build a matrix from a pointer.
	/// @see gtc_type_ptr
	template<typename T>
	GLM_FUNC_QUALIFIER mat<4, 2, T, defaultp> make_mat4x2(T const *const ptr)
	{
		mat<4, 2, T, defaultp> Result;
		memcpy(value_ptr(Result), ptr, sizeof(mat<4, 2, T, defaultp>));
		return Result;
	}

	//! Build a matrix from a pointer.
	/// @see gtc_type_ptr
	template<typename T>
	GLM_FUNC_QUALIFIER mat<4, 3, T, defaultp> make_mat4x3(T const *const ptr)
	{
		mat<4, 3, T, defaultp> Result;
		memcpy(value_ptr(Result), ptr, sizeof(mat<4, 3, T, defaultp>));
		return Result;
	}

	//! Build a matrix from a pointer.
	/// @see gtc_type_ptr
	template<typename T>
	GLM_FUNC_QUALIFIER mat<4, 4, T, defaultp> make_mat4x4(T const *const ptr)
	{
		mat<4, 4, T, defaultp> Result;
		memcpy(value_ptr(Result), ptr, sizeof(mat<4, 4, T, defaultp>));
		return Result;
	}

	//! Build a matrix from a pointer.
	/// @see gtc_type_ptr
	template<typename T>
	GLM_FUNC_QUALIFIER mat<2, 2, T, defaultp> make_mat2(T const *const ptr)
	{
		return make_mat2x2(ptr);
	}

	//! Build a matrix from a pointer.
	/// @see gtc_type_ptr
	template<typename T>
	GLM_FUNC_QUALIFIER mat<3, 3, T, defaultp> make_mat3(T const *const ptr)
	{
		return make_mat3x3(ptr);
	}
		
	//! Build a matrix from a pointer.
	/// @see gtc_type_ptr
	template<typename T>
	GLM_FUNC_QUALIFIER mat<4, 4, T, defaultp> make_mat4(T const *const ptr)
	{
		return make_mat4x4(ptr);
	}

	//! Build a quaternion from a pointer.
	/// @see gtc_type_ptr
	template<typename T>
	GLM_FUNC_QUALIFIER tquat<T, defaultp> make_quat(T const *const ptr)
	{
		tquat<T, defaultp> Result;
		memcpy(value_ptr(Result), ptr, sizeof(tquat<T, defaultp>));
		return Result;
	}

	/// @}
}//namespace glm

