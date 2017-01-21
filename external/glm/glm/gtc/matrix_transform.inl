/// @ref gtc_matrix_transform
/// @file glm/gtc/matrix_transform.inl

#include "../geometric.hpp"
#include "../trigonometric.hpp"
#include "../matrix.hpp"

namespace glm
{
	template<typename T, precision P>
	GLM_FUNC_QUALIFIER mat<4, 4, T, P> translate(mat<4, 4, T, P> const & m, vec<3, T, P> const & v)
	{
		mat<4, 4, T, P> Result(m);
		Result[3] = m[0] * v[0] + m[1] * v[1] + m[2] * v[2] + m[3];
		return Result;
	}
	
	template<typename T, precision P>
	GLM_FUNC_QUALIFIER mat<4, 4, T, P> rotate(mat<4, 4, T, P> const & m, T angle, vec<3, T, P> const & v)
	{
		T const a = angle;
		T const c = cos(a);
		T const s = sin(a);

		vec<3, T, P> axis(normalize(v));
		vec<3, T, P> temp((T(1) - c) * axis);

		mat<4, 4, T, P> Rotate(uninitialize);
		Rotate[0][0] = c + temp[0] * axis[0];
		Rotate[0][1] = temp[0] * axis[1] + s * axis[2];
		Rotate[0][2] = temp[0] * axis[2] - s * axis[1];

		Rotate[1][0] = temp[1] * axis[0] - s * axis[2];
		Rotate[1][1] = c + temp[1] * axis[1];
		Rotate[1][2] = temp[1] * axis[2] + s * axis[0];

		Rotate[2][0] = temp[2] * axis[0] + s * axis[1];
		Rotate[2][1] = temp[2] * axis[1] - s * axis[0];
		Rotate[2][2] = c + temp[2] * axis[2];

		mat<4, 4, T, P> Result(uninitialize);
		Result[0] = m[0] * Rotate[0][0] + m[1] * Rotate[0][1] + m[2] * Rotate[0][2];
		Result[1] = m[0] * Rotate[1][0] + m[1] * Rotate[1][1] + m[2] * Rotate[1][2];
		Result[2] = m[0] * Rotate[2][0] + m[1] * Rotate[2][1] + m[2] * Rotate[2][2];
		Result[3] = m[3];
		return Result;
	}
		
	template<typename T, precision P>
	GLM_FUNC_QUALIFIER mat<4, 4, T, P> rotate_slow(mat<4, 4, T, P> const & m, T angle, vec<3, T, P> const & v)
	{
		T const a = angle;
		T const c = cos(a);
		T const s = sin(a);
		mat<4, 4, T, P> Result;

		vec<3, T, P> axis = normalize(v);

		Result[0][0] = c + (static_cast<T>(1) - c)      * axis.x     * axis.x;
		Result[0][1] = (static_cast<T>(1) - c) * axis.x * axis.y + s * axis.z;
		Result[0][2] = (static_cast<T>(1) - c) * axis.x * axis.z - s * axis.y;
		Result[0][3] = static_cast<T>(0);

		Result[1][0] = (static_cast<T>(1) - c) * axis.y * axis.x - s * axis.z;
		Result[1][1] = c + (static_cast<T>(1) - c) * axis.y * axis.y;
		Result[1][2] = (static_cast<T>(1) - c) * axis.y * axis.z + s * axis.x;
		Result[1][3] = static_cast<T>(0);

		Result[2][0] = (static_cast<T>(1) - c) * axis.z * axis.x + s * axis.y;
		Result[2][1] = (static_cast<T>(1) - c) * axis.z * axis.y - s * axis.x;
		Result[2][2] = c + (static_cast<T>(1) - c) * axis.z * axis.z;
		Result[2][3] = static_cast<T>(0);

		Result[3] = vec<4, T, P>(0, 0, 0, 1);
		return m * Result;
	}

	template<typename T, precision P>
	GLM_FUNC_QUALIFIER mat<4, 4, T, P> scale(mat<4, 4, T, P> const & m, vec<3, T, P> const & v)
	{
		mat<4, 4, T, P> Result(uninitialize);
		Result[0] = m[0] * v[0];
		Result[1] = m[1] * v[1];
		Result[2] = m[2] * v[2];
		Result[3] = m[3];
		return Result;
	}

	template<typename T, precision P>
	GLM_FUNC_QUALIFIER mat<4, 4, T, P> scale_slow(mat<4, 4, T, P> const & m, vec<3, T, P> const & v)
	{
		mat<4, 4, T, P> Result(T(1));
		Result[0][0] = v.x;
		Result[1][1] = v.y;
		Result[2][2] = v.z;
		return m * Result;
	}

	template<typename T>
	GLM_FUNC_QUALIFIER mat<4, 4, T, defaultp> ortho
	(
		T left, T right,
		T bottom, T top,
		T zNear, T zFar
	)
	{
#		if GLM_COORDINATE_SYSTEM == GLM_LEFT_HANDED
			return orthoLH(left, right, bottom, top, zNear, zFar);
#		else
			return orthoRH(left, right, bottom, top, zNear, zFar);
#		endif
	}

	template<typename T>
	GLM_FUNC_QUALIFIER mat<4, 4, T, defaultp> orthoLH
	(
		T left, T right,
		T bottom, T top,
		T zNear, T zFar
	)
	{
		mat<4, 4, T, defaultp> Result(1);
		Result[0][0] = static_cast<T>(2) / (right - left);
		Result[1][1] = static_cast<T>(2) / (top - bottom);
		Result[3][0] = - (right + left) / (right - left);
		Result[3][1] = - (top + bottom) / (top - bottom);

#		if GLM_DEPTH_CLIP_SPACE == GLM_DEPTH_ZERO_TO_ONE
			Result[2][2] = static_cast<T>(1) / (zFar - zNear);
			Result[3][2] = - zNear / (zFar - zNear);
#		else
			Result[2][2] = static_cast<T>(2) / (zFar - zNear);
			Result[3][2] = - (zFar + zNear) / (zFar - zNear);
#		endif

		return Result;
	}

	template<typename T>
	GLM_FUNC_QUALIFIER mat<4, 4, T, defaultp> orthoRH
	(
		T left, T right,
		T bottom, T top,
		T zNear, T zFar
	)
	{
		mat<4, 4, T, defaultp> Result(1);
		Result[0][0] = static_cast<T>(2) / (right - left);
		Result[1][1] = static_cast<T>(2) / (top - bottom);
		Result[3][0] = - (right + left) / (right - left);
		Result[3][1] = - (top + bottom) / (top - bottom);

#		if GLM_DEPTH_CLIP_SPACE == GLM_DEPTH_ZERO_TO_ONE
			Result[2][2] = - static_cast<T>(1) / (zFar - zNear);
			Result[3][2] = - zNear / (zFar - zNear);
#		else
			Result[2][2] = - static_cast<T>(2) / (zFar - zNear);
			Result[3][2] = - (zFar + zNear) / (zFar - zNear);
#		endif

		return Result;
	}

	template<typename T>
	GLM_FUNC_QUALIFIER mat<4, 4, T, defaultp> ortho
	(
		T left, T right,
		T bottom, T top
	)
	{
		mat<4, 4, T, defaultp> Result(static_cast<T>(1));
		Result[0][0] = static_cast<T>(2) / (right - left);
		Result[1][1] = static_cast<T>(2) / (top - bottom);
		Result[2][2] = - static_cast<T>(1);
		Result[3][0] = - (right + left) / (right - left);
		Result[3][1] = - (top + bottom) / (top - bottom);
		return Result;
	}

	template<typename T>
	GLM_FUNC_QUALIFIER mat<4, 4, T, defaultp> frustum
	(
		T left, T right,
		T bottom, T top,
		T nearVal, T farVal
	)
	{
#		if GLM_COORDINATE_SYSTEM == GLM_LEFT_HANDED
			return frustumLH(left, right, bottom, top, nearVal, farVal);
#		else
			return frustumRH(left, right, bottom, top, nearVal, farVal);
#		endif
	}

	template<typename T>
	GLM_FUNC_QUALIFIER mat<4, 4, T, defaultp> frustumLH
	(
		T left, T right,
		T bottom, T top,
		T nearVal, T farVal
	)
	{
		mat<4, 4, T, defaultp> Result(0);
		Result[0][0] = (static_cast<T>(2) * nearVal) / (right - left);
		Result[1][1] = (static_cast<T>(2) * nearVal) / (top - bottom);
		Result[2][0] = (right + left) / (right - left);
		Result[2][1] = (top + bottom) / (top - bottom);
		Result[2][3] = static_cast<T>(1);

#		if GLM_DEPTH_CLIP_SPACE == GLM_DEPTH_ZERO_TO_ONE
			Result[2][2] = farVal / (farVal - nearVal);
			Result[3][2] = -(farVal * nearVal) / (farVal - nearVal);
#		else
			Result[2][2] = (farVal + nearVal) / (farVal - nearVal);
			Result[3][2] = - (static_cast<T>(2) * farVal * nearVal) / (farVal - nearVal);
#		endif

		return Result;
	}

	template<typename T>
	GLM_FUNC_QUALIFIER mat<4, 4, T, defaultp> frustumRH
	(
		T left, T right,
		T bottom, T top,
		T nearVal, T farVal
	)
	{
		mat<4, 4, T, defaultp> Result(0);
		Result[0][0] = (static_cast<T>(2) * nearVal) / (right - left);
		Result[1][1] = (static_cast<T>(2) * nearVal) / (top - bottom);
		Result[2][0] = (right + left) / (right - left);
		Result[2][1] = (top + bottom) / (top - bottom);
		Result[2][3] = static_cast<T>(-1);

#		if GLM_DEPTH_CLIP_SPACE == GLM_DEPTH_ZERO_TO_ONE
			Result[2][2] = farVal / (nearVal - farVal);
			Result[3][2] = -(farVal * nearVal) / (farVal - nearVal);
#		else
			Result[2][2] = - (farVal + nearVal) / (farVal - nearVal);
			Result[3][2] = - (static_cast<T>(2) * farVal * nearVal) / (farVal - nearVal);
#		endif

		return Result;
	}

	template<typename T>
	GLM_FUNC_QUALIFIER mat<4, 4, T, defaultp> perspective(T fovy, T aspect, T zNear, T zFar)
	{
#		if GLM_COORDINATE_SYSTEM == GLM_LEFT_HANDED
			return perspectiveLH(fovy, aspect, zNear, zFar);
#		else
			return perspectiveRH(fovy, aspect, zNear, zFar);
#		endif
	}

	template<typename T>
	GLM_FUNC_QUALIFIER mat<4, 4, T, defaultp> perspectiveRH(T fovy, T aspect, T zNear, T zFar)
	{
		assert(abs(aspect - std::numeric_limits<T>::epsilon()) > static_cast<T>(0));

		T const tanHalfFovy = tan(fovy / static_cast<T>(2));

		mat<4, 4, T, defaultp> Result(static_cast<T>(0));
		Result[0][0] = static_cast<T>(1) / (aspect * tanHalfFovy);
		Result[1][1] = static_cast<T>(1) / (tanHalfFovy);
		Result[2][3] = - static_cast<T>(1);

#		if GLM_DEPTH_CLIP_SPACE == GLM_DEPTH_ZERO_TO_ONE
			Result[2][2] = zFar / (zNear - zFar);
			Result[3][2] = -(zFar * zNear) / (zFar - zNear);
#		else
			Result[2][2] = - (zFar + zNear) / (zFar - zNear);
			Result[3][2] = - (static_cast<T>(2) * zFar * zNear) / (zFar - zNear);
#		endif

		return Result;
	}
	
	template<typename T>
	GLM_FUNC_QUALIFIER mat<4, 4, T, defaultp> perspectiveLH(T fovy, T aspect, T zNear, T zFar)
	{
		assert(abs(aspect - std::numeric_limits<T>::epsilon()) > static_cast<T>(0));

		T const tanHalfFovy = tan(fovy / static_cast<T>(2));
		
		mat<4, 4, T, defaultp> Result(static_cast<T>(0));
		Result[0][0] = static_cast<T>(1) / (aspect * tanHalfFovy);
		Result[1][1] = static_cast<T>(1) / (tanHalfFovy);
		Result[2][3] = static_cast<T>(1);

#		if GLM_DEPTH_CLIP_SPACE == GLM_DEPTH_ZERO_TO_ONE
			Result[2][2] = zFar / (zFar - zNear);
			Result[3][2] = -(zFar * zNear) / (zFar - zNear);
#		else
			Result[2][2] = (zFar + zNear) / (zFar - zNear);
			Result[3][2] = - (static_cast<T>(2) * zFar * zNear) / (zFar - zNear);
#		endif

		return Result;
	}

	template<typename T>
	GLM_FUNC_QUALIFIER mat<4, 4, T, defaultp> perspectiveFov(T fov, T width, T height, T zNear, T zFar)
	{
#		if GLM_COORDINATE_SYSTEM == GLM_LEFT_HANDED
			return perspectiveFovLH(fov, width, height, zNear, zFar);
#		else
			return perspectiveFovRH(fov, width, height, zNear, zFar);
#		endif
	}

	template<typename T>
	GLM_FUNC_QUALIFIER mat<4, 4, T, defaultp> perspectiveFovRH(T fov, T width, T height, T zNear, T zFar)
	{
		assert(width > static_cast<T>(0));
		assert(height > static_cast<T>(0));
		assert(fov > static_cast<T>(0));
	
		T const rad = fov;
		T const h = glm::cos(static_cast<T>(0.5) * rad) / glm::sin(static_cast<T>(0.5) * rad);
		T const w = h * height / width; ///todo max(width , Height) / min(width , Height)?

		mat<4, 4, T, defaultp> Result(static_cast<T>(0));
		Result[0][0] = w;
		Result[1][1] = h;
		Result[2][3] = - static_cast<T>(1);

#		if GLM_DEPTH_CLIP_SPACE == GLM_DEPTH_ZERO_TO_ONE
			Result[2][2] = zFar / (zNear - zFar);
			Result[3][2] = -(zFar * zNear) / (zFar - zNear);
#		else
			Result[2][2] = - (zFar + zNear) / (zFar - zNear);
			Result[3][2] = - (static_cast<T>(2) * zFar * zNear) / (zFar - zNear);
#		endif

		return Result;
	}

	template<typename T>
	GLM_FUNC_QUALIFIER mat<4, 4, T, defaultp> perspectiveFovLH(T fov, T width, T height, T zNear, T zFar)
	{
		assert(width > static_cast<T>(0));
		assert(height > static_cast<T>(0));
		assert(fov > static_cast<T>(0));
	
		T const rad = fov;
		T const h = glm::cos(static_cast<T>(0.5) * rad) / glm::sin(static_cast<T>(0.5) * rad);
		T const w = h * height / width; ///todo max(width , Height) / min(width , Height)?

		mat<4, 4, T, defaultp> Result(static_cast<T>(0));
		Result[0][0] = w;
		Result[1][1] = h;
		Result[2][3] = static_cast<T>(1);

#		if GLM_DEPTH_CLIP_SPACE == GLM_DEPTH_ZERO_TO_ONE
			Result[2][2] = zFar / (zFar - zNear);
			Result[3][2] = -(zFar * zNear) / (zFar - zNear);
#		else
			Result[2][2] = (zFar + zNear) / (zFar - zNear);
			Result[3][2] = - (static_cast<T>(2) * zFar * zNear) / (zFar - zNear);
#		endif

		return Result;
	}

	template<typename T>
	GLM_FUNC_QUALIFIER mat<4, 4, T, defaultp> infinitePerspective(T fovy, T aspect, T zNear)
	{
#		if GLM_COORDINATE_SYSTEM == GLM_LEFT_HANDED
			return infinitePerspectiveLH(fovy, aspect, zNear);
#		else
			return infinitePerspectiveRH(fovy, aspect, zNear);
#		endif
	}

	template<typename T>
	GLM_FUNC_QUALIFIER mat<4, 4, T, defaultp> infinitePerspectiveRH(T fovy, T aspect, T zNear)
	{
		T const range = tan(fovy / static_cast<T>(2)) * zNear;
		T const left = -range * aspect;
		T const right = range * aspect;
		T const bottom = -range;
		T const top = range;

		mat<4, 4, T, defaultp> Result(static_cast<T>(0));
		Result[0][0] = (static_cast<T>(2) * zNear) / (right - left);
		Result[1][1] = (static_cast<T>(2) * zNear) / (top - bottom);
		Result[2][2] = - static_cast<T>(1);
		Result[2][3] = - static_cast<T>(1);
		Result[3][2] = - static_cast<T>(2) * zNear;
		return Result;
	}

	template<typename T>
	GLM_FUNC_QUALIFIER mat<4, 4, T, defaultp> infinitePerspectiveLH(T fovy, T aspect, T zNear)
	{
		T const range = tan(fovy / static_cast<T>(2)) * zNear;
		T const left = -range * aspect;
		T const right = range * aspect;
		T const bottom = -range;
		T const top = range;

		mat<4, 4, T, defaultp> Result(T(0));
		Result[0][0] = (static_cast<T>(2) * zNear) / (right - left);
		Result[1][1] = (static_cast<T>(2) * zNear) / (top - bottom);
		Result[2][2] = static_cast<T>(1);
		Result[2][3] = static_cast<T>(1);
		Result[3][2] = - static_cast<T>(2) * zNear;
		return Result;
	}

	// Infinite projection matrix: http://www.terathon.com/gdc07_lengyel.pdf
	template<typename T>
	GLM_FUNC_QUALIFIER mat<4, 4, T, defaultp> tweakedInfinitePerspective(T fovy, T aspect, T zNear, T ep)
	{
		T const range = tan(fovy / static_cast<T>(2)) * zNear;	
		T const left = -range * aspect;
		T const right = range * aspect;
		T const bottom = -range;
		T const top = range;

		mat<4, 4, T, defaultp> Result(static_cast<T>(0));
		Result[0][0] = (static_cast<T>(2) * zNear) / (right - left);
		Result[1][1] = (static_cast<T>(2) * zNear) / (top - bottom);
		Result[2][2] = ep - static_cast<T>(1);
		Result[2][3] = static_cast<T>(-1);
		Result[3][2] = (ep - static_cast<T>(2)) * zNear;
		return Result;
	}

	template<typename T>
	GLM_FUNC_QUALIFIER mat<4, 4, T, defaultp> tweakedInfinitePerspective(T fovy, T aspect, T zNear)
	{
		return tweakedInfinitePerspective(fovy, aspect, zNear, epsilon<T>());
	}

	template<typename T, typename U, precision P>
	GLM_FUNC_QUALIFIER vec<3, T, P> project
	(
		vec<3, T, P> const & obj,
		mat<4, 4, T, P> const& model,
		mat<4, 4, T, P> const& proj,
		vec<4, U, P> const & viewport
	)
	{
		vec<4, T, P> tmp = vec<4, T, P>(obj, static_cast<T>(1));
		tmp = model * tmp;
		tmp = proj * tmp;

		tmp /= tmp.w;
#		if GLM_DEPTH_CLIP_SPACE == GLM_DEPTH_ZERO_TO_ONE
			tmp.x = tmp.x * static_cast<T>(0.5) + static_cast<T>(0.5);
			tmp.y = tmp.y * static_cast<T>(0.5) + static_cast<T>(0.5);
#		else
			tmp = tmp * static_cast<T>(0.5) + static_cast<T>(0.5);
#		endif
		tmp[0] = tmp[0] * T(viewport[2]) + T(viewport[0]);
		tmp[1] = tmp[1] * T(viewport[3]) + T(viewport[1]);

		return vec<3, T, P>(tmp);
	}

	template<typename T, typename U, precision P>
	GLM_FUNC_QUALIFIER vec<3, T, P> unProject
	(
		vec<3, T, P> const & win,
		mat<4, 4, T, P> const& model,
		mat<4, 4, T, P> const& proj,
		vec<4, U, P> const & viewport
	)
	{
		mat<4, 4, T, P> Inverse = inverse(proj * model);

		vec<4, T, P> tmp = vec<4, T, P>(win, T(1));
		tmp.x = (tmp.x - T(viewport[0])) / T(viewport[2]);
		tmp.y = (tmp.y - T(viewport[1])) / T(viewport[3]);
#		if GLM_DEPTH_CLIP_SPACE == GLM_DEPTH_ZERO_TO_ONE
			tmp.x = tmp.x * static_cast<T>(2) - static_cast<T>(1);
			tmp.y = tmp.y * static_cast<T>(2) - static_cast<T>(1);
#		else
			tmp = tmp * static_cast<T>(2) - static_cast<T>(1);
#		endif

		vec<4, T, P> obj = Inverse * tmp;
		obj /= obj.w;

		return vec<3, T, P>(obj);
	}

	template<typename T, precision P, typename U>
	GLM_FUNC_QUALIFIER mat<4, 4, T, P> pickMatrix(vec<2, T, P> const & center, vec<2, T, P> const & delta, vec<4, U, P> const & viewport)
	{
		assert(delta.x > static_cast<T>(0) && delta.y > static_cast<T>(0));
		mat<4, 4, T, P> Result(static_cast<T>(1));

		if(!(delta.x > static_cast<T>(0) && delta.y > static_cast<T>(0)))
			return Result; // Error

		vec<3, T, P> Temp(
			(static_cast<T>(viewport[2]) - static_cast<T>(2) * (center.x - static_cast<T>(viewport[0]))) / delta.x,
			(static_cast<T>(viewport[3]) - static_cast<T>(2) * (center.y - static_cast<T>(viewport[1]))) / delta.y,
			static_cast<T>(0));

		// Translate and scale the picked region to the entire window
		Result = translate(Result, Temp);
		return scale(Result, vec<3, T, P>(static_cast<T>(viewport[2]) / delta.x, static_cast<T>(viewport[3]) / delta.y, static_cast<T>(1)));
	}

	template<typename T, precision P>
	GLM_FUNC_QUALIFIER mat<4, 4, T, P> lookAt(vec<3, T, P> const & eye, vec<3, T, P> const & center, vec<3, T, P> const & up)
	{
#		if GLM_COORDINATE_SYSTEM == GLM_LEFT_HANDED
			return lookAtLH(eye, center, up);
#		else
			return lookAtRH(eye, center, up);
#		endif
	}

	template<typename T, precision P>
	GLM_FUNC_QUALIFIER mat<4, 4, T, P> lookAtRH
	(
		vec<3, T, P> const & eye,
		vec<3, T, P> const & center,
		vec<3, T, P> const & up
	)
	{
		vec<3, T, P> const f(normalize(center - eye));
		vec<3, T, P> const s(normalize(cross(f, up)));
		vec<3, T, P> const u(cross(s, f));

		mat<4, 4, T, P> Result(1);
		Result[0][0] = s.x;
		Result[1][0] = s.y;
		Result[2][0] = s.z;
		Result[0][1] = u.x;
		Result[1][1] = u.y;
		Result[2][1] = u.z;
		Result[0][2] =-f.x;
		Result[1][2] =-f.y;
		Result[2][2] =-f.z;
		Result[3][0] =-dot(s, eye);
		Result[3][1] =-dot(u, eye);
		Result[3][2] = dot(f, eye);
		return Result;
	}

	template<typename T, precision P>
	GLM_FUNC_QUALIFIER mat<4, 4, T, P> lookAtLH
	(
		vec<3, T, P> const & eye,
		vec<3, T, P> const & center,
		vec<3, T, P> const & up
	)
	{
		vec<3, T, P> const f(normalize(center - eye));
		vec<3, T, P> const s(normalize(cross(up, f)));
		vec<3, T, P> const u(cross(f, s));

		mat<4, 4, T, P> Result(1);
		Result[0][0] = s.x;
		Result[1][0] = s.y;
		Result[2][0] = s.z;
		Result[0][1] = u.x;
		Result[1][1] = u.y;
		Result[2][1] = u.z;
		Result[0][2] = f.x;
		Result[1][2] = f.y;
		Result[2][2] = f.z;
		Result[3][0] = -dot(s, eye);
		Result[3][1] = -dot(u, eye);
		Result[3][2] = -dot(f, eye);
		return Result;
	}
}//namespace glm
