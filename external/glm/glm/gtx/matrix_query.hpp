/// @ref gtx_matrix_query
/// @file glm/gtx/matrix_query.hpp
///
/// @see core (dependence)
/// @see gtx_vector_query (dependence)
///
/// @defgroup gtx_matrix_query GLM_GTX_matrix_query
/// @ingroup gtx
///
/// @brief Query to evaluate matrix properties
///
/// <glm/gtx/matrix_query.hpp> need to be included to use these functionalities.

#pragma once

// Dependency:
#include "../glm.hpp"
#include "../gtx/vector_query.hpp"
#include <limits>

#ifndef GLM_ENABLE_EXPERIMENTAL
#	error "GLM: GLM_GTX_matrix_query is an experimental extension and may change in the future. Use #define GLM_ENABLE_EXPERIMENTAL before including it, if you really want to use it."
#endif

#if GLM_MESSAGES == GLM_MESSAGES_ENABLED && !defined(GLM_EXT_INCLUDED)
#	pragma message("GLM: GLM_GTX_matrix_query extension included")
#endif

namespace glm
{
	/// @addtogroup gtx_matrix_query
	/// @{

	/// Return whether a matrix a null matrix.
	/// From GLM_GTX_matrix_query extension.
	template<typename T, precision P>
	GLM_FUNC_DECL bool isNull(mat<2, 2, T, P> const & m, T const & epsilon);
		
	/// Return whether a matrix a null matrix.
	/// From GLM_GTX_matrix_query extension.
	template<typename T, precision P>
	GLM_FUNC_DECL bool isNull(mat<3, 3, T, P> const & m, T const & epsilon);
		
	/// Return whether a matrix is a null matrix.
	/// From GLM_GTX_matrix_query extension.
	template<typename T, precision P>
	GLM_FUNC_DECL bool isNull(mat<4, 4, T, P> const & m, T const & epsilon);
			
	/// Return whether a matrix is an identity matrix.
	/// From GLM_GTX_matrix_query extension.
	template<length_t C, length_t R, typename T, precision P, template<length_t, length_t, typename, precision> class matType>
	GLM_FUNC_DECL bool isIdentity(matType<C, R, T, P> const & m, T const & epsilon);

	/// Return whether a matrix is a normalized matrix.
	/// From GLM_GTX_matrix_query extension.
	template<typename T, precision P>
	GLM_FUNC_DECL bool isNormalized(mat<2, 2, T, P> const & m, T const & epsilon);

	/// Return whether a matrix is a normalized matrix.
	/// From GLM_GTX_matrix_query extension.
	template<typename T, precision P>
	GLM_FUNC_DECL bool isNormalized(mat<3, 3, T, P> const & m, T const & epsilon);

	/// Return whether a matrix is a normalized matrix.
	/// From GLM_GTX_matrix_query extension.
	template<typename T, precision P>
	GLM_FUNC_DECL bool isNormalized(mat<4, 4, T, P> const & m, T const & epsilon);

	/// Return whether a matrix is an orthonormalized matrix.
	/// From GLM_GTX_matrix_query extension.
	template<length_t C, length_t R, typename T, precision P, template<length_t, length_t, typename, precision> class matType>
	GLM_FUNC_DECL bool isOrthogonal(matType<C, R, T, P> const & m, T const & epsilon);

	/// @}
}//namespace glm

#include "matrix_query.inl"
