/* -*- tab-width: 4; -*- */
/* vi: set sw=2 ts=4 expandtab: */

/*
 * ©2010-2018 Mark Callow.
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

/**
 * @internal
 * @file
 * @~English
 *
 * @brief Functions for instantiating GL or GLES textures from KTX files.
 *
 * @author Georg Kolling, Imagination Technology
 * @author Mark Callow, HI Corporation & Edgewise Consulting
 */

#ifdef _WIN32
#define _CRT_SECURE_NO_WARNINGS
#endif

#include <assert.h>
#include <string.h>
#include <stdlib.h>

#if KTX_OPENGL

  #ifdef _WIN32
    #include <windows.h>
    #undef KTX_USE_GETPROC  /* Must use GETPROC on Windows */
    #define KTX_USE_GETPROC 1
  #else
    #if !defined(KTX_USE_GETPROC)
      #define KTX_USE_GETPROC 0
    #endif
  #endif
  #if KTX_USE_GETPROC
    #include <GL/glew.h>
  #else
    #define GL_GLEXT_PROTOTYPES
    #include <GL/glcorearb.h>
  #endif

  #define GL_APIENTRY APIENTRY
  #include "gl_funcptrs.h"

#elif KTX_OPENGL_ES1

  #include <GLES/gl.h>
  #include <GLES/glext.h>
  #include "gles1_funcptrs.h"

#elif KTX_OPENGL_ES2

  #define GL_GLEXT_PROTOTYPES
  #include <GLES2/gl2.h>
  #include <GLES2/gl2ext.h>
  #include "gles2_funcptrs.h"

#elif KTX_OPENGL_ES3

  #define GL_GLEXT_PROTOTYPES
  #include <GLES3/gl3.h>
  #include <GLES2/gl2ext.h>
  #include "gles3_funcptrs.h"

#else
  #error Please #define one of KTX_OPENGL, KTX_OPENGL_ES1, KTX_OPENGL_ES2 or KTX_OPENGL_ES3 as 1
#endif

#include "ktx.h"
#include "ktxint.h"
#include "ktxgl.h"

DECLARE_GL_FUNCPTRS

/**
 * @defgroup ktx_glloader OpenGL Texture Image Loader
 * @brief Create texture objects in current OpenGL context.
 * @{
 */

/**
 * @example glloader.c
 * This is an example of using the low-level ktxTexture API to create and load
 * an OpenGL texture. It is a fragment of the code used by
 * @ref ktxTexture_GLUpload which underpins the @c ktxLoadTexture* functions.
 *
 * @code
 * #include <ktx.h>
 * @endcode
 *
 * This structure is used to pass to a callback function data that is uniform
 * across all images.
 * @snippet this cbdata
 *
 * One of these callbacks, selected by @ref ktxTexture_GLUpload based on the
 * dimensionality and arrayness of the texture, is called from
 * @ref ktxTexture_IterateLevelFaces to upload the texture data to OpenGL.
 * @snippet this imageCallbacks
 *
 * This function creates the GL texture object and sets up the callbacks to
 * load the image data into it.
 * @snippet this loadGLTexture
 */

/**
 * @internal
 * @~English
 * @brief Additional contextProfile bit indicating an OpenGL ES context.
 *
 * This is the same value NVIDIA returns when using an OpenGL ES profile
 * of their desktop drivers. However it is not specified in any official
 * specification as OpenGL ES does not support the GL_CONTEXT_PROFILE_MASK
 * query.
 */
#define _CONTEXT_ES_PROFILE_BIT 0x4

/**
 * @internal
 * @~English
 * @name Supported Sized Format Macros
 *
 * These macros describe values that may be used with the sizedFormats
 * variable.
 */
/**@{*/
#define _NON_LEGACY_FORMATS 0x1 /*< @internal Non-legacy sized formats are supported. */
#define _LEGACY_FORMATS 0x2  /*< @internal Legacy sized formats are supported. */
/**
 * @internal
 * @~English
 * @brief All sized formats are supported
 */
#define _ALL_SIZED_FORMATS (_NON_LEGACY_FORMATS | _LEGACY_FORMATS)
#define _NO_SIZED_FORMATS 0 /*< @internal No sized formats are supported. */
/**@}*/

/**
 * @internal
 * @~English
 * @brief indicates the profile of the current context.
 */
static GLint contextProfile = 0;
/**
 * @internal
 * @~English
 * @brief Indicates what sized texture formats are supported
 *        by the current context.
 */
static GLint sizedFormats = _ALL_SIZED_FORMATS;
static GLboolean supportsSwizzle = GL_TRUE;
/**
 * @internal
 * @~English
 * @brief Indicates which R16 & RG16 formats are supported by the current
 *        context.
 */
static GLint R16Formats = _KTX_ALL_R16_FORMATS;
/**
 * @internal
 * @~English
 * @brief Indicates if the current context supports sRGB textures.
 */
static GLboolean supportsSRGB = GL_TRUE;
/**
 * @internal
 * @~English
 * @brief Indicates if the current context supports cube map arrays.
 */
static GLboolean supportsCubeMapArrays = GL_FALSE;

/**
 * @internal
 * @~English
 * @brief Workaround mismatch of glGetString declaration and standard string
 *        function parameters.
 */
#define glGetString(x) (const char*)glGetString(x)

/**
 * @internal
 * @~English
 * @brief Workaround mismatch of glGetStringi declaration and standard string
 *        function parameters.
 */
#define pfGlGetStringi(x,y) (const char*)pfGlGetStringi(x,y)

/**
 * @internal
 * @~English
 * @brief Check for existence of OpenGL extension
 */
static GLboolean
hasExtension(const char* extension) 
{
    if (pfGlGetStringi == NULL) {
        if (strstr(glGetString(GL_EXTENSIONS), extension) != NULL)
            return GL_TRUE;
        else
            return GL_FALSE;
    } else {
        int i, n;

        glGetIntegerv(GL_NUM_EXTENSIONS, &n);
        for (i = 0; i < n; i++) {
            if (strcmp(pfGlGetStringi(GL_EXTENSIONS, i), extension) == 0)
                return GL_TRUE;
        }
        return GL_FALSE;
    }
}

/**
 * @internal
 * @~English
 * @brief Discover the capabilities of the current GL context.
 *
 * Queries the context and sets several the following internal variables
 * indicating the capabilities of the context:
 *
 * @li sizedFormats
 * @li supportsSwizzle
 * @li supportsSRGB
 * @li b16Formats
 */
static void
discoverContextCapabilities(void)
{
    GLint majorVersion = 1;
    GLint minorVersion = 0;

    // Done here so things will work when GLEW, or equivalent, is being used
    // and GL function names are defined as pointers. Initialization at
    // declaration would happen before these pointers have been initialized.
    INITIALIZE_GL_FUNCPTRS

    if (strstr(glGetString(GL_VERSION), "GL ES") != NULL)
        contextProfile = _CONTEXT_ES_PROFILE_BIT;
    // MAJOR & MINOR only introduced in GL {,ES} 3.0
    glGetIntegerv(GL_MAJOR_VERSION, &majorVersion);
    glGetIntegerv(GL_MINOR_VERSION, &minorVersion);
    if (glGetError() != GL_NO_ERROR) {
        // < v3.0; resort to the old-fashioned way.
        if (contextProfile & _CONTEXT_ES_PROFILE_BIT)
            sscanf(glGetString(GL_VERSION), "OpenGL ES %d.%d ",
                   &majorVersion, &minorVersion);
        else
            sscanf(glGetString(GL_VERSION), "OpenGL %d.%d ",
                   &majorVersion, &minorVersion);
    }
    if (contextProfile & _CONTEXT_ES_PROFILE_BIT) {
        if (majorVersion < 3) {
            supportsSwizzle = GL_FALSE;
            sizedFormats = _NO_SIZED_FORMATS;
            R16Formats = _KTX_NO_R16_FORMATS;
            supportsSRGB = GL_FALSE;
        } else {
            sizedFormats = _NON_LEGACY_FORMATS;
            if (hasExtension("GL_EXT_texture_cube_map_array")) {
                supportsCubeMapArrays = GL_TRUE;
            }
        }
        if (hasExtension("GL_OES_required_internalformat")) {
            sizedFormats |= _ALL_SIZED_FORMATS;
        }
        // There are no OES extensions for sRGB textures or R16 formats.
    } else {
        // PROFILE_MASK was introduced in OpenGL 3.2.
        // Profiles: CONTEXT_CORE_PROFILE_BIT 0x1,
        //           CONTEXT_COMPATIBILITY_PROFILE_BIT 0x2.
        glGetIntegerv(GL_CONTEXT_PROFILE_MASK, &contextProfile);
        if (glGetError() == GL_NO_ERROR) {
            // >= 3.2
            if (majorVersion == 3 && minorVersion < 3)
                supportsSwizzle = GL_FALSE;
            if ((contextProfile & GL_CONTEXT_CORE_PROFILE_BIT))
                sizedFormats &= ~_LEGACY_FORMATS;
            if (majorVersion >= 4)
                supportsCubeMapArrays = GL_TRUE;
        } else {
            // < 3.2
            contextProfile = GL_CONTEXT_COMPATIBILITY_PROFILE_BIT;
            supportsSwizzle = GL_FALSE;
            // sRGB textures introduced in 2.0
            if (majorVersion < 2 && hasExtension("GL_EXT_texture_sRGB")) {
                supportsSRGB = GL_FALSE;
            }
            // R{,G]16 introduced in 3.0; R{,G}16_SNORM introduced in 3.1.
            if (majorVersion == 3) {
                if (minorVersion == 0)
                    R16Formats &= ~_KTX_R16_FORMATS_SNORM;
            } else if (hasExtension("GL_ARB_texture_rg")) {
                R16Formats &= ~_KTX_R16_FORMATS_SNORM;
            } else {
                R16Formats = _KTX_NO_R16_FORMATS;
            }
        }
        if (!supportsCubeMapArrays) {
            if (hasExtension("GL_ARB_texture_cube_map_array")) {
                supportsCubeMapArrays = GL_TRUE;
            }
        }
    }
}

#if SUPPORT_LEGACY_FORMAT_CONVERSION
/**
 * @internal
 * @~English
 * @brief Convert deprecated legacy-format texture to modern format.
 *
 * The function sets the GL_TEXTURE_SWIZZLEs necessary to get the same
 * behavior as the legacy format.
 *
 * @param[in] target       texture target on which the swizzle will
 *                          be set.
 * @param[in,out] pFormat  pointer to variable holding the base format of the
 *                          texture. The new base format is written here.
 * @param[in,out] pInternalformat  pointer to variable holding the
 *                                  internalformat of the texture. The new
 *                                  internalformat is written here.
 * @return void unrecognized formats will be passed on to OpenGL. Any loading
 *              error that arises will be handled in the usual way.
 */
static void convertFormat(GLenum target, GLenum* pFormat, GLenum* pInternalformat) {
    switch (*pFormat) {
      case GL_ALPHA:
        {
          GLint swizzle[] = {GL_ZERO, GL_ZERO, GL_ZERO, GL_RED};
          *pFormat = GL_RED;
          glTexParameteriv(target, GL_TEXTURE_SWIZZLE_RGBA, swizzle);
          switch (*pInternalformat) {
            case GL_ALPHA:
            case GL_ALPHA4:
            case GL_ALPHA8:
              *pInternalformat = GL_R8;
              break;
            case GL_ALPHA12:
            case GL_ALPHA16:
              *pInternalformat = GL_R16;
              break;
          }
        }
      case GL_LUMINANCE:
        {
          GLint swizzle[] = {GL_RED, GL_RED, GL_RED, GL_ONE};
          *pFormat = GL_RED;
          glTexParameteriv(target, GL_TEXTURE_SWIZZLE_RGBA, swizzle);
          switch (*pInternalformat) {
            case GL_LUMINANCE:
            case GL_LUMINANCE4:
            case GL_LUMINANCE8:
              *pInternalformat = GL_R8;
              break;
            case GL_LUMINANCE12:
            case GL_LUMINANCE16:
              *pInternalformat = GL_R16;
              break;
#if 0
            // XXX Must avoid setting TEXTURE_SWIZZLE in these cases
            // XXX Must manually swizzle.
            case GL_SLUMINANCE:
            case GL_SLUMINANCE8:
              *pInternalformat = GL_SRGB8;
              break;
#endif
          }
          break;
        }
      case GL_LUMINANCE_ALPHA:
        {
          GLint swizzle[] = {GL_RED, GL_RED, GL_RED, GL_GREEN};
          *pFormat = GL_RG;
          glTexParameteriv(target, GL_TEXTURE_SWIZZLE_RGBA, swizzle);
          switch (*pInternalformat) {
            case GL_LUMINANCE_ALPHA:
            case GL_LUMINANCE4_ALPHA4:
            case GL_LUMINANCE6_ALPHA2:
            case GL_LUMINANCE8_ALPHA8:
              *pInternalformat = GL_RG8;
              break;
            case GL_LUMINANCE12_ALPHA4:
            case GL_LUMINANCE12_ALPHA12:
            case GL_LUMINANCE16_ALPHA16:
              *pInternalformat = GL_RG16;
              break;
#if 0
            // XXX Must avoid setting TEXTURE_SWIZZLE in these cases
            // XXX Must manually swizzle.
            case GL_SLUMINANCE_ALPHA:
            case GL_SLUMINANCE8_ALPHA8:
              *pInternalformat = GL_SRGB8_ALPHA8;
              break;
#endif
          }
          break;
        }
      case GL_INTENSITY:
        {
          GLint swizzle[] = {GL_RED, GL_RED, GL_RED, GL_RED};
          *pFormat = GL_RED;
          glTexParameteriv(target, GL_TEXTURE_SWIZZLE_RGBA, swizzle);
          switch (*pInternalformat) {
            case GL_INTENSITY:
            case GL_INTENSITY4:
            case GL_INTENSITY8:
              *pInternalformat = GL_R8;
              break;
            case GL_INTENSITY12:
            case GL_INTENSITY16:
              *pInternalformat = GL_R16;
              break;
          }
          break;
        }
      default:
        break;
    }
}
#endif /* SUPPORT_LEGACY_FORMAT_CONVERSION */

/* [cbdata] */
typedef struct ktx_cbdata {
    GLenum glTarget;
    GLenum glFormat;
    GLenum glInternalformat;
    GLenum glType;
    GLenum glError;
    GLuint numLayers;
} ktx_cbdata;
/* [cbdata] */

/* [imageCallbacks] */

KTX_error_code KTXAPIENTRY
texImage1DCallback(int miplevel, int face,
                   int width, int height,
                   int depth,
                   ktx_uint32_t faceLodSize,
                   void* pixels, void* userdata)
{
    ktx_cbdata* cbData = (ktx_cbdata*)userdata;
    
    assert(pfGlTexImage1D != NULL);
    pfGlTexImage1D(cbData->glTarget + face, miplevel,
                   cbData->glInternalformat, width, 0,
                   cbData->glFormat, cbData->glType, pixels);
    
    if ((cbData->glError = glGetError()) == GL_NO_ERROR) {
        return KTX_SUCCESS;
    } else {
        return KTX_GL_ERROR;
    }
}

KTX_error_code KTXAPIENTRY
compressedTexImage1DCallback(int miplevel, int face,
                             int width, int height,
                             int depth,
                             ktx_uint32_t faceLodSize,
                             void* pixels, void* userdata)
{
    ktx_cbdata* cbData = (ktx_cbdata*)userdata;
    
    assert(pfGlCompressedTexImage1D != NULL);
    pfGlCompressedTexImage1D(cbData->glTarget + face, miplevel,
                             cbData->glInternalformat, width, 0,
                             faceLodSize, pixels);
    
    if ((cbData->glError = glGetError()) == GL_NO_ERROR) {
        return KTX_SUCCESS;
    } else {
        return KTX_GL_ERROR;
    }
}

KTX_error_code KTXAPIENTRY
texImage2DCallback(int miplevel, int face,
                   int width, int height,
                   int depth,
                   ktx_uint32_t faceLodSize,
                   void* pixels, void* userdata)
{
    ktx_cbdata* cbData = (ktx_cbdata*)userdata;
 
    glTexImage2D(cbData->glTarget + face, miplevel,
                 cbData->glInternalformat, width,
                 cbData->numLayers == 0 ? height : cbData->numLayers, 0,
                 cbData->glFormat, cbData->glType, pixels);

    if ((cbData->glError = glGetError()) == GL_NO_ERROR) {
        return KTX_SUCCESS;
    } else {
        return KTX_GL_ERROR;
    }
}


KTX_error_code KTXAPIENTRY
compressedTexImage2DCallback(int miplevel, int face,
                             int width, int height,
                             int depth,
                             ktx_uint32_t faceLodSize,
                             void* pixels, void* userdata)
{
    ktx_cbdata* cbData = (ktx_cbdata*)userdata;
    GLenum glerror;
    KTX_error_code result;
    
    // It is simpler to just attempt to load the format, rather than divine
    // which formats are supported by the implementation. In the event of an
    // error, software unpacking can be attempted.
    glCompressedTexImage2D(cbData->glTarget + face, miplevel,
                           cbData->glInternalformat, width,
                           cbData->numLayers == 0 ? height : cbData->numLayers,
                           0,
                           faceLodSize, pixels);
    
    glerror = glGetError();
#if SUPPORT_SOFTWARE_ETC_UNPACK
    // Renderion is returning INVALID_VALUE. Oops!!
    if ((glerror == GL_INVALID_ENUM || glerror == GL_INVALID_VALUE)
        && (cbData->glInternalformat == GL_ETC1_RGB8_OES
            || (cbData->glInternalformat >= GL_COMPRESSED_R11_EAC
                && cbData->glInternalformat <= GL_COMPRESSED_SRGB8_ALPHA8_ETC2_EAC)
            ))
    {
        GLubyte* unpacked;
        GLenum format, internalformat, type;
        
        result = _ktxUnpackETC((GLubyte*)pixels, cbData->glInternalformat,
                                  width, height, &unpacked,
                                  &format, &internalformat,
                                  &type, R16Formats, supportsSRGB);
        if (result != KTX_SUCCESS) {
            return result;
        }
        if (!(sizedFormats & _NON_LEGACY_FORMATS)) {
            if (internalformat == GL_RGB8)
                internalformat = GL_RGB;
            else if (internalformat == GL_RGBA8)
                internalformat = GL_RGBA;
        }
        glTexImage2D(cbData->glTarget + face, miplevel,
                     internalformat, width,
                     cbData->numLayers == 0 ? height : cbData->numLayers, 0,
                     format, type, unpacked);
        
        free(unpacked);
        glerror = glGetError();
    }
#endif
    
    if ((cbData->glError = glerror) == GL_NO_ERROR) {
        return KTX_SUCCESS;
    } else {
        return KTX_GL_ERROR;
    }
}

KTX_error_code KTXAPIENTRY
texImage3DCallback(int miplevel, int face,
                   int width, int height,
                   int depth,
                   ktx_uint32_t faceLodSize,
                   void* pixels, void* userdata)
{
    ktx_cbdata* cbData = (ktx_cbdata*)userdata;
    
    assert(pfGlTexImage3D != NULL);
    pfGlTexImage3D(cbData->glTarget + face, miplevel,
                   cbData->glInternalformat,
                   width, height,
                   cbData->numLayers == 0 ? depth : cbData->numLayers,
                   0,
                   cbData->glFormat, cbData->glType, pixels);
    
    if ((cbData->glError = glGetError()) == GL_NO_ERROR) {
        return KTX_SUCCESS;
    } else {
        return KTX_GL_ERROR;
    }
}

KTX_error_code KTXAPIENTRY
compressedTexImage3DCallback(int miplevel, int face,
                             int width, int height,
                             int depth,
                             ktx_uint32_t faceLodSize,
                             void* pixels, void* userdata)
{
    ktx_cbdata* cbData = (ktx_cbdata*)userdata;
    
    assert(pfGlCompressedTexImage3D != NULL);
    pfGlCompressedTexImage3D(cbData->glTarget + face, miplevel,
                             cbData->glInternalformat,
                             width, height,
                             cbData->numLayers == 0 ? depth : cbData->numLayers,
                             0,
                             faceLodSize, pixels);
    
    if ((cbData->glError = glGetError()) == GL_NO_ERROR) {
        return KTX_SUCCESS;
    } else {
        return KTX_GL_ERROR;
    }
}
/* [imageCallbacks] */

/**
 * @memberof ktxTexture
 * @~English
 * @brief Create a GL texture object from a ktxTexture object.
 *
 * Sets the texture object's GL_TEXTURE_MAX_LEVEL parameter according to the
 * number of levels in the KTX data, provided the library has been compiled
 * with a version of gl.h where GL_TEXTURE_MAX_LEVEL is defined.
 *
 * Unpacks compressed GL_ETC1_RGB8_OES and GL_ETC2_* format
 * textures in software when the format is not supported by the GL context,
 * provided the library has been compiled with SUPPORT_SOFTWARE_ETC_UNPACK
 * defined as 1.
 *
 * It will also convert textures with legacy formats to their modern equivalents
 * when the format is not supported by the GL context, provided the library
 * has been compiled with SUPPORT_LEGACY_FORMAT_CONVERSION defined as 1.
 *
 * @param[in] This          handle of the ktxTexture to upload.
 * @param[in,out] pTexture  name of the GL texture object to load. If NULL or if
 *                          <tt>*pTexture == 0</tt> the function will generate
 *                          a texture name. The function binds either the
 *                          generated name or the name given in @p *pTexture
 *                          to the texture target returned in @p *pTarget,
 *                          before loading the texture data. If @p pTexture
 *                          is not NULL and a name was generated, the generated
 *                          name will be returned in *pTexture.
 * @param[out] pTarget      @p *pTarget is set to the texture target used. The
 *                          target is chosen based on the file contents.
 * @param[out] pGlerror     @p *pGlerror is set to the value returned by
 *                          glGetError when this function returns the error
 *                          KTX_GL_ERROR. glerror can be NULL.
 *
 * @return  KTX_SUCCESS on success, other KTX_* enum values on error.
 *
 * @exception KTX_INVALID_VALUE @p This or @p target is @c NULL or the size of
 *                              a mip level is greater than the size of the
 *                              preceding level.
 * @exception KTX_GL_ERROR      A GL error was raised by glBindTexture,
 *                              glGenTextures or gl*TexImage*. The GL error
 *                              will be returned in @p *glerror, if glerror
 *                              is not @c NULL.
 * @exception KTX_UNSUPPORTED_TEXTURE_TYPE The type of texture is not supported
 *                                         by the current OpenGL context.
 */
/* [loadGLTexture] */
KTX_error_code
ktxTexture_GLUpload(ktxTexture* This, GLuint* pTexture, GLenum* pTarget,
                    GLenum* pGlerror)
{
    GLint                 previousUnpackAlignment;
    GLuint                texname;
    GLenum                target = GL_TEXTURE_2D;
    int                   texnameUser;
    KTX_error_code        result = KTX_SUCCESS;
    PFNKTXITERCB          iterCb = NULL;
    ktx_cbdata            cbData;
    int                   dimensions;

    if (pGlerror)
        *pGlerror = GL_NO_ERROR;

    if (!This) {
        return KTX_INVALID_VALUE;
    }

    if (!pTarget) {
        return KTX_INVALID_VALUE;
    }

    if (contextProfile == 0)
        discoverContextCapabilities();

    /* KTX files require an unpack alignment of 4 */
    glGetIntegerv(GL_UNPACK_ALIGNMENT, &previousUnpackAlignment);
    if (previousUnpackAlignment != KTX_GL_UNPACK_ALIGNMENT) {
        glPixelStorei(GL_UNPACK_ALIGNMENT, KTX_GL_UNPACK_ALIGNMENT);
    }

    cbData.glFormat = This->glFormat;
    cbData.glInternalformat = This->glInternalformat;
    cbData.glType = This->glType;

    texnameUser = pTexture && *pTexture;
    if (texnameUser) {
        texname = *pTexture;
    } else {
        glGenTextures(1, &texname);
    }

    dimensions = This->numDimensions;
    if (This->isArray) {
        dimensions += 1;
        if (This->numFaces == 6) {
            /* _ktxCheckHeader should have caught this. */
            assert(This->numDimensions == 2);
            target = GL_TEXTURE_CUBE_MAP_ARRAY;
        } else {
            switch (This->numDimensions) {
              case 1: target = GL_TEXTURE_1D_ARRAY_EXT; break;
              case 2: target = GL_TEXTURE_2D_ARRAY_EXT; break;
              /* _ktxCheckHeader should have caught this. */
              default: assert(KTX_TRUE);
            }
        }
        cbData.numLayers = This->numLayers;
    } else {
        if (This->numFaces == 6) {
            /* _ktxCheckHeader should have caught this. */
            assert(This->numDimensions == 2);
            target = GL_TEXTURE_CUBE_MAP;
        } else {
            switch (This->numDimensions) {
              case 1: target = GL_TEXTURE_1D; break;
              case 2: target = GL_TEXTURE_2D; break;
              case 3: target = GL_TEXTURE_3D; break;
              /* _ktxCheckHeader shold have caught this. */
              default: assert(KTX_TRUE);
            }
        }
        cbData.numLayers = 0;
    }
    
    if (target == GL_TEXTURE_1D &&
        ((This->isCompressed && (pfGlCompressedTexImage1D == NULL)) ||
         (!This->isCompressed && (pfGlTexImage1D == NULL))))
    {
        return KTX_UNSUPPORTED_TEXTURE_TYPE;
    }
    
    /* Reject 3D texture if unsupported. */
    if (target == GL_TEXTURE_3D &&
        ((This->isCompressed && (pfGlCompressedTexImage3D == NULL)) ||
         (!This->isCompressed && (pfGlTexImage3D == NULL))))
    {
        return KTX_UNSUPPORTED_TEXTURE_TYPE;
    }
    
    /* Reject cube map arrays if not supported. */
    if (target == GL_TEXTURE_CUBE_MAP_ARRAY && !supportsCubeMapArrays) {
        return KTX_UNSUPPORTED_TEXTURE_TYPE;
    }
    
    /* XXX Need to reject other array textures & cube maps if not supported. */
    
    switch (dimensions) {
      case 1:
        iterCb = This->isCompressed
                  ? compressedTexImage1DCallback : texImage1DCallback;
        break;
      case 2:
        iterCb = This->isCompressed
                  ? compressedTexImage2DCallback : texImage2DCallback;
            break;
      case 3:
        iterCb = This->isCompressed
                  ? compressedTexImage3DCallback : texImage3DCallback;
        break;
      default:
            assert(KTX_TRUE);
    }
   
    glBindTexture(target, texname);
    
    // Prefer glGenerateMipmaps over GL_GENERATE_MIPMAP
    if (This->generateMipmaps && (pfGlGenerateMipmap == NULL)) {
        glTexParameteri(target, GL_GENERATE_MIPMAP, GL_TRUE);
    }
#ifdef GL_TEXTURE_MAX_LEVEL
    if (!This->generateMipmaps)
        glTexParameteri(target, GL_TEXTURE_MAX_LEVEL, This->numLevels - 1);
#endif

    if (target == GL_TEXTURE_CUBE_MAP) {
        cbData.glTarget = GL_TEXTURE_CUBE_MAP_POSITIVE_X;
    } else {
        cbData.glTarget = target;
    }
    
    cbData.glInternalformat = This->glInternalformat;
    cbData.glFormat = This->glFormat;
    if (!This->isCompressed) {
#if SUPPORT_LEGACY_FORMAT_CONVERSION
        // If sized legacy formats are supported there is no need to convert.
        // If only unsized formats are supported, there is no point in
        // converting as the modern formats aren't supported either.
        if (sizedFormats == _NON_LEGACY_FORMATS && supportsSwizzle) {
            convertFormat(target, &cbData.glFormat, &cbData.glInternalformat);
        } else if (sizedFormats == _NO_SIZED_FORMATS)
            cbData.glInternalformat = This->glBaseInternalformat;
#else
        // When no sized formats are supported, or legacy sized formats are not
        // supported, must change internal format.
        if (sizedFormats == _NO_SIZED_FORMATS
            || (!(sizedFormats & _LEGACY_FORMATS) &&
                (This->glBaseInternalformat == GL_ALPHA
                || This->glBaseInternalformat == GL_LUMINANCE
                || This->glBaseInternalformat == GL_LUMINANCE_ALPHA
                || This->glBaseInternalformat == GL_INTENSITY))) {
            cbData.glInternalformat = This->glBaseInternalformat;
        }
#endif
    }

    if (ktxTexture_isActiveStream(This))
        result = ktxTexture_IterateLoadLevelFaces(This, iterCb, &cbData);
    else
        result = ktxTexture_IterateLevelFaces(This, iterCb, &cbData);

    /* GL errors are the only reason for failure. */
    if (result != KTX_SUCCESS && cbData.glError != GL_NO_ERROR) {
        if (pGlerror)
            *pGlerror = cbData.glError;
    }

    /* restore previous GL state */
    if (previousUnpackAlignment != KTX_GL_UNPACK_ALIGNMENT) {
        glPixelStorei(GL_UNPACK_ALIGNMENT, previousUnpackAlignment);
    }

    if (result == KTX_SUCCESS)
    {
        // Prefer glGenerateMipmaps over GL_GENERATE_MIPMAP
        if (This->generateMipmaps && pfGlGenerateMipmap) {
            pfGlGenerateMipmap(target);
        }
        *pTarget = target;
        if (pTexture) {
            *pTexture = texname;
        }
    } else if (!texnameUser) {
        glDeleteTextures(1, &texname);
    }
    return result;
}
/* [loadGLTexture] */

/**
 * @~English
 * @deprecated Use ktxTexture_CreateFromStdioStream() and ktxTexture_GLUpload().
 * @brief Create a GL texture object from KTX data in a stdio FILE stream.
 *
 * Sets the texture object's GL_TEXTURE_MAX_LEVEL parameter according to the
 * number of levels in the ktxStream, provided the library has been compiled
 * with a version of gl.h where GL_TEXTURE_MAX_LEVEL is defined.
 *
 * Unpacks compressed GL_ETC1_RGB8_OES and GL_ETC2_* format textures in
 * software when the format is not supported by the GL context, provided the
 * library has been compiled with SUPPORT_SOFTWARE_ETC_UNPACK defined as 1.
 *
 * Also converts texture with legacy formats to their modern equivalents
 * when the format is not supported by the GL context, provided the library
 * has been compiled with SUPPORT_LEGACY_FORMAT_CONVERSION defined as 1.
 *
 * @param[in] file         stdio stream FILE pointer
 * @param[in,out] pTexture name of the GL texture object to load. If NULL or if
 *                         <tt>*pTexture == 0</tt> the function will generate
 *                         a texture name. The function binds either the
 *                         generated name or the name given in @p *pTexture
 *                         to the texture target returned in @p *pTarget,
 *                         before loading the texture data. If @p pTexture
 *                         is not NULL and a name was generated, the generated
 *                         name will be returned in *pTexture.
 * @param[out] pTarget     @p *pTarget is set to the texture target used. The
 *                         target is chosen based on the file contents.
 * @param[out] pDimensions If @p pDimensions is not NULL, the width, height and
 *                         depth of the texture's base level are returned in
 *                         the fields of the KTX_dimensions structure to which
 *                         it points.
 * @param[out] pIsMipmapped
 *                         If @p pIsMipmapped is not NULL, @p *pIsMipmapped is
 *                         set to GL_TRUE if the KTX texture is mipmapped,
 *                         GL_FALSE otherwise.
 * @param[out] pGlerror    @p *pGlerror is set to the value returned by
 *                         glGetError when this function returns the error
 *                         KTX_GL_ERROR. glerror can be NULL.
 * @param[in,out] pKvdLen  If not NULL, @p *pKvdLen is set to the number of
 *                         bytes of key-value data pointed at by @p *ppKvd.
 *                         Must not be NULL, if @p ppKvd is not NULL.
 * @param[in,out] ppKvd    If not NULL, @p *ppKvd is set to the point to a
 *                         block of memory containing key-value data read from
 *                         the file. The application is responsible for freeing
 *                         the memory.
 *
 * @return  KTX_SUCCESS on success, other KTX_* enum values on error.
 *
 * @exception KTX_INVALID_VALUE @p target is @c NULL or the size of a mip
 *                              level is greater than the size of the
 *                              preceding level.
 * @exception KTX_INVALID_OPERATION @p ppKvd is not NULL but pKvdLen is NULL.
 * @exception KTX_UNEXPECTED_END_OF_FILE the file does not contain the
 *                                       expected amount of data.
 * @exception KTX_OUT_OF_MEMORY Sufficient memory could not be allocated for the
 *                              underlying ktxTexture object or to store the
 *                              requested key-value data.
 * @exception KTX_GL_ERROR      A GL error was raised by glBindTexture,
 *                              glGenTextures or gl*TexImage*. The GL error
 *                              will be returned in @p *glerror, if glerror
 *                              is not @c NULL.
 * @exception KTX_UNSUPPORTED_TEXTURE_TYPE The type of texture is not supported
 *                                         by the current OpenGL context.
 */
KTX_error_code
ktxLoadTextureF(FILE* file, GLuint* pTexture, GLenum* pTarget,
                KTX_dimensions* pDimensions, GLboolean* pIsMipmapped,
                GLenum* pGlerror,
                unsigned int* pKvdLen, unsigned char** ppKvd)
{
    ktxTexture* texture;
    KTX_error_code result = KTX_SUCCESS;
    
    if (ppKvd != NULL && pKvdLen == NULL)
        return KTX_INVALID_VALUE;

    result = ktxTexture_CreateFromStdioStream(file,
                                              KTX_TEXTURE_CREATE_RAW_KVDATA_BIT,
                                              &texture);
    if (result != KTX_SUCCESS)
        return result;

    result = ktxTexture_GLUpload(texture, pTexture, pTarget, pGlerror);
    
    if (result == KTX_SUCCESS) {
        if (ppKvd != NULL) {
            *ppKvd = texture->kvData;
            *pKvdLen = texture->kvDataLen;
            /* Remove to avoid it being freed when texture is destroyed. */
            texture->kvData = NULL;
            texture->kvDataLen = 0;
        }
        if (pDimensions) {
            pDimensions->width = texture->baseWidth;
            pDimensions->height = texture->baseHeight;
            pDimensions->depth = texture->baseDepth;
        }
        if (pIsMipmapped) {
            if (texture->generateMipmaps || texture->numLevels > 1)
                *pIsMipmapped = GL_TRUE;
            else
                *pIsMipmapped = GL_FALSE;
        }
    }

    ktxTexture_Destroy(texture);

    return result;
}

/**
 * @~English
 * @deprecated Use ktxTexture_CreateFromNamedFile() and ktxTexture_GLUpload().
 * @brief Create a GL texture object from KTX data in a named file on disk.
 *
 * @param[in] filename      pointer to a C string that contains the path of
 *                          the file to load.
 * @param[in,out] pTexture  name of the GL texture object to load. See
 *                          ktxLoadTextureF() for details.
 * @param[out] pTarget      @p *pTarget is set to the texture target used. See
 *                          ktxLoadTextureF() for details.
 * @param[out] pDimensions  @p the texture's base level width depth and height
 *                          are returned in structure to which this points.
 *                          See ktxLoadTextureF() for details.
 * @param[out] pIsMipmapped @p pIsMipMapped is set to indicate if the loaded
 *                          texture is mipmapped. See ktxLoadTextureF() for
 *                          details.
 * @param[out] pGlerror     @p *pGlerror is set to the value returned by
 *                          glGetError when this function returns the error
 *                          KTX_GL_ERROR. glerror can be NULL.
 * @param[in,out] pKvdLen   If not NULL, @p *pKvdLen is set to the number of
 *                          bytes of key-value data pointed at by @p *ppKvd.
 *                          Must not be NULL, if @p ppKvd is not NULL.
 * @param[in,out] ppKvd     If not NULL, @p *ppKvd is set to the point to a
 *                          block of memory containing key-value data read from
 *                          the file. The application is responsible for freeing
 *                          the memory.
 *
 * @return  KTX_SUCCESS on success, other KTX_* enum values on error.
 *
 * @exception KTX_FILE_OPEN_FAILED  The specified file could not be opened.
 * @exception KTX_INVALID_VALUE     See ktxLoadTextureF() for causes.
 * @exception KTX_INVALID_OPERATION See ktxLoadTextureF() for causes.
 * @exception KTX_UNEXPECTED_END_OF_FILE See ktxLoadTextureF() for causes.
 * @exception KTX_GL_ERROR          See ktxLoadTextureF() for causes.
 * @exception KTX_UNSUPPORTED_TEXTURE_TYPE See ktxLoadTextureF() for causes.
 */
KTX_error_code
ktxLoadTextureN(const char* const filename, GLuint* pTexture, GLenum* pTarget,
                KTX_dimensions* pDimensions, GLboolean* pIsMipmapped,
                GLenum* pGlerror,
                unsigned int* pKvdLen, unsigned char** ppKvd)
{
    KTX_error_code result;

    FILE* file = fopen(filename, "rb");
    if (file) {
        result = ktxLoadTextureF(file, pTexture, pTarget, pDimensions,
                                 pIsMipmapped, pGlerror, pKvdLen, ppKvd);
        fclose(file);
    } else
        result = KTX_FILE_OPEN_FAILED;

    return result;
}

/**
 * @~English
 * @deprecated Use ktxTexture_CreateFromMemory() and ktxTexture_GLUpload().
 * @brief Create a GL texture object from KTX formatted data in memory.
 *
 * @param[in] bytes         pointer to the array of bytes containing
 *                          the KTX data to load.
 * @param[in] size          size of the memory array containing the
 *                          KTX format data.
 * @param[in,out] pTexture  name of the GL texture object to load. See
 *                          ktxLoadTextureF() for details.
 * @param[out] pTarget      @p *pTarget is set to the texture target used. See
 *                          ktxLoadTextureF() for details.
 * @param[out] pDimensions  @p the texture's base level width depth and height
 *                          are returned in structure to which this points.
 *                          See ktxLoadTextureF() for details.
 * @param[out] pIsMipmapped @p *pIsMipMapped is set to indicate if the loaded
 *                          texture is mipmapped. See ktxLoadTextureF() for
 *                          details.
 * @param[out] pGlerror     @p *pGlerror is set to the value returned by
 *                          glGetError when this function returns the error
 *                          KTX_GL_ERROR. glerror can be NULL.
 * @param[in,out] pKvdLen   If not NULL, @p *pKvdLen is set to the number of
 *                          bytes of key-value data pointed at by @p *ppKvd.
 *                          Must not be NULL, if @p ppKvd is not NULL.
 * @param[in,out] ppKvd     If not NULL, @p *ppKvd is set to the point to a
 *                          block of memory containing key-value data read from
 *                          the file. The application is responsible for freeing
 *                          the memory.
 *
 * @return  KTX_SUCCESS on success, other KTX_* enum values on error.
 *
 * @exception KTX_FILE_OPEN_FAILED  The specified memory could not be opened as
 *                                  a file.
 * @exception KTX_INVALID_VALUE     See ktxLoadTextureF() for causes.
 * @exception KTX_INVALID_OPERATION See ktxLoadTextureF() for causes.
 * @exception KTX_UNEXPECTED_END_OF_FILE See ktxLoadTextureF() for causes.
 *
 * @exception KTX_GL_ERROR          See ktxLoadTextureF() for causes.
 * @exception KTX_UNSUPPORTED_TEXTURE_TYPE See ktxLoadTextureF() for causes.
 */
KTX_error_code
ktxLoadTextureM(const void* bytes, GLsizei size, GLuint* pTexture,
                GLenum* pTarget, KTX_dimensions* pDimensions,
                GLboolean* pIsMipmapped, GLenum* pGlerror,
                unsigned int* pKvdLen, unsigned char** ppKvd)
{
    ktxTexture* texture;
    KTX_error_code result = KTX_SUCCESS;

    if (ppKvd != NULL && pKvdLen == NULL)
        return KTX_INVALID_VALUE;
    
    result = ktxTexture_CreateFromMemory(bytes, size,
                                         KTX_TEXTURE_CREATE_RAW_KVDATA_BIT,
                                         &texture);

    if (result != KTX_SUCCESS)
        return result;

    result = ktxTexture_GLUpload(texture, pTexture, pTarget, pGlerror);
    if (result == KTX_SUCCESS) {
        if (ppKvd != NULL) {
            *ppKvd = texture->kvData;
            *pKvdLen = texture->kvDataLen;
            /* Remove to avoid it being freed when texture is destroyed. */
            texture->kvData = NULL;
            texture->kvDataLen = 0;
        }
        if (pDimensions) {
            pDimensions->width = texture->baseWidth;
            pDimensions->height = texture->baseHeight;
            pDimensions->depth = texture->baseDepth;
        }
        if (pIsMipmapped) {
            if (texture->generateMipmaps || texture->numLevels > 1)
                *pIsMipmapped = GL_TRUE;
            else
                *pIsMipmapped = GL_FALSE;
        }
    }
    
    ktxTexture_Destroy(texture);

    return result;
}

/** @} */
