/* -*- tab-width: 4; -*- */
/* vi: set sw=2 ts=4 expandtab: */

/* $Id: 687889ad2b1bee58a6d439ef4d6c10830a733418 $ */

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
 * @file writer.c
 * @~English
 *
 * @brief V1 API functions for creating KTX-format files from a set of images.
 *
 * Keep the v1 API implementation as is because reimplementing it in terms
 * of the v2 API would use too much memory. This is because this API expects
 * all the images to already be loaded in memory and the v2 api would load
 * them into another memory buffer prior to writing the file.
 *
 * @author Mark Callow, when at HI Corporation
 */

#ifdef _WIN32
#define _CRT_SECURE_NO_WARNINGS
#endif

#include <assert.h>
#include <limits.h>

#include "GL/glcorearb.h"
#include "ktx.h"
#include "ktxint.h"
#include "stream.h"
#include "filestream.h"
#include "memstream.h"
#include "gl_format.h"

static GLenum getFormatFromInternalFormatLegacy(GLenum internalFormat);
static GLenum getTypeFromInternalFormatLegacy(GLenum internalFormat);
static void getFormatSizeLegacy(GLenum internalFormat,
                                GlFormatSize* formatSize);

/**
 * @internal
 * @ingroup writer
 * @~English
 * @deprecated Use ktxTexture_writeToStream().
 * @brief Write image(s) in a KTX-format to a ktxStream.
 *
 * @param [in] stream       pointer to the ktxStream from which to load.
 * @param [in] textureInfo  pointer to a KTX_texture_info structure providing
 *                          information about the images to be included in
 *                          the KTX file.
 * @param [in] bytesOfKeyValueData
 *                          specifies the number of bytes of key-value data.
 * @param [in] keyValueData a pointer to the keyValue data.
 * @param [in] numImages    number of images in the following array
 * @param [in] images       array of KTX_image_info providing image size and
 *                          data.
 *
 * @return  KTX_SUCCESS on success, other KTX_* enum values on error.
 *
 * @exception KTX_INVALID_VALUE @c glTypeSize in @p textureInfo is not 1, 2, or
 *                              4 or is different from the size of the type
 *                              specified in @c glType.
 * @exception KTX_INVALID_VALUE @c pixelWidth in @p textureInfo is 0 or
 *                              pixelDepth != 0 && pixelHeight == 0.
 * @exception KTX_INVALID_VALUE In @p textureInfo, @c numberOfFaces != 1 or
 *                              numberOfFaces != 6 or numberOfArrayElements
 *                              or numberOfMipmapLevels are < 0.
 * @exception KTX_INVALID_VALUE @c glType in @p textureInfo is an unrecognized
 *                              type.
 * @exception KTX_INVALID_OPERATION
 *                              In @p textureInfo, numberOfFaces == 6 and
 *                              images are either not 2D or are not square.
 * @exception KTX_INVALID_OPERATION
 *                              @p numImages is insufficient for the specified
 *                              number of mipmap levels and faces.
 * @exception KTX_INVALID_OPERATION
 *                              The size of a provided image is different than
 *                              that required for the specified width, height
 *                              or depth or for the mipmap level being
 *                              processed.
 * @exception KTX_INVALID_OPERATION
 *                              @c glType and @c glFormat in @p textureInfo are
 *                              mismatched. See OpenGL 4.4 specification
 *                              section 8.4.4 and table 8.5.
 * @exception KTX_FILE_WRITE_ERROR
 *                              A system error occurred while writing the file.
 * @exception KTX_OUT_OF_MEMORY System failed to allocate sufficient memory.
 */
static
KTX_error_code
ktxWriteKTXS(struct ktxStream *stream, const KTX_texture_info* textureInfo,
             GLsizei bytesOfKeyValueData, const void* keyValueData,
             GLuint numImages, KTX_image_info images[])
{
    KTX_header header = KTX_IDENTIFIER_REF;
    GLuint i, level, dimension, cubemap = 0;
    GLuint numMipmapLevels, numArrayElements;
    GLbyte pad[4] = { 0, 0, 0, 0 };
    KTX_error_code result = KTX_SUCCESS;
    GLboolean compressed = GL_FALSE;
    GLuint groupBytes;

    if (!stream) {
        return KTX_INVALID_VALUE;
    }

    /* endianess int.. if this comes out reversed, all of the other ints will
     * too.
     */
    header.endianness = KTX_ENDIAN_REF;
    header.glType = textureInfo->glType;
    header.glTypeSize = textureInfo->glTypeSize;
    header.glFormat = textureInfo->glFormat;
    header.glInternalformat = textureInfo->glInternalFormat;
    header.glBaseInternalformat = textureInfo->glBaseInternalFormat;
    header.pixelWidth = textureInfo->pixelWidth;
    header.pixelHeight = textureInfo->pixelHeight;
    header.pixelDepth = textureInfo->pixelDepth;
    header.numberOfArrayElements = textureInfo->numberOfArrayElements;
    header.numberOfFaces = textureInfo->numberOfFaces;
    header.numberOfMipmapLevels = textureInfo->numberOfMipmapLevels;
    header.bytesOfKeyValueData = bytesOfKeyValueData;

    /* Do some sanity checking */
    if (header.glTypeSize != 1 &&
        header.glTypeSize != 2 &&
        header.glTypeSize != 4)
    {
        /* Only 8, 16, and 32-bit types are supported for byte-swapping.
         * See UNPACK_SWAP_BYTES & table 8.4 in the OpenGL 4.4 spec.
         */
        return KTX_INVALID_VALUE;
    }

    if (header.glType == 0 || header.glFormat == 0)
    {
        if (header.glType + header.glFormat != 0) {
            /* either both or neither of glType & glFormat must be zero */
            return KTX_INVALID_VALUE;
        } else
            compressed = GL_TRUE;
    }
    else
    {
        GlFormatSize formatInfo;
        GLenum expectedFormat, expectedType;
        
        expectedFormat = getFormatFromInternalFormatLegacy(header.glInternalformat);
        if (expectedFormat == GL_INVALID_VALUE
            || expectedFormat != header.glFormat)
            return KTX_INVALID_OPERATION;
        expectedType = getTypeFromInternalFormatLegacy(header.glInternalformat);
        if (expectedType == GL_INVALID_VALUE
            || expectedType != header.glType)
            return KTX_INVALID_OPERATION;
        getFormatSizeLegacy(header.glInternalformat, &formatInfo);
        groupBytes = formatInfo.blockSizeInBits / CHAR_BIT;
    }


    /* Check texture dimensions. KTX files can store 8 types of textures:
     * 1D, 2D, 3D, cube, and array variants of these. There is currently
     * no GL extension that would accept 3D array array textures but we'll
     * let such files be created.
     */
    if ((header.pixelWidth == 0) ||
        (header.pixelDepth > 0 && header.pixelHeight == 0))
    {
        /* texture must have width */
        /* texture must have height if it has depth */
        return KTX_INVALID_VALUE;
    }
    if (header.pixelHeight > 0 && header.pixelDepth > 0)
        dimension = 3;
    else if (header.pixelHeight > 0)
        dimension = 2;
    else
        dimension = 1;

    if (header.numberOfFaces != 1 && header.pixelDepth != 0)
    {
        /* No 3D cubemaps so either faces or depth must be 1. */
            return KTX_INVALID_OPERATION;
    }

    if (header.numberOfFaces == 6)
    {
        if (dimension != 2)
        {
            /* cube map needs 2D faces */
            return KTX_INVALID_OPERATION;
        }
        if (header.pixelWidth != header.pixelHeight)
        {
            /* cube maps require square images */
            return KTX_INVALID_OPERATION;
        }
    }
    else if (header.numberOfFaces != 1)
    {
        /* numberOfFaces must be either 1 or 6 */
        return KTX_INVALID_VALUE;
    }
 
    if (header.numberOfArrayElements == 0)
        numArrayElements = 1;
    else
        numArrayElements = header.numberOfArrayElements;
    
    if (header.numberOfFaces == 6)
        cubemap = 1;

    /* Check number of mipmap levels */
    if (header.numberOfMipmapLevels == 0)
    {
        numMipmapLevels = 1;
    }
    else
        numMipmapLevels = header.numberOfMipmapLevels;
    if (numMipmapLevels > 1) {
        GLuint max_dim = MAX(MAX(header.pixelWidth, header.pixelHeight), header.pixelDepth);
        if (max_dim < ((GLuint)1 << (header.numberOfMipmapLevels - 1)))
        {
            /* Can't have more mip levels than 1 + log2(max(width, height, depth)) */
            return KTX_INVALID_VALUE;
        }
    }

    if (numImages < numMipmapLevels * header.numberOfFaces)
    {
        /* Not enough images */
        return KTX_INVALID_OPERATION;
    }

    //write header
    result = stream->write(stream, &header, sizeof(KTX_header), 1);
    if (result != KTX_SUCCESS)
        return result;

    //write keyValueData
    if (bytesOfKeyValueData != 0) {
        if (keyValueData == NULL)
            return KTX_INVALID_OPERATION;

        result = stream->write(stream, keyValueData, 1, bytesOfKeyValueData);
        if (result != KTX_SUCCESS)
            return result;
    }

    /* Write the image data */
    for (level = 0, i = 0; level < numMipmapLevels; ++level)
    {
        GLuint faceSlice, faceLodSize;
#if (KTX_GL_UNPACK_ALIGNMENT != 4)
        GLuint faceLodPadding;
#endif
        GLuint pixelWidth, pixelHeight, pixelDepth;
        GLsizei imageBytes, packedImageBytes;
        GLsizei packedRowBytes, rowBytes, rowPadding;
        GLuint numImages;

        pixelWidth  = MAX(1, header.pixelWidth  >> level);
        pixelHeight = MAX(1, header.pixelHeight >> level);
        pixelDepth  = MAX(1, header.pixelDepth  >> level);

        /* Calculate face sizes for this LoD based on glType, glFormat, width & height */
        packedImageBytes = groupBytes
                           * pixelWidth
                           * pixelHeight;

        rowPadding = 0;
        packedRowBytes = groupBytes * pixelWidth;
        /* KTX format specifies UNPACK_ALIGNMENT==4 */
        /* GL spec: rows are not to be padded when elementBytes != 1, 2, 4 or 8.
         * As GL currently has no such elements, no test is necessary.
         */
        if (!compressed) {
            rowBytes = _KTX_PAD_UNPACK_ALIGN(packedRowBytes);
            rowPadding = rowBytes - packedRowBytes;
        }
        if (rowPadding == 0) {
            imageBytes = packedImageBytes;
        } else {
            /* Need to pad the rows to meet the required UNPACK_ALIGNMENT */
            imageBytes = rowBytes * pixelHeight;
        }

        if (textureInfo->numberOfArrayElements == 0 && cubemap) {
            /* Non-array cubemap. */
            numImages = 6;
            faceLodSize = imageBytes;
        } else {
            numImages = cubemap ? 6 : pixelDepth;
            numImages *= numArrayElements;
            faceLodSize = imageBytes * numImages;
        }
#if (KTX_GL_UNPACK_ALIGNMENT != 4)
        faceLodPadding = _KTX_PAD4_LEN(faceLodSize);
#endif
 
        result = stream->write(stream, &faceLodSize, sizeof(faceLodSize), 1);
        if (result != KTX_SUCCESS)
            goto cleanup;

        for (faceSlice = 0; faceSlice < numImages; ++faceSlice, ++i) {
            if (!compressed) {
                /* Sanity check. */
                if (images[i].size != packedImageBytes) {
                    result = KTX_INVALID_OPERATION;
                    goto cleanup;
                }
            }
            if (rowPadding == 0) {
                /* Can write whole face at once */
                result = stream->write(stream, images[i].data, images[i].size,
                                       1);
                if (result != KTX_SUCCESS)
                    goto cleanup;
            } else {
                /* Write the rows individually, padding each one */
                GLuint row;
                GLuint numRows = pixelHeight;
                for (row = 0; row < numRows; row++) {
                    result = stream->write(stream,
                                            &images[i].data[row*packedRowBytes],
                                            packedRowBytes, 1);
                    if (result != KTX_SUCCESS)
                        goto cleanup;

                    result = stream->write(stream, pad, sizeof(GLbyte),
                                              rowPadding);
                    if (result != KTX_SUCCESS)
                        goto cleanup;
                }
            }
#if (KTX_GL_UNPACK_ALIGNMENT != 4)
            /*
             * When KTX_GL_UNPACK_ALIGNMENT == 4, rows, and therefore everything
             * else, are always 4-byte aligned and faceLodPadding is always 0.
             * It is always 0 for compressed formats too because they all have
             * multiple-of-4 block sizes.
             */
            if (faceLodPadding) {
                result = stream->write(stream, pad, sizeof(GLbyte),
                                       faceLodPadding);
                if (result != KTX_SUCCESS)
                    goto cleanup;
            }
#endif
        }
    }

cleanup:
    return result;
}

/**
 * @~English
 * @ingroup writer
 * @deprecated Use ktxTexture_WriteToStdioStream().
 * @brief Write image(s) in KTX format to a stdio FILE stream.
 *
 * @note textureInfo directly reflects what is written to the KTX file
 *       header. That is @c numberOfArrayElements should be 0 for non arrays;
 *       @c numMipmapLevels should be 0 to request generateMipmaps and @c type,
 *       @c format & @c typesize should be 0 for compressed textures.
 *
 * @param[in] file         pointer to the FILE stream to write to.
 * @param[in] textureInfo  pointer to a KTX_texture_info structure providing
 *                         information about the images to be included in
 *                         the KTX file.
 * @param[in] bytesOfKeyValueData
 *                         specifies the number of bytes of key-value data.
 * @param[in] keyValueData a pointer to the keyValue data.
 * @param[in] numImages    number of images in the following array
 * @param[in] images       array of KTX_image_info providing image size and
 *                         data.
 *
 * @return      KTX_SUCCESS on success, other KTX_* enum values on error.
 *
 * @exception KTX_INVALID_VALUE @c glTypeSize in @p textureInfo is not 1, 2, or
 *                              4 or is different from the size of the type
 *                              specified in @c glType.
 * @exception KTX_INVALID_VALUE @c pixelWidth in @p textureInfo is 0 or
 *                              pixelDepth != 0 && pixelHeight == 0.
 * @exception KTX_INVALID_VALUE In @p textureInfo, @c numberOfFaces != 1 or
 *                              numberOfFaces != 6 or numberOfArrayElements
 *                              or numberOfMipmapLevels are < 0.
 * @exception KTX_INVALID_VALUE @c glType in @p textureInfo is an unrecognized
 *                              type.
 * @exception KTX_INVALID_OPERATION
 *                              In @p textureInfo, numberOfFaces == 6 and
 *                              images are either not 2D or are not square.
 * @exception KTX_INVALID_OPERATION
 *                              @p numImages is insufficient for the specified
 *                              number of mipmap levels and faces.
 * @exception KTX_INVALID_OPERATION
 *                              The size of a provided image is different than
 *                              that required for the specified width, height
 *                              or depth or for the mipmap level being
 *                              processed.
 * @exception KTX_INVALID_OPERATION
 *                              @c glType and @c glFormat in @p textureInfo are
 *                              mismatched. See OpenGL 4.4 specification
 *                              section 8.4.4 and table 8.5.
 * @exception KTX_FILE_WRITE_ERROR
 *                              A system error occurred while writing the file.
 * @exception KTX_OUT_OF_MEMORY System failed to allocate sufficient memory.
 */
KTX_error_code
ktxWriteKTXF(FILE *file, const KTX_texture_info* textureInfo,
                         GLsizei bytesOfKeyValueData, const void* keyValueData,
                         GLuint numImages, KTX_image_info images[])
{
    struct ktxStream stream;
    KTX_error_code result;
    
    result = ktxFileStream_construct(&stream, file, KTX_FALSE);
    if (result != KTX_SUCCESS)
        return result;
    result = ktxWriteKTXS(&stream, textureInfo, bytesOfKeyValueData, keyValueData,
                          numImages, images);
    stream.destruct(&stream);
    return result;
}

/**
 * @~English
 * @ingroup writer
 * @deprecated Use ktxTexture_WriteToNamedFile().
 * @brief Write image(s) in KTX format to a file on disk.
 *
 * @note textureInfo directly reflects what is written to the KTX file
 *       header. That is @c numberOfArrayElements should be 0 for non arrays;
 *       @c numMipmapLevels should be 0 to request generateMipmaps and @c type,
 *       @c format & @c typesize should be 0 for compressed textures.
 *
 * @param[in] dstname      pointer to a C string that contains the path of
 *                         the file to load.
 * @param[in] textureInfo  pointer to a KTX_texture_info structure providing
 *                         information about the images to be included in
 *                         the KTX file.
 * @param[in] bytesOfKeyValueData
 *                         specifies the number of bytes of key-value data.
 * @param[in] keyValueData a pointer to the keyValue data.
 * @param[in] numImages    number of images in the following array.
 * @param[in] images       array of KTX_image_info providing image size and
 *                         data.
 *
 * @return  KTX_SUCCESS on success, other KTX_* enum values on error.
 *
 * @exception KTX_FILE_OPEN_FAILED Unable to open the specified file for
 *                                 writing.
 *
 * For other exceptions, see ktxWriteKTXF().
 */
KTX_error_code
ktxWriteKTXN(const char* dstname, const KTX_texture_info* textureInfo,
             GLsizei bytesOfKeyValueData, const void* keyValueData,
             GLuint numImages, KTX_image_info images[])
{
    struct ktxStream stream;
    KTX_error_code result;
    FILE* dst = fopen(dstname, "wb");

    if (dst) {
        result = ktxWriteKTXS(&stream, textureInfo, bytesOfKeyValueData,
                              keyValueData, numImages, images);
        fclose(dst);
    } else
        result = KTX_FILE_OPEN_FAILED;

    return result;
}

/**
 * @~English
 * @ingroup writer
 * @deprecated Use ktxTexture_WriteToMemory().
 * @brief Write image(s) in KTX format to memory.
 *
 * Memory is allocated by the function and the caller is responsible for
 * freeing it.
 *
 * @note textureInfo directly reflects what is written to the KTX file
 *       header. That is @c numberOfArrayElements should be 0 for non arrays;
 *       @c numMipmapLevels should be 0 to request generateMipmaps and @c type,
 *       @c format & @c typesize should be 0 for compressed textures.
 *
 * @param[out] ppDstBytes  pointer to location to write the address of
 *                         the destination memory. The Application is
                           responsible for freeing this memory.
 * @param[out] pSize       pointer to location to write the size in bytes of
 *                         the KTX data.
 * @param[in] textureInfo  pointer to a KTX_texture_info structure providing
 *                         information about the images to be included in
 *                         the KTX file.
 * @param[in] bytesOfKeyValueData
 *                         specifies the number of bytes of key-value data.
 * @param[in] keyValueData a pointer to the keyValue data.
 * @param[in] numImages    number of images in the following array.
 * @param[in] images       array of KTX_image_info providing image size and
 *                         data.
 *
 * @return      KTX_SUCCESS on success, other KTX_* enum values on error.
 *
 * @exception KTX_INVALID_VALUE @p dst or @p size is NULL.
 * @exception KTX_INVALID_VALUE @c glTypeSize in @p textureInfo is not 1, 2, or
 *                              4 or is different from the size of the type
 *                              specified in @c glType.
 * @exception KTX_INVALID_VALUE @c pixelWidth in @p textureInfo is 0 or
 *                              pixelDepth != 0 && pixelHeight == 0.
 * @exception KTX_INVALID_VALUE In @p textureInfo, @c numberOfFaces != 1 or
 *                              numberOfFaces != 6 or numberOfArrayElements
 *                              or numberOfMipmapLevels are < 0.
 * @exception KTX_INVALID_VALUE @c glType in @p textureInfo is an unrecognized
 *                              type.
 * @exception KTX_INVALID_OPERATION
 *                              In @p textureInfo, numberOfFaces == 6 and
 *                              images are either not 2D or are not square.
 * @exception KTX_INVALID_OPERATION
 *                              @p numImages is insufficient for the specified
 *                              number of mipmap levels and faces.
 * @exception KTX_INVALID_OPERATION
 *                              The size of a provided image is different than
 *                              that required for the specified width, height
 *                              or depth or for the mipmap level being
 *                              processed.
 * @exception KTX_INVALID_OPERATION
 *                              @c glType and @c glFormat in @p textureInfo are
 *                              mismatched. See OpenGL 4.4 specification
 *                              section 8.4.4 and table 8.5.
 * @exception KTX_FILE_WRITE_ERROR
 *                              A system error occurred while writing the file.
 * @exception KTX_OUT_OF_MEMORY System failed to allocate sufficient memory.
 */
KTX_error_code
ktxWriteKTXM(unsigned char** ppDstBytes, GLsizei* pSize,
             const KTX_texture_info* textureInfo,
             GLsizei bytesOfKeyValueData, const void* keyValueData,
             GLuint numImages, KTX_image_info images[])
{
    struct ktxStream stream;
    KTX_error_code rc;
    ktx_size_t strSize;

    if (ppDstBytes == NULL || pSize == NULL)
        return KTX_INVALID_VALUE;

    *ppDstBytes = NULL;

    rc = ktxMemStream_construct(&stream, KTX_FALSE);
    if (rc != KTX_SUCCESS)
        return rc;

    rc = ktxWriteKTXS(&stream, textureInfo, bytesOfKeyValueData, keyValueData,
                      numImages, images);
    if(rc != KTX_SUCCESS)
    {
        ktxMemStream_destruct(&stream);
        return rc;
    }

    ktxMemStream_getdata(&stream, ppDstBytes);
    stream.getsize(&stream, &strSize);
    *pSize = (GLsizei)strSize;
    /* This function does not free the memory pointed at by the
     * value obtained from ktxMemStream_getdata().
     */
    stream.destruct(&stream);
    return KTX_SUCCESS;
}

/**
 * @internal
 * @~English
 * @brief Get the matching format for an internalformat.
 *
 * Adds support for deprecated legacy formats needed to create textures for
 * use with OpenGL ES 1 & 2 to glGetFormatFromInternalFormat.
 *
 * @param[in] internalFormat  the internal format of the image data
 *
 * @return    the matching glFormat enum or GL_INVALID_VALUE if format is
 *            unrecognized.
 */
GLenum
getFormatFromInternalFormatLegacy(GLenum internalFormat)
{
    switch (internalFormat) {
      case GL_LUMINANCE8:
      case GL_LUMINANCE16:
        return GL_LUMINANCE;
            
      case GL_ALPHA8:
      case GL_ALPHA16:
        return GL_ALPHA;
            
      case GL_LUMINANCE8_ALPHA8:
      case GL_LUMINANCE16_ALPHA16:
        return GL_LUMINANCE_ALPHA;

      default:
        return glGetFormatFromInternalFormat(internalFormat);
    }
}

/**
 * @internal
 * @~English
 * @brief Get the GL data type for an internalformat.
 *
 * Adds support for deprecated legacy formats needed to create textures for
 * use with OpenGL ES 1 & 2 to glGetTypeFromInternalFormat.
 *
 * @param[in] internalFormat  the internal format of the image data
 *
 * @return    the matching glFormat enum or GL_INVALID_VALUE if format is
 *            unrecognized.
 */
GLenum
getTypeFromInternalFormatLegacy(GLenum internalFormat)
{
    switch (internalFormat) {
      case GL_LUMINANCE8:
      case GL_ALPHA8:
        return GL_UNSIGNED_BYTE;
            
      case GL_LUMINANCE16:
      case GL_ALPHA16:
      case GL_LUMINANCE8_ALPHA8:
        return GL_UNSIGNED_SHORT;
            
      case GL_LUMINANCE16_ALPHA16:
        return GL_UNSIGNED_INT;
            
      default:
        return glGetTypeFromInternalFormat(internalFormat);
    }
}
        
/**
 * @internal
 * @~English
 * @brief Get size information for an internalformat.
 *
 * Adds support for deprecated legacy formats needed to create textures for
 * use with OpenGL ES 1 & 2 to glGetTypeFromInternalFormat.
 *
 * @param[in] internalFormat  the internal format of the image data
 * @param[in,out] formatSize  pointer to a formatSize struct in which the
 *                            information is returned.
 */
void
getFormatSizeLegacy(GLenum internalFormat, GlFormatSize* pFormatSize)
{
    switch (internalFormat) {
      case GL_LUMINANCE8:
      case GL_ALPHA8:
        pFormatSize->flags = 0;
        pFormatSize->paletteSizeInBits = 0;
        pFormatSize->blockSizeInBits = 1 * 8;
        pFormatSize->blockWidth = 1;
        pFormatSize->blockHeight = 1;
        pFormatSize->blockDepth = 1;
        break;

      case GL_LUMINANCE16:
      case GL_ALPHA16:
      case GL_LUMINANCE8_ALPHA8:
        pFormatSize->flags = 0;
        pFormatSize->paletteSizeInBits = 0;
        pFormatSize->blockSizeInBits = 2 * 8;
        pFormatSize->blockWidth = 1;
        pFormatSize->blockHeight = 1;
        pFormatSize->blockDepth = 1;
        break;

      case GL_LUMINANCE16_ALPHA16:
        pFormatSize->flags = 0;
        pFormatSize->paletteSizeInBits = 0;
        pFormatSize->blockSizeInBits = 4 * 8;
        pFormatSize->blockWidth = 1;
        pFormatSize->blockHeight = 1;
        pFormatSize->blockDepth = 1;
        break;

      default:
        glGetFormatSize(internalFormat, pFormatSize);
    }
}
