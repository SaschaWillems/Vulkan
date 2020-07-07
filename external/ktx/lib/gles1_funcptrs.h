/* -*- tab-width: 4; -*- */
/* vi: set sw=2 ts=4 expandtab: */

/* $Id: a9dea3dfe7b8266c56bd790f15e03e8e7e1baa8e $ */

/*
 * ©2010-2017 The khronos Group, Inc.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

/*
 * Author: Mark Callow based on code from Georg Kolling
 */

#ifndef GLES1_FUNCPTRS_H
#define GLES1_FUNCPTRS_H

#ifdef __cplusplus
extern "C" {
#endif

/* remove these where already defined as typedefs (GCC 4 complains of duplicate definitions) */
typedef void (GL_APIENTRY* PFNGLTEXIMAGE1DPROC) (GLenum target, GLint level, GLint internalformat, GLsizei width, GLint border, GLenum format, GLenum type, const GLvoid *pixels);
typedef void (GL_APIENTRY* PFNGLTEXIMAGE3DPROC) (GLenum target, GLint level, GLint internalformat, GLsizei width, GLsizei height, GLsizei depth, GLint border, GLenum format, GLenum type, const GLvoid *pixels);
typedef void (GL_APIENTRY* PFNGLCOMPRESSEDTEXIMAGE1DPROC) (GLenum target, GLint level, GLenum internalformat, GLsizei width, GLint border, GLsizei imageSize, const GLvoid *data);
typedef void (GL_APIENTRY* PFNGLCOMPRESSEDTEXIMAGE3DPROC) (GLenum target, GLint level, GLenum internalformat, GLsizei width, GLsizei height, GLsizei depth, GLint border, GLsizei imageSize, const GLvoid *data);
typedef void (GL_APIENTRY* PFNGLGENERATEMIPMAPPROC) (GLenum target);
typedef const GLubyte* (GL_APIENTRY* PFNGLGETSTRINGIPROC) (GLenum name, GLint index);

extern PFNGLTEXIMAGE1DPROC pfGlTexImage1D;
extern PFNGLTEXIMAGE3DPROC pfGlTexImage3D;
extern PFNGLCOMPRESSEDTEXIMAGE1DPROC pfGlCompressedTexImage1D;
extern PFNGLCOMPRESSEDTEXIMAGE3DPROC pfGlCompressedTexImage3D;
extern PFNGLGENERATEMIPMAPPROC pfGlGenerateMipmap;
extern PFNGLGETSTRINGIPROC pfGlGetStringi;

#define DECLARE_GL_FUNCPTRS \
    PFNGLTEXIMAGE1DPROC pfGlTexImage1D; \
    PFNGLTEXIMAGE3DPROC pfGlTexImage3D; \
    PFNGLCOMPRESSEDTEXIMAGE1DPROC pfGlCompressedTexImage1D; \
    PFNGLCOMPRESSEDTEXIMAGE3DPROC pfGlCompressedTexImage3D; \
    PFNGLGENERATEMIPMAPPROC pfGlGenerateMipmap; \
    PFNGLGETSTRINGIPROC pfGlGetStringi;

#define INITIALIZE_GL_FUNCPTRS \
    pfGlTexImage1D = 0; \
    pfGlTexImage3D = 0; \
    pfGlCompressedTexImage1D = 0; \
    pfGlCompressedTexImage3D = 0; \
    pfGlGenerateMipmap = 0; \
    pfGlGetStringi = 0;

#ifdef __cplusplus
}
#endif

#endif /* GLES1_FUNCPTRS_H */
