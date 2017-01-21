/// @ref gtx_hash
/// @file glm/gtx/hash.hpp
///
/// @see core (dependence)
///
/// @defgroup gtx_hash GLM_GTX_hash
/// @ingroup gtx
/// 
/// @brief Add std::hash support for glm types
/// 
/// <glm/gtx/hash.hpp> need to be included to use these functionalities.

#pragma once

#ifndef GLM_ENABLE_EXPERIMENTAL
#	error "GLM: GLM_GTX_hash is an experimental extension and may change in the future. Use #define GLM_ENABLE_EXPERIMENTAL before including it, if you really want to use it."
#endif

#include <functional>

#include "../vec2.hpp"
#include "../vec3.hpp"
#include "../vec4.hpp"
#include "../gtc/vec1.hpp"

#include "../gtc/quaternion.hpp"
#include "../gtx/dual_quaternion.hpp"

#include "../mat2x2.hpp"
#include "../mat2x3.hpp"
#include "../mat2x4.hpp"

#include "../mat3x2.hpp"
#include "../mat3x3.hpp"
#include "../mat3x4.hpp"

#include "../mat4x2.hpp"
#include "../mat4x3.hpp"
#include "../mat4x4.hpp"

#if !GLM_HAS_CXX11_STL
#	error "GLM_GTX_hash requires C++11 standard library support"
#endif

namespace std
{
	template<typename T, glm::precision P>
	struct hash<glm::vec<1, T,P> >
	{
		GLM_FUNC_DECL size_t operator()(glm::vec<1, T, P> const & v) const;
	};

	template<typename T, glm::precision P>
	struct hash<glm::vec<2, T,P> >
	{
		GLM_FUNC_DECL size_t operator()(glm::vec<2, T, P> const & v) const;
	};

	template<typename T, glm::precision P>
	struct hash<glm::vec<3, T,P> >
	{
		GLM_FUNC_DECL size_t operator()(glm::vec<3, T, P> const & v) const;
	};

	template<typename T, glm::precision P>
	struct hash<glm::vec<4, T,P> >
	{
		GLM_FUNC_DECL size_t operator()(glm::vec<4, T, P> const & v) const;
	};

	template<typename T, glm::precision P>
	struct hash<glm::tquat<T,P>>
	{
		GLM_FUNC_DECL size_t operator()(glm::tquat<T, P> const & q) const;
	};

	template<typename T, glm::precision P>
	struct hash<glm::tdualquat<T,P> >
	{
		GLM_FUNC_DECL size_t operator()(glm::tdualquat<T,P> const & q) const;
	};

	template<typename T, glm::precision P>
	struct hash<glm::mat<2, 2, T,P> >
	{
		GLM_FUNC_DECL size_t operator()(glm::mat<2, 2, T,P> const & m) const;
	};

	template<typename T, glm::precision P>
	struct hash<glm::mat<2, 3, T,P> >
	{
		GLM_FUNC_DECL size_t operator()(glm::mat<2, 3, T,P> const & m) const;
	};

	template<typename T, glm::precision P>
	struct hash<glm::mat<2, 4, T,P> >
	{
		GLM_FUNC_DECL size_t operator()(glm::mat<2, 4, T,P> const & m) const;
	};

	template<typename T, glm::precision P>
	struct hash<glm::mat<3, 2, T,P> >
	{
		GLM_FUNC_DECL size_t operator()(glm::mat<3, 2, T,P> const & m) const;
	};

	template<typename T, glm::precision P>
	struct hash<glm::mat<3, 3, T,P> >
	{
		GLM_FUNC_DECL size_t operator()(glm::mat<3, 3, T,P> const & m) const;
	};

	template<typename T, glm::precision P>
	struct hash<glm::mat<3, 4, T,P> >
	{
		GLM_FUNC_DECL size_t operator()(glm::mat<3, 4, T,P> const & m) const;
	};

	template<typename T, glm::precision P>
	struct hash<glm::mat<4, 2, T,P> >
	{
		GLM_FUNC_DECL size_t operator()(glm::mat<4, 2, T,P> const & m) const;
	};
	
	template<typename T, glm::precision P>
	struct hash<glm::mat<4, 3, T,P> >
	{
		GLM_FUNC_DECL size_t operator()(glm::mat<4, 3, T,P> const & m) const;
	};

	template<typename T, glm::precision P>
	struct hash<glm::mat<4, 4, T,P> >
	{
		GLM_FUNC_DECL size_t operator()(glm::mat<4, 4, T,P> const & m) const;
	};
} // namespace std

#include "hash.inl"
