/* -*- tab-width: 4; -*- */
/* vi: set sw=2 ts=4 expandtab: */

/*
 * ©2018 Mark Callow.
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
 * @brief ktxTexture implementation.
 *
 * @author Mark Callow, www.edgewise-consulting.com
 */

#ifdef _WIN32
#define _CRT_SECURE_NO_WARNINGS
#endif

#include <stdlib.h>

#include "ktx.h"
#include "ktxint.h"
#include "stream.h"
#include "filestream.h"
#include "memstream.h"
#include "gl_format.h"
#include "uthash.h"
#include <math.h>

/**
 * @internal
 * @~English
 * @brief Internal ktxTexture structure.
 *
 * This is kept hidden to avoid burdening applications with the definitions
 * of GlFormatSize and ktxStream.
 */
typedef struct _ktxTextureInt {
    ktxTexture super;         /*!< Base ktxTexture class. */
    GlFormatSize formatInfo;  /*!< Info about the image data format. */
    // The following are needed because image data reading can be delayed.
    ktx_uint32_t glTypeSize;  /*!< Size of the image data type in bytes. */
    ktxStream stream;         /*!< Stream connected to KTX source. */
    ktx_bool_t needSwap;   /*!< If KTX_TRUE, image data needs byte swapping. */
} ktxTextureInt;

ktx_size_t ktxTexture_GetSize(ktxTexture* This);
KTX_error_code ktxTexture_LoadImageData(ktxTexture* This,
                                        ktx_uint8_t* pBuffer,
                                        ktx_size_t bufSize);

static ktx_size_t ktxTexture_calcDataSize(ktxTexture* This);
static ktx_uint32_t padRow(ktx_uint32_t* rowBytes);


/**
 * @memberof ktxTexture @private
 * @brief Construct (initialize) a ktxTexture.
 *
 * @param[in] This pointer to a ktxTextureInt-sized block of memory to
 *                 initialize.
 * @param[in] createInfo pointer to a ktxTextureCreateInfo struct with
 *                       information describing the texture.
 * @param[in] storageAllocation
 *                       enum indicating whether or not to allocation storage
 *                       for the texture images.
 *
 * @return      KTX_SUCCESS on success, other KTX_* enum values on error.
 *
 * @exception KTX_INVALID_VALUE @c glInternalFormat in @p createInfo is not a
 *                              valid OpenGL internal format value.
 * @exception KTX_INVALID_VALUE @c numDimensions in @p createInfo is not 1, 2
 *                              or 3.
 * @exception KTX_INVALID_VALUE One of <tt>base{Width,Height,Depth}</tt> in
 *                              @p createInfo is 0.
 * @exception KTX_INVALID_VALUE @c numFaces in @p createInfo is not 1 or 6.
 * @exception KTX_INVALID_VALUE @c numLevels in @p createInfo is 0.
 * @exception KTX_INVALID_OPERATION
 *                              The <tt>base{Width,Height,Depth}</tt> specified
 *                              in @p createInfo are inconsistent with
 *                              @c numDimensions.
 * @exception KTX_INVALID_OPERATION
 *                              @p createInfo is requesting a 3D array or
 *                              3D cubemap texture.
 * @exception KTX_INVALID_OPERATION
 *                              @p createInfo is requesting a cubemap with
 *                              non-square or non-2D images.
 * @exception KTX_INVALID_OPERATION
 *                              @p createInfo is requesting more mip levels
 *                              than needed for the specified
 *                              <tt>base{Width,Height,Depth}</tt>.
 * @exception KTX_OUT_OF_MEMORY Not enough memory for the texture's images.
 */
static KTX_error_code
ktxTextureInt_construct(ktxTextureInt* This, ktxTextureCreateInfo* createInfo,
                        ktxTextureCreateStorageEnum storageAllocation)
{
    ktxTexture* super = (ktxTexture*)This;
    GLuint typeSize;
    GLenum glFormat;
    
    memset(This, 0, sizeof(*This));

    super->glInternalformat = createInfo->glInternalformat;
    glGetFormatSize(super->glInternalformat, &This->formatInfo);;
    glFormat= glGetFormatFromInternalFormat(createInfo->glInternalformat);
    if (glFormat == GL_INVALID_VALUE)
        return KTX_INVALID_VALUE;
    super->isCompressed
                    = (This->formatInfo.flags & GL_FORMAT_SIZE_COMPRESSED_BIT);
    if (super->isCompressed) {
        super->glFormat = 0;
        super->glBaseInternalformat = glFormat;
        super->glType = 0;
        This->glTypeSize = 0;
    } else {
        super->glBaseInternalformat = super->glFormat = glFormat;
        super->glType
                = glGetTypeFromInternalFormat(createInfo->glInternalformat);
        if (super->glType == GL_INVALID_VALUE)
            return KTX_INVALID_VALUE;
        typeSize = glGetTypeSizeFromType(super->glType);
        assert(typeSize != GL_INVALID_VALUE);
    
        /* Do some sanity checking */
        if (typeSize != 1 &&
            typeSize != 2 &&
            typeSize != 4)
        {
            /* Only 8, 16, and 32-bit types are supported for byte-swapping.
             * See UNPACK_SWAP_BYTES & table 8.4 in the OpenGL 4.4 spec.
             */
            return KTX_INVALID_VALUE;
        }
        This->glTypeSize = typeSize;
    }
    
    /* Check texture dimensions. KTX files can store 8 types of textures:
     * 1D, 2D, 3D, cube, and array variants of these.
     */
    if (createInfo->numDimensions < 1 || createInfo->numDimensions > 3)
        return KTX_INVALID_VALUE;

    if (createInfo->baseWidth == 0 || createInfo->baseHeight == 0
        || createInfo->baseDepth == 0)
        return KTX_INVALID_VALUE;

    super->baseWidth = createInfo->baseWidth;
    switch (createInfo->numDimensions) {
      case 1:
        if (createInfo->baseHeight > 1 || createInfo->baseDepth > 1)
            return KTX_INVALID_OPERATION;
        break;
            
      case 2:
        if (createInfo->baseDepth > 1)
            return KTX_INVALID_OPERATION;
        super->baseHeight = createInfo->baseHeight;
        break;
            
      case 3:
        /* 3D array textures and 3D cubemaps are not supported by either
         * OpenGL or Vulkan.
         */
        if (createInfo->isArray || createInfo->numFaces != 1
            || createInfo->numLayers != 1)
            return KTX_INVALID_OPERATION;
        super->baseDepth = createInfo->baseDepth;
        super->baseHeight = createInfo->baseHeight;
        break;
    }
    super->numDimensions = createInfo->numDimensions;

    if (createInfo->numLayers == 0)
        return KTX_INVALID_VALUE;
    super->numLayers = createInfo->numLayers;

    if (createInfo->numFaces == 6) {
        if (super->numDimensions != 2) {
            /* cube map needs 2D faces */
            return KTX_INVALID_OPERATION;
        }
        if (createInfo->baseWidth != createInfo->baseHeight) {
            /* cube maps require square images */
            return KTX_INVALID_OPERATION;
        }
        super->isCubemap = KTX_TRUE;
    } else if (createInfo->numFaces != 1) {
        /* numFaces must be either 1 or 6 */
        return KTX_INVALID_VALUE;
    }
    super->numFaces = createInfo->numFaces;


    /* Check number of mipmap levels */
    if (createInfo->numLevels == 0)
        return KTX_INVALID_VALUE;
    super->numLevels = createInfo->numLevels;
    super->generateMipmaps = createInfo->generateMipmaps;
 
    if (createInfo->numLevels > 1) {
        GLuint max_dim = MAX(MAX(createInfo->baseWidth, createInfo->baseHeight), createInfo->baseDepth);
        if (max_dim < ((GLuint)1 << (super->numLevels - 1)))
        {
            /* Can't have more mip levels than 1 + log2(max(width, height, depth)) */
            return KTX_INVALID_OPERATION;
        }
    }
    
    super->numLayers = createInfo->numLayers;
    super->isArray = createInfo->isArray;
    
    ktxHashList_Construct(&super->kvDataHead);
    if (storageAllocation == KTX_TEXTURE_CREATE_ALLOC_STORAGE) {
        super->dataSize = ktxTexture_calcDataSize(super);
        super->pData = malloc(super->dataSize);
        if (super->pData == NULL)
            return KTX_OUT_OF_MEMORY;
    }
    return KTX_SUCCESS;
}

/**
 * @memberof ktxTexture @private
 * @brief Construct a ktxTexture from a ktxStream reading from a KTX source.
 *
 * The caller constructs the stream inside the ktxTextureInt before calling
 * this.
 *
 * The create flag KTX_TEXTURE_CREATE_LOAD_IMAGE_DATA_BIT should not be set,
 * if the ktxTexture is ultimately to be uploaded to OpenGL or Vulkan. This
 * will minimize memory usage by allowing, for example, loading the images
 * directly from the source into a Vulkan staging buffer.
 *
 * The create flag KTX_TEXTURE_CREATE_RAW_KVDATA_BIT should not be used. It is
 * provided solely to enable implementation of the @e libktx v1 API on top of
 * ktxTexture.
 *
 * @param[in] This pointer to a ktxTextureInt-sized block of memory to
 *                 initialize.
 * @param[in] createFlags bitmask requesting specific actions during creation.
 *
 * @return      KTX_SUCCESS on success, other KTX_* enum values on error.
 *
 * @exception KTX_FILE_DATA_ERROR
 *                              Source data is inconsistent with the KTX
 *                              specification.
 * @exception KTX_FILE_READ_ERROR
 *                              An error occurred while reading the source.
 * @exception KTX_FILE_UNEXPECTED_EOF
 *                              Not enough data in the source.
 * @exception KTX_OUT_OF_MEMORY Not enough memory to load either the images or
 *                              the key-value data.
 * @exception KTX_UNKNOWN_FILE_FORMAT
 *                              The source is not in KTX format.
 * @exception KTX_UNSUPPORTED_TEXTURE_TYPE
 *                              The source describes a texture type not
 *                              supported by OpenGL or Vulkan, e.g, a 3D array.
 */
static KTX_error_code
ktxTextureInt_constructFromStream(ktxTextureInt* This,
                                  ktxTextureCreateFlags createFlags)
{
    ktxTexture* super = (ktxTexture*)This;
    KTX_error_code result;
    KTX_header header;
    KTX_supplemental_info suppInfo;
    ktxStream* stream;
    ktx_off_t pos;
    ktx_size_t size;
    
    assert(This != NULL);
    assert(This->stream.data.mem != NULL);
    assert(This->stream.type == eStreamTypeFile
           || This->stream.type == eStreamTypeMemory);
    stream = &This->stream;
  
    // Read header.
    result = stream->read(stream, &header, KTX_HEADER_SIZE);
    if (result != KTX_SUCCESS)
        return result;
    
    result = _ktxCheckHeader(&header, &suppInfo);
    if (result != KTX_SUCCESS)
        return result;
    
    /*
     * Initialize from header info.
     */
    super->glFormat = header.glFormat;
    super->glInternalformat = header.glInternalformat;
    super->glType = header.glType;
    glGetFormatSize(super->glInternalformat, &This->formatInfo);
    super->glBaseInternalformat = header.glBaseInternalformat;
    super->numDimensions = suppInfo.textureDimension;
    super->baseWidth = header.pixelWidth;
    assert(suppInfo.textureDimension > 0 && suppInfo.textureDimension < 4);
    switch (suppInfo.textureDimension) {
      case 1:
        super->baseHeight = super->baseDepth = 1;
        break;
      case 2:
        super->baseHeight = header.pixelHeight;
        super->baseDepth = 1;
        break;
      case 3:
        super->baseHeight = header.pixelHeight;
        super->baseDepth = header.pixelDepth;
        break;
    }
    if (header.numberOfArrayElements > 0) {
        super->numLayers = header.numberOfArrayElements;
        super->isArray = KTX_TRUE;
    } else {
        super->numLayers = 1;
        super->isArray = KTX_FALSE;
    }
    super->numFaces = header.numberOfFaces;
    if (header.numberOfFaces == 6)
        super->isCubemap = KTX_TRUE;
    else
        super->isCubemap = KTX_FALSE;
    super->numLevels = header.numberOfMipmapLevels;
    super->isCompressed = suppInfo.compressed;
    super->generateMipmaps = suppInfo.generateMipmaps;
    if (header.endianness == KTX_ENDIAN_REF_REV)
        This->needSwap = KTX_TRUE;
    This->glTypeSize = header.glTypeSize;

    /*
     * Make an empty hash list.
     */
    ktxHashList_Construct(&super->kvDataHead);
    /*
     * Load KVData.
     */
    if (header.bytesOfKeyValueData > 0) {
        if (!(createFlags & KTX_TEXTURE_CREATE_SKIP_KVDATA_BIT)) {
            ktx_uint32_t kvdLen = header.bytesOfKeyValueData;
            ktx_uint8_t* pKvd;

            pKvd = malloc(kvdLen);
            if (pKvd == NULL)
                return KTX_OUT_OF_MEMORY;
            
            result = stream->read(stream, pKvd, kvdLen);
            if (result != KTX_SUCCESS)
                return result;

            if (This->needSwap) {
                /* Swap the counts inside the key & value data. */
                ktx_uint8_t* src = pKvd;
                ktx_uint8_t* end = pKvd + kvdLen;
                while (src < end) {
                    ktx_uint32_t keyAndValueByteSize = *((ktx_uint32_t*)src);
                    _ktxSwapEndian32(&keyAndValueByteSize, 1);
                    src += _KTX_PAD4(keyAndValueByteSize);
                }
            }
            
            if (!(createFlags & KTX_TEXTURE_CREATE_RAW_KVDATA_BIT)) {
                result = ktxHashList_Deserialize(&super->kvDataHead,
                                                 kvdLen, pKvd);
                if (result != KTX_SUCCESS) {
                    free(pKvd);
                    return result;
                }
            } else {
                super->kvDataLen = kvdLen;
                super->kvData = pKvd;
            }
        } else {
            stream->skip(stream, header.bytesOfKeyValueData);
        }
    }
    
    /*
     * Get the size of the image data.
     */
    result = stream->getsize(stream, &size);
    if (result == KTX_SUCCESS) {
        result = stream->getpos(stream, &pos);
        if (result == KTX_SUCCESS)
            super->dataSize = size - pos
                                 /* Remove space for faceLodSize fields */
                                 - super->numLevels * sizeof(ktx_uint32_t);
    }

    /*
     * Load the images, if requested.
     */
    if (result == KTX_SUCCESS
        && (createFlags & KTX_TEXTURE_CREATE_LOAD_IMAGE_DATA_BIT)) {
        result = ktxTexture_LoadImageData((ktxTexture*)super, NULL, 0);
    }
    return result;
}

/**
 * @memberof ktxTexture @private
 * @brief Construct a ktxTexture from a stdio stream reading from a KTX source.
 *
 * See ktxTextureInt_constructFromStream for details.
 *
 * @note Do not close the stdio stream until you are finished with the texture
 *       object.
 *
 * @param[in] This pointer to a ktxTextureInt-sized block of memory to
 *                 initialize.
 * @param[in] stdioStream a stdio FILE pointer opened on the source.
 * @param[in] createFlags bitmask requesting specific actions during creation.
 *
 * @return      KTX_SUCCESS on success, other KTX_* enum values on error.
 *
 * @exception KTX_INVALID_VALUE Either @p stdiostream or @p This is null.
 *
 * For other exceptions, see ktxTexture_constructFromStream().
 */
static KTX_error_code
ktxTextureInt_constructFromStdioStream(ktxTextureInt* This, FILE* stdioStream,
                                       ktxTextureCreateFlags createFlags)
{
    KTX_error_code result;

    if (stdioStream == NULL || This == NULL)
        return KTX_INVALID_VALUE;
    
    memset(This, 0, sizeof(*This));
    
    result = ktxFileStream_construct(&This->stream, stdioStream, KTX_FALSE);
    if (result == KTX_SUCCESS)
        result = ktxTextureInt_constructFromStream(This, createFlags);
    return result;
}

/**
 * @memberof ktxTexture @private
 * @brief Construct a ktxTexture from a named KTX file.
 *
 * See ktxTextureInt_constructFromStream for details.
 *
 * @param[in] This pointer to a ktxTextureInt-sized block of memory to
 *                 initialize.
 * @param[in] filename    pointer to a char array containing the file name.
 * @param[in] createFlags bitmask requesting specific actions during creation.
 *
 * @return      KTX_SUCCESS on success, other KTX_* enum values on error.
 *
 * @exception KTX_FILE_OPEN_FAILED The file could not be opened.
 * @exception KTX_INVALID_VALUE @p filename is @c NULL.
 *
 * For other exceptions, see ktxTexture_constructFromStream().
 */
static KTX_error_code
ktxTextureInt_constructFromNamedFile(ktxTextureInt* This,
                                     const char* const filename,
                                     ktxTextureCreateFlags createFlags)
{
    KTX_error_code result;
    FILE* file;
    
    if (This == NULL || filename == NULL)
        return KTX_INVALID_VALUE;
    
    memset(This, 0, sizeof(*This));

    file = fopen(filename, "rb");
    if (!file)
       return KTX_FILE_OPEN_FAILED;
    
    result = ktxFileStream_construct(&This->stream, file, KTX_TRUE);
    if (result == KTX_SUCCESS)
        result = ktxTextureInt_constructFromStream(This, createFlags);
    
    return result;
}

/**
 * @memberof ktxTexture @private
 * @brief Construct a ktxTexture from KTX-formatted data in memory.
 *
 * See ktxTextureInt_constructFromStream for details.
 *
 * @param[in] This  pointer to a ktxTextureInt-sized block of memory to
 *                  initialize.
 * @param[in] bytes pointer to the memory containing the serialized KTX data.
 * @param[in] size  length of the KTX data in bytes.
 * @param[in] createFlags bitmask requesting specific actions during creation.
 *
 * @return      KTX_SUCCESS on success, other KTX_* enum values on error.
 *
 * @exception KTX_INVALID_VALUE Either @p bytes is NULL or @p size is 0.
 *
 * For other exceptions, see ktxTexture_constructFromStream().
 */
static KTX_error_code
ktxTextureInt_constructFromMemory(ktxTextureInt* This,
                                  const ktx_uint8_t* bytes, ktx_size_t size,
                                  ktxTextureCreateFlags createFlags)
{
    KTX_error_code result;
    
    if (bytes == NULL || size == 0)
        return KTX_INVALID_VALUE;
    
    memset(This, 0, sizeof(*This));
    
    result = ktxMemStream_construct_ro(&This->stream, bytes, size);
    if (result == KTX_SUCCESS)
        result = ktxTextureInt_constructFromStream(This, createFlags);
    
    return result;
}

/**
 * @memberof ktxTexture @private
 * @~English
 * @brief Free the memory associated with the texture contents
 *
 * @param[in] This pointer to the ktxTextureInt whose texture contents are
 *                 to be freed.
 */
void
ktxTextureInt_destruct(ktxTextureInt* This)
{
    ktxTexture* super = (ktxTexture*)This;
    if (This->stream.data.file != NULL)
        This->stream.destruct(&This->stream);
    if (super->kvDataHead != NULL)
        ktxHashList_Destruct(&super->kvDataHead);
    if (super->kvData != NULL)
        free(super->kvData);
    if (super->pData != NULL)
        free(super->pData);
}

/**
 * @memberof ktxTexture
 * @ingroup writer
 * @brief Create a new empty ktxTexture.
 *
 * The address of the newly created ktxTexture is written to the location
 * pointed at by @p newTex.
 *
 * @param[in] createInfo pointer to a ktxTextureCreateInfo struct with
 *                       information describing the texture.
 * @param[in] storageAllocation
 *                       enum indicating whether or not to allocate storage
 *                       for the texture images.
 * @param[in,out] newTex pointer to a location in which store the address of
 *                       the newly created texture.
 *
 * @return      KTX_SUCCESS on success, other KTX_* enum values on error.
 *
 * @exception KTX_INVALID_VALUE @c glInternalFormat in @p createInfo is not a
 *                              valid OpenGL internal format value.
 * @exception KTX_INVALID_VALUE @c numDimensions in @p createInfo is not 1, 2
 *                              or 3.
 * @exception KTX_INVALID_VALUE One of <tt>base{Width,Height,Depth}</tt> in
 *                              @p createInfo is 0.
 * @exception KTX_INVALID_VALUE @c numFaces in @p createInfo is not 1 or 6.
 * @exception KTX_INVALID_VALUE @c numLevels in @p createInfo is 0.
 * @exception KTX_INVALID_OPERATION
 *                              The <tt>base{Width,Height,Depth}</tt> specified
 *                              in @p createInfo are inconsistent with
 *                              @c numDimensions.
 * @exception KTX_INVALID_OPERATION
 *                              @p createInfo is requesting a 3D array or
 *                              3D cubemap texture.
 * @exception KTX_INVALID_OPERATION
 *                              @p createInfo is requesting a cubemap with
 *                              non-square or non-2D images.
 * @exception KTX_INVALID_OPERATION
 *                              @p createInfo is requesting more mip levels
 *                              than needed for the specified
 *                              <tt>base{Width,Height,Depth}</tt>.
 * @exception KTX_OUT_OF_MEMORY Not enough memory for the texture's images.
 */
KTX_error_code
ktxTexture_Create(ktxTextureCreateInfo* createInfo,
                  ktxTextureCreateStorageEnum storageAllocation,
                  ktxTexture** newTex)
{
    KTX_error_code result;
    
    if (newTex == NULL)
        return KTX_INVALID_VALUE;
    
    ktxTextureInt* tex = (ktxTextureInt*)malloc(sizeof(ktxTextureInt));
    if (tex == NULL)
        return KTX_OUT_OF_MEMORY;
    
    result = ktxTextureInt_construct(tex, createInfo, storageAllocation);
    if (result == KTX_SUCCESS)
        *newTex = (ktxTexture*)tex;
    else {
        free(tex);
        *newTex = NULL;
    }
    return result;
}

/**
 * @defgroup reader Reader
 * @brief Read KTX-formatted data.
 * @{
 */

/**
 * @memberof ktxTexture
 * @~English
 * @brief Create a ktxTexture from a stdio stream reading from a KTX source.
 *
 * The address of a newly created ktxTexture reflecting the contents of the
 * stdio stream is written to the location pointed at by @p newTex.
 *
 * The create flag KTX_TEXTURE_CREATE_LOAD_IMAGE_DATA_BIT should not be set,
 * if the ktxTexture is ultimately to be uploaded to OpenGL or Vulkan. This
 * will minimize memory usage by allowing, for example, loading the images
 * directly from the source into a Vulkan staging buffer.
 *
 * The create flag KTX_TEXTURE_CREATE_RAW_KVDATA_BIT should not be used. It is
 * provided solely to enable implementation of the @e libktx v1 API on top of
 * ktxTexture.
 *
 * @param[in] stdioStream stdio FILE pointer created from the desired file.
 * @param[in] createFlags bitmask requesting specific actions during creation.
 * @param[in,out] newTex  pointer to a location in which store the address of
 *                        the newly created texture.
 *
 * @return      KTX_SUCCESS on success, other KTX_* enum values on error.
 *
 * @exception KTX_INVALID_VALUE @p newTex is @c NULL.
 * @exception KTX_FILE_DATA_ERROR
 *                              Source data is inconsistent with the KTX
 *                              specification.
 * @exception KTX_FILE_READ_ERROR
 *                              An error occurred while reading the source.
 * @exception KTX_FILE_UNEXPECTED_EOF
 *                              Not enough data in the source.
 * @exception KTX_OUT_OF_MEMORY Not enough memory to create the texture object,
 *                              load the images or load the key-value data.
 * @exception KTX_UNKNOWN_FILE_FORMAT
 *                              The source is not in KTX format.
 * @exception KTX_UNSUPPORTED_TEXTURE_TYPE
 *                              The source describes a texture type not
 *                              supported by OpenGL or Vulkan, e.g, a 3D array.
 */
KTX_error_code
ktxTexture_CreateFromStdioStream(FILE* stdioStream,
                                 ktxTextureCreateFlags createFlags,
                                 ktxTexture** newTex)
{
    KTX_error_code result;
    if (newTex == NULL)
        return KTX_INVALID_VALUE;
    
    ktxTextureInt* tex = (ktxTextureInt*)malloc(sizeof(ktxTextureInt));
    if (tex == NULL)
        return KTX_OUT_OF_MEMORY;
    
    result = ktxTextureInt_constructFromStdioStream(tex, stdioStream,
                                                    createFlags);
    if (result == KTX_SUCCESS)
        *newTex = (ktxTexture*)tex;
    else {
        free(tex);
        *newTex = NULL;
    }
    return result;
}

/**
 * @memberof ktxTexture
 * @~English
 * @brief Create a ktxTexture from a named KTX file.
 *
 * The address of a newly created ktxTexture reflecting the contents of the
 * file is written to the location pointed at by @p newTex.
 *
 * The create flag KTX_TEXTURE_CREATE_LOAD_IMAGE_DATA_BIT should not be set,
 * if the ktxTexture is ultimately to be uploaded to OpenGL or Vulkan. This
 * will minimize memory usage by allowing, for example, loading the images
 * directly from the source into a Vulkan staging buffer.
 *
 * The create flag KTX_TEXTURE_CREATE_RAW_KVDATA_BIT should not be used. It is
 * provided solely to enable implementation of the @e libktx v1 API on top of
 * ktxTexture.
 *
 * @param[in] filename    pointer to a char array containing the file name.
 * @param[in] createFlags bitmask requesting specific actions during creation.
 * @param[in,out] newTex  pointer to a location in which store the address of
 *                        the newly created texture.
 *
 * @return      KTX_SUCCESS on success, other KTX_* enum values on error.

 * @exception KTX_FILE_OPEN_FAILED The file could not be opened.
 * @exception KTX_INVALID_VALUE @p filename is @c NULL.
 *
 * For other exceptions, see ktxTexture_CreateFromStdioStream().
 */
KTX_error_code
ktxTexture_CreateFromNamedFile(const char* const filename,
                               ktxTextureCreateFlags createFlags,
                               ktxTexture** newTex)
{
    KTX_error_code result;

    if (newTex == NULL)
        return KTX_INVALID_VALUE;

    ktxTextureInt* tex = (ktxTextureInt*)malloc(sizeof(ktxTextureInt));
    if (tex == NULL)
        return KTX_OUT_OF_MEMORY;
    
    result = ktxTextureInt_constructFromNamedFile(tex, filename, createFlags);
    if (result == KTX_SUCCESS)
        *newTex = (ktxTexture*)tex;
    else {
        free(tex);
        *newTex = NULL;
    }
    return result;
}

/**
 * @memberof ktxTexture
 * @~English
 * @brief Create a ktxTexture from KTX-formatted data in memory.
 *
 * The address of a newly created ktxTexture reflecting the contents of the
 * serialized KTX data is written to the location pointed at by @p newTex.
 *
 * The create flag KTX_TEXTURE_CREATE_LOAD_IMAGE_DATA_BIT should not be set,
 * if the ktxTexture is ultimately to be uploaded to OpenGL or Vulkan. This
 * will minimize memory usage by allowing, for example, loading the images
 * directly from the source into a Vulkan staging buffer.
 *
 * The create flag KTX_TEXTURE_CREATE_RAW_KVDATA_BIT should not be used. It is
 * provided solely to enable implementation of the @e libktx v1 API on top of
 * ktxTexture.
 *
 * @param[in] bytes pointer to the memory containing the serialized KTX data.
 * @param[in] size  length of the KTX data in bytes.
 * @param[in] createFlags bitmask requesting specific actions during creation.
 * @param[in,out] newTex  pointer to a location in which store the address of
 *                        the newly created texture.
 *
 * @return      KTX_SUCCESS on success, other KTX_* enum values on error.
 *
 * @exception KTX_INVALID_VALUE Either @p bytes is NULL or @p size is 0.
 *
 * For other exceptions, see ktxTexture_CreateFromStdioStream().
 */
KTX_error_code
ktxTexture_CreateFromMemory(const ktx_uint8_t* bytes, ktx_size_t size,
                            ktxTextureCreateFlags createFlags,
                            ktxTexture** newTex)
{
    KTX_error_code result;
    if (newTex == NULL)
        return KTX_INVALID_VALUE;
    
    ktxTextureInt* tex = (ktxTextureInt*)malloc(sizeof(ktxTextureInt));
    if (tex == NULL)
        return KTX_OUT_OF_MEMORY;
    
    result = ktxTextureInt_constructFromMemory(tex, bytes, size,
                                               createFlags);
    if (result == KTX_SUCCESS)
        *newTex = (ktxTexture*)tex;
    else {
        free(tex);
        *newTex = NULL;
    }
    return result;
}

/**
 * @memberof ktxTexture
 * @~English
 * @brief Destroy a ktxTexture object.
 *
 * This frees the memory associated with the texture contents and the memory
 * of the ktxTexture object. This does @e not delete any OpenGL or Vulkan
 * texture objects created by ktxTexture_GLUpload or ktxTexture_VkUpload.
 *
 * @param[in] This pointer to the ktxTexture object to destroy
 */
void
ktxTexture_Destroy(ktxTexture* This)
{
    ktxTextureInt_destruct((ktxTextureInt*)This);
    free(This);
}

/**
 * @memberof ktxTexture
 * @~English
 * @brief Return a pointer to the texture image data.
 *
 * @param[in] This pointer to the ktxTexture object of interest.
 */
ktx_uint8_t*
ktxTexture_GetData(ktxTexture* This)
{
    return This->pData;
}

/**
 * @memberof ktxTexture
 * @~English
 * @brief Return the total size of the texture image data in bytes.
 *
 * @param[in] This pointer to the ktxTexture object of interest.
 */
ktx_size_t
ktxTexture_GetSize(ktxTexture* This)
{
    assert(This != NULL);
    return This->dataSize;
}

/**
 * @memberof ktxTexture
 * @~English
 * @brief Return the size in bytes of an elements of a texture's
 *        images.
 *
 * For uncompressed textures an element is one texel. For compressed
 * textures it is one block.
 *
 * @param[in]     This     pointer to the ktxTexture object of interest.
 */
ktx_uint32_t
ktxTexture_GetElementSize(ktxTexture* This)
{
    GlFormatSize* formatInfo;

    assert (This != NULL);

    formatInfo = &((ktxTextureInt*)This)->formatInfo;
    return (formatInfo->blockSizeInBits / 8);
}

/**
 * @memberof ktxTexture
 * @~English
 * @brief Calculate & return the size in bytes of an image at the specified
 *        mip level.
 *
 * For arrays, this is the size of layer, for cubemaps, the size of a face
 * and for 3D textures, the size of a depth slice.
 *
 * The size reflects the padding of each row to KTX_GL_UNPACK_ALIGNMENT.
 *
 * @param[in]     This     pointer to the ktxTexture object of interest.
 * @param[in]     level    level of interest. *
 */
ktx_size_t
ktxTexture_GetImageSize(ktxTexture* This, ktx_uint32_t level)
{
    GlFormatSize* formatInfo;
    struct blockCount {
        ktx_uint32_t x, y, z;
    } blockCount;
    ktx_uint32_t blockSizeInBytes;
    ktx_uint32_t rowBytes;

    assert (This != NULL);

    formatInfo = &((ktxTextureInt*)This)->formatInfo;

    float levelWidth  = (float) (This->baseWidth >> level);
    float levelHeight = (float) (This->baseHeight >> level);
    blockCount.x      = (ktx_uint32_t) ceilf(levelWidth / formatInfo->blockWidth);
    blockCount.y      = (ktx_uint32_t) ceilf(levelHeight / formatInfo->blockHeight);
    blockCount.x      = MAX(1, blockCount.x);
    blockCount.y      = MAX(1, blockCount.y);
    blockSizeInBytes  = formatInfo->blockSizeInBits / 8;

    if (formatInfo->flags & GL_FORMAT_SIZE_COMPRESSED_BIT) {
        assert(This->isCompressed);
        return blockCount.x * blockCount.y * blockSizeInBytes;
    } else {
        assert(formatInfo->blockWidth == formatInfo->blockHeight == formatInfo->blockDepth == 1);
        rowBytes = blockCount.x * blockSizeInBytes;
        (void)padRow(&rowBytes);
        return rowBytes * blockCount.y;
    }
}

/**
 * @memberof ktxTexture
 * @~English
 * @brief Load all the image data from the ktxTexture's source.
 *
 * The data is loaded into the provided buffer or to an internally allocated
 * buffer, if @p pBuffer is @c NULL.
 *
 * @param[in] This pointer to the ktxTexture object of interest.
 * @param[in] pBuffer pointer to the buffer in which to load the image data.
 * @param[in] bufSize size of the buffer pointed at by @p pBuffer.
 *
 * @return      KTX_SUCCESS on success, other KTX_* enum values on error.
 *
 * @exception KTX_INVALID_VALUE @p This is NULL.
 * @exception KTX_INVALID_VALUE @p bufSize is less than the the image data size.
 * @exception KTX_INVALID_OPERATION
 *                              The data has already been loaded or the
 *                              ktxTexture was not created from a KTX source.
 * @exception KTX_OUT_OF_MEMORY Insufficient memory for the image data.
 */
KTX_error_code
ktxTexture_LoadImageData(ktxTexture* This,
                         ktx_uint8_t* pBuffer, ktx_size_t bufSize)
{
    ktxTextureInt*  subthis = (ktxTextureInt*)This;
    ktx_uint32_t    miplevel;
    ktx_uint8_t*    pDest;
    KTX_error_code  result = KTX_SUCCESS;

    if (This == NULL)
        return KTX_INVALID_VALUE;
    
    if (subthis->stream.data.file == NULL)
        // This Texture not created from a stream or images already loaded;
        return KTX_INVALID_OPERATION;

    if (pBuffer == NULL) {
        This->pData = malloc(This->dataSize);
        if (This->pData == NULL)
            return KTX_OUT_OF_MEMORY;
        pDest = This->pData;
    } else if (bufSize < This->dataSize) {
        return KTX_INVALID_VALUE;
    } else {
        pDest = pBuffer;
    }

    // Need to loop through for correct byte swapping
    for (miplevel = 0; miplevel < This->numLevels; ++miplevel)
    {
        ktx_uint32_t faceLodSize;
        ktx_uint32_t faceLodSizePadded;
        ktx_uint32_t face;
        ktx_uint32_t innerIterations;

        result = subthis->stream.read(&subthis->stream, &faceLodSize,
                                      sizeof(ktx_uint32_t));
        if (result != KTX_SUCCESS) {
            goto cleanup;
        }
        if (subthis->needSwap) {
            _ktxSwapEndian32(&faceLodSize, 1);
        }
#if (KTX_GL_UNPACK_ALIGNMENT != 4)
        faceLodSizePadded = _KTX_PAD4(faceLodSize);
#else
        faceLodSizePadded = faceLodSize;
#endif
        
        if (This->isCubemap && !This->isArray)
            innerIterations = This->numFaces;
        else
            innerIterations = 1;
        for (face = 0; face < innerIterations; ++face)
        {
            result = subthis->stream.read(&subthis->stream, pDest,
                                          faceLodSizePadded);
            if (result != KTX_SUCCESS) {
                goto cleanup;
            }
            
            /* Perform endianness conversion on texture data */
            if (subthis->needSwap) {
                if (subthis->glTypeSize == 2)
                    _ktxSwapEndian16((ktx_uint16_t*)pDest, faceLodSize / 2);
                else if (subthis->glTypeSize == 4)
                    _ktxSwapEndian32((ktx_uint32_t*)pDest, faceLodSize / 4);
            }
            
            pDest += faceLodSizePadded;
        }
    }

cleanup:
    // No further need for This->
    subthis->stream.destruct(&subthis->stream);
    return result;
}

/**
 * @memberof ktxTexture
 * @~English
 * @brief Iterate over the images in a ktxTexture object.
 *
 * Blocks of image data are passed to an application-supplied callback
 * function. This is not a strict per-image iteration. Rather it reflects how
 * OpenGL needs the images. For most textures the block of data includes all
 * images of a mip level which implies all layers of an array. However, for
 * non-array cube map textures the block is a single face of the mip level,
 * i.e the callback is called once for each face.
 *
 * This function works even if @p This->pData == 0 so it can be used to
 * obtain offsets and sizes for each level by callers who have loaded the data
 * externally.
 *
 * @param[in]     This      pointer to the ktxTexture object of interest.
 * @param[in,out] iterCb    the address of a callback function which is called
 *                          with the data for each image block.
 * @param[in,out] userdata  the address of application-specific data which is
 *                          passed to the callback along with the image data.
 *
 * @return  KTX_SUCCESS on success, other KTX_* enum values on error. The
 *          following are returned directly by this function. @p iterCb may
 *          return these for other causes or may return additional errors.
 *
 * @exception KTX_FILE_DATA_ERROR   Mip level sizes are increasing not
 *                                  decreasing
 * @exception KTX_INVALID_VALUE     @p This is @c NULL or @p iterCb is @c NULL.
 *
 */
KTX_error_code
ktxTexture_IterateLevelFaces(ktxTexture* This, PFNKTXITERCB iterCb,
                             void* userdata)
{
    ktx_uint32_t    miplevel;
    KTX_error_code  result = KTX_SUCCESS;
    
    if (This == NULL)
        return KTX_INVALID_VALUE;
    
    if (iterCb == NULL)
        return KTX_INVALID_VALUE;
    
    for (miplevel = 0; miplevel < This->numLevels; ++miplevel)
    {
        ktx_uint32_t faceLodSize;
        ktx_uint32_t face;
        ktx_uint32_t innerIterations;
        GLsizei      width, height, depth;
        
        /* Array textures have the same number of layers at each mip level. */
        width = MAX(1, This->baseWidth  >> miplevel);
        height = MAX(1, This->baseHeight >> miplevel);
        depth = MAX(1, This->baseDepth  >> miplevel);

        faceLodSize = (ktx_uint32_t)ktxTexture_faceLodSize(This, miplevel);

        /* All array layers are passed in a group because that is how
         * GL & Vulkan need them. Hence no
         *    for (layer = 0; layer < This->numLayers)
         */
        if (This->isCubemap && !This->isArray)
            innerIterations = This->numFaces;
        else
            innerIterations = 1;
        for (face = 0; face < innerIterations; ++face)
        {
            /* And all z_slices are also passed as a group hence no
             *    for (slice = 0; slice < This->depth)
             */
            ktx_size_t offset;

            ktxTexture_GetImageOffset(This, miplevel, 0, face, &offset);
            result = iterCb(miplevel, face,
                             width, height, depth,
                             faceLodSize, This->pData + offset, userdata);
            
            if (result != KTX_SUCCESS)
                break;
        }
    }
    
    return result;
}
/**
 * @memberof ktxTexture
 * @~English
 * @brief Iterate over the images in a ktxTexture object while loading the
 *        image data.
 *
 * This operates similarly to ktxTexture_IterateLevelFaces() except that it
 * loads the images from the ktxTexture's source to a temporary buffer
 * while iterating. The callback function must copy the image data if it
 * wishes to preserve it as the temporary buffer is reused for each level and
 * is freed when this function exits.
 *
 * This function is helpful for reducing memory usage when uploading the data
 * to a graphics API.
 *
 * @param[in]     This     pointer to the ktxTexture object of interest.
 * @param[in,out] iterCb   the address of a callback function which is called
 *                         with the data for each image.
 * @param[in,out] userdata the address of application-specific data which is
 *                         passed to the callback along with the image data.
 *
 * @return  KTX_SUCCESS on success, other KTX_* enum values on error. The
 *          following are returned directly by this function. @p iterCb may
 *          return these for other causes or may return additional errors.
 *
 * @exception KTX_FILE_DATA_ERROR   mip level sizes are increasing not
 *                                  decreasing
 * @exception KTX_INVALID_OPERATION the ktxTexture was not created from a
 *                                  stream, i.e there is no data to load, or
 *                                  this ktxTexture's images have already
 *                                  been loaded.
 * @exception KTX_INVALID_VALUE     @p This is @c NULL or @p iterCb is @c NULL.
 * @exception KTX_OUT_OF_MEMORY     not enough memory to allocate a block to
 *                                  hold the base level image.
 */
KTX_error_code
ktxTexture_IterateLoadLevelFaces(ktxTexture* This, PFNKTXITERCB iterCb,
                                 void* userdata)
{
    ktxTextureInt*  subthis = (ktxTextureInt*)This;
    ktx_uint32_t    dataSize = 0;
    ktx_uint32_t    miplevel;
    KTX_error_code  result = KTX_SUCCESS;
    void*           data = NULL;
    
    if (This == NULL)
        return KTX_INVALID_VALUE;
    
    if (iterCb == NULL)
        return KTX_INVALID_VALUE;
    
    if (subthis->stream.data.file == NULL)
        // This Texture not created from a stream or images are already loaded.
        return KTX_INVALID_OPERATION;
    
    for (miplevel = 0; miplevel < This->numLevels; ++miplevel)
    {
        ktx_uint32_t faceLodSize;
        ktx_uint32_t faceLodSizePadded;
        ktx_uint32_t face;
        ktx_uint32_t innerIterations;
        GLsizei      width, height, depth;
        
        /* Array textures have the same number of layers at each mip level. */
        width = MAX(1, This->baseWidth  >> miplevel);
        height = MAX(1, This->baseHeight >> miplevel);
        depth = MAX(1, This->baseDepth  >> miplevel);
        
        result = subthis->stream.read(&subthis->stream, &faceLodSize,
                                      sizeof(ktx_uint32_t));
        if (result != KTX_SUCCESS) {
            goto cleanup;
        }
        if (subthis->needSwap) {
            _ktxSwapEndian32(&faceLodSize, 1);
        }
#if (KTX_GL_UNPACK_ALIGNMENT != 4)
        faceLodSizePadded = _KTX_PAD4(faceLodSize);
#else
        faceLodSizePadded = faceLodSize;
#endif
        if (!data) {
            /* allocate memory sufficient for the base miplevel */
            data = malloc(faceLodSizePadded);
            if (!data) {
                result = KTX_OUT_OF_MEMORY;
                goto cleanup;
            }
            dataSize = faceLodSizePadded;
        }
        else if (dataSize < faceLodSizePadded) {
            /* subsequent miplevels cannot be larger than the base miplevel */
            result = KTX_FILE_DATA_ERROR;
            goto cleanup;
        }
        
        /* All array layers are passed in a group because that is how
         * GL & Vulkan need them. Hence no
         *    for (layer = 0; layer < This->numLayers)
         */
        if (This->isCubemap && !This->isArray)
            innerIterations = This->numFaces;
        else
            innerIterations = 1;
        for (face = 0; face < innerIterations; ++face)
        {
            /* And all z_slices are also passed as a group hence no
             *    for (z_slice = 0; z_slice < This->depth)
             */
            result = subthis->stream.read(&subthis->stream, data,
                                          faceLodSizePadded);
            if (result != KTX_SUCCESS) {
                goto cleanup;
            }
            
            /* Perform endianness conversion on texture data */
            if (subthis->needSwap) {
                if (subthis->glTypeSize == 2)
                    _ktxSwapEndian16((ktx_uint16_t*)data, faceLodSize / 2);
                else if (subthis->glTypeSize == 4)
                    _ktxSwapEndian32((ktx_uint32_t*)data, faceLodSize / 4);
            }
            
            result = iterCb(miplevel, face,
                             width, height, depth,
                             faceLodSize, data, userdata);
            
            if (result != KTX_SUCCESS)
                goto cleanup;
        }
    }
    
cleanup:
    free(data);
    // No further need for this.
    subthis->stream.destruct(&subthis->stream);

    return result;
}

/**
 * @memberof ktxTexture
 * @~English
 * @brief Iterate over the mip levels in a ktxTexture object.
 *
 * This is almost identical to ktxTexture_IterateLevelFaces(). The difference is
 * that the blocks of image data for non-array cube maps include all faces of
 * a mip level.
 *
 * This function works even if @p This->pData == 0 so it can be used to
 * obtain offsets and sizes for each level by callers who have loaded the data
 * externally.
 *
 * @param[in]     This     handle of the ktxTexture opened on the data.
 * @param[in,out] iterCb   the address of a callback function which is called
 *                         with the data for each image block.
 * @param[in,out] userdata the address of application-specific data which is
 *                         passed to the callback along with the image data.
 *
 * @return  KTX_SUCCESS on success, other KTX_* enum values on error. The
 *          following are returned directly by this function. @p iterCb may
 *          return these for other causes or may return additional errors.
 *
 * @exception KTX_FILE_DATA_ERROR   Mip level sizes are increasing not
 *                                  decreasing
 * @exception KTX_INVALID_VALUE     @p This is @c NULL or @p iterCb is @c NULL.
 *
 */
KTX_error_code
ktxTexture_IterateLevels(ktxTexture* This, PFNKTXITERCB iterCb, void* userdata)
{
    ktx_uint32_t    miplevel;
    KTX_error_code  result = KTX_SUCCESS;
    
    if (This == NULL)
        return KTX_INVALID_VALUE;
    
    if (iterCb == NULL)
        return KTX_INVALID_VALUE;
    
    for (miplevel = 0; miplevel < This->numLevels; ++miplevel)
    {
        GLsizei width, height, depth;
        ktx_uint32_t levelSize;
        ktx_size_t offset;
        
        /* Array textures have the same number of layers at each mip level. */
        width = MAX(1, This->baseWidth  >> miplevel);
        height = MAX(1, This->baseHeight >> miplevel);
        depth = MAX(1, This->baseDepth  >> miplevel);

        levelSize = (ktx_uint32_t)ktxTexture_levelSize(This, miplevel);

        /* All array layers are passed in a group because that is how
         * GL & Vulkan need them. Hence no
         *    for (layer = 0; layer < This->numLayers)
         */
        ktxTexture_GetImageOffset(This, miplevel, 0, 0, &offset);
        result = iterCb(miplevel, 0, width, height, depth,
                         levelSize, This->pData + offset, userdata);
        if (result != KTX_SUCCESS)
            break;
    }
    
    return result;
}

/**
 * @internal
 * @brief  Calculate and apply the padding needed to comply with
 *         KTX_GL_UNPACK_ALIGNMENT.
 *
 * For uncompressed textures, KTX format specifies KTX_GL_UNPACK_ALIGNMENT = 4.
 *
 * @param[in,out] rowBytes    pointer to variable containing the packed no. of
 *                            bytes in a row. The no. of bytes after padding
 *                            is written into this location.
 * @return the no. of bytes of padding.
 */
static ktx_uint32_t
padRow(ktx_uint32_t* rowBytes)
{
    ktx_uint32_t rowPadding;

    assert (rowBytes != NULL);

    rowPadding = _KTX_PAD_UNPACK_ALIGN_LEN(*rowBytes);
    *rowBytes += rowPadding;
    return rowPadding;
}

/**
 * @memberof ktxTexture @private
 * @~English
 * @brief Calculate the size of an array layer at the specified mip level.
 *
 * The size of a layer is the size of an image * either the number of faces
 * or the number of depth slices. This is the size of a layer as needed to
 * find the offset within the array of images of a level and layer so the size
 * reflects any @c cubePadding.
 *
 * @param[in]  This     pointer to the ktxTexture object of interest.
 * @param[in] level     level whose layer size to return.
 *
 * @return the layer size in bytes.
 */
static inline ktx_size_t
ktxTexture_layerSize(ktxTexture* This, ktx_uint32_t level)
{
    /*
     * As there are no 3D cubemaps, the image's z block count will always be
     * 1 for cubemaps and numFaces will always be 1 for 3D textures so the
     * multiply is safe. 3D cubemaps, if they existed, would require
     * imageSize * (blockCount.z + This->numFaces);
     */
    GlFormatSize* formatInfo;
    ktx_uint32_t blockCountZ;
    ktx_size_t imageSize, layerSize;

    assert (This != NULL);
    
    formatInfo = &((ktxTextureInt*)This)->formatInfo;
    blockCountZ = MAX(1, (This->baseDepth / formatInfo->blockDepth)  >> level);
    imageSize = ktxTexture_GetImageSize(This, level);
    layerSize = imageSize * blockCountZ;
#if (KTX_GL_UNPACK_ALIGNMENT != 4)
    if (This->isCubemap && !This->isArray) {
        /* cubePadding. NOTE: this adds padding after the last face too. */
        _KTX_PAD4(layerSize);
    }
#endif
    return layerSize * This->numFaces;
}

/**
 * @memberof ktxTexture @private
 * @~English
 * @brief Calculate the size of the specified mip level.
 *
 * The size of a level is the size of a layer * the number of layers.
 *
 * @param[in]  This     pointer to the ktxTexture object of interest.
 * @param[in] level     level whose layer size to return.
 *
 * @return the level size in bytes.
 */
ktx_size_t
ktxTexture_levelSize(ktxTexture* This, ktx_uint32_t level)
{
    assert (This != NULL);
    return ktxTexture_layerSize(This, level) * This->numLayers;
}

/**
 * @memberof ktxTexture @private
 * @~English
 * @brief Calculate the faceLodSize of the specified mip level.
 *
 * The faceLodSize of a level for most textures is the size of a level. For
 * non-array cube map textures is the size of a face. This is the size that
 * must be provided to OpenGL when uploading textures. Faces get uploaded 1
 * at a time while all layers of an array or all slices of a 3D texture are
 * uploaded together.
 *
 * @param[in]  This     pointer to the ktxTexture object of interest.
 * @param[in] level     level whose layer size to return.
 *
 * @return the faceLodSize size in bytes.
 */
ktx_size_t
ktxTexture_faceLodSize(ktxTexture* This, ktx_uint32_t level)
{
    /*
     * For non-array cubemaps this is the size of a face. For everything
     * else it is the size of the level.
     */
    if (This->isCubemap && !This->isArray)
        return ktxTexture_GetImageSize(This, level);
    else
        return ktxTexture_levelSize(This, level);
}

/**
 * @memberof ktxTexture @private
 * @~English
 * @brief Calculate the size of the image data for the specified number
 *        of levels.
 *
 * The data size is the sum of the sizes of each level up to the number
 * specified and includes any @c mipPadding.
 *
 * @param[in] This     pointer to the ktxTexture object of interest.
 * @param[in] levels   number of levels whose data size to return.
 *
 * @return the data size in bytes.
 */
static inline ktx_size_t
ktxTexture_dataSize(ktxTexture* This, ktx_uint32_t levels)
{
    ktx_uint32_t i;
    ktx_size_t dataSize = 0;

    assert (This != NULL);
    for (i = 0; i < levels; i++) {
        ktx_size_t levelSize = ktxTexture_levelSize(This, i);
#if (KTX_GL_UNPACK_ALIGNMENT != 4)
        /* mipPadding. NOTE: this adds padding after the last level too. */
        dataSize += _KTX_PAD4(levelSize);
#else
        dataSize += levelSize;
#endif
    }
    return dataSize;
}

/**
 * @memberof ktxTexture @private
 * @~English
 * @brief Return the number of bytes needed to store all the image data for
 *        a ktxTexture.
 *
 * The caclulated size does not include space for storing the @c imageSize
 * fields of each mip level.
 *
 * @param[in]     This       pointer to the ktxTexture object of interest.
 *
 * @return the data size in bytes.
 */
static ktx_size_t
ktxTexture_calcDataSize(ktxTexture* This)
{
    assert (This != NULL);
    return ktxTexture_dataSize(This, This->numLevels);
}

/**
 * @memberof ktxTexture @private
 * @~English
 * @brief Return the size of the primitive type of a single color component
 *
 * @param[in]     This       pointer to the ktxTexture object of interest.
 *
 * @return the type size in bytes.
 */
ktx_uint32_t
ktxTexture_glTypeSize(ktxTexture* This)
{
    assert(This != NULL);
    return ((ktxTextureInt*)This)->glTypeSize;
}

/**
 * @memberof ktxTexture @private
 * @~English
 * @brief Get information about rows of an uncompresssed texture image at a
 *        specified level.
 *
 * For an image at @p level of a ktxTexture provide the number of rows, the
 * packed (unpadded) number of bytes in a row and the padding necessary to
 * comply with KTX_GL_UNPACK_ALIGNMENT.
 *
 * @param[in]     This     pointer to the ktxTexture object of interest.
 * @param[in]     level    level of interest.
 * @param[in,out] numRows  pointer to location to store the number of rows.
 * @param[in,out] pRowLengthBytes pointer to location to store number of bytes
 *                                in a row.
 * @param[in.out] pRowPadding pointer to location to store the number of bytes
 *                            of padding.
 */
void
ktxTexture_rowInfo(ktxTexture* This, ktx_uint32_t level,
                   ktx_uint32_t* numRows, ktx_uint32_t* pRowLengthBytes,
                   ktx_uint32_t* pRowPadding)
{
    GlFormatSize* formatInfo;
    struct blockCount {
        ktx_uint32_t x;
    } blockCount;

    assert (This != NULL);
    
    formatInfo = &((ktxTextureInt*)This)->formatInfo;
    assert(!This->isCompressed);
    assert(formatInfo->blockWidth == formatInfo->blockHeight == formatInfo->blockDepth == 1);

    blockCount.x = MAX(1, (This->baseWidth / formatInfo->blockWidth)  >> level);
    *numRows = MAX(1, (This->baseHeight / formatInfo->blockHeight)  >> level);

    *pRowLengthBytes = blockCount.x * formatInfo->blockSizeInBits / 8;
    *pRowPadding = padRow(pRowLengthBytes);
}

/**
 * @memberof ktxTexture
 * @~English
 * @brief Return pitch betweeb rows of a texture image level in bytes.
 *
 * For uncompressed textures the pitch is the number of bytes between
 * rows of texels. For compressed textures it is the number of bytes
 * between rows of blocks. The value is padded to GL_UNPACK_ALIGNMENT,
 * if necessary. For all currently known compressed formats padding
 * will not be necessary.
 *
 * @param[in]     This     pointer to the ktxTexture object of interest.
 * @param[in]     level    level of interest.
 *
 * @return  the row pitch in bytes.
 */
 ktx_uint32_t
 ktxTexture_GetRowPitch(ktxTexture* This, ktx_uint32_t level)
 {
    GlFormatSize* formatInfo;
    struct blockCount {
        ktx_uint32_t x;
    } blockCount;
    ktx_uint32_t pitch;

    formatInfo = &((ktxTextureInt*)This)->formatInfo;
    blockCount.x = MAX(1, (This->baseWidth / formatInfo->blockWidth)  >> level);
    pitch = blockCount.x * formatInfo->blockSizeInBits / 8;
    (void)padRow(&pitch);

    return pitch;
 }

/**
 * @memberof ktxTexture @private
 * @~English
 * @brief Query if a ktxTexture has an active stream.
 *
 * Tests if a ktxTexture has unread image data. The internal stream is closed
 * once all the images have been read.
 *
 * @param[in]     This     pointer to the ktxTexture object of interest.
 *
 * @return KTX_TRUE if there is an active stream, KTX_FALSE otherwise.
 */
ktx_bool_t
ktxTexture_isActiveStream(ktxTexture* This)
{
    assert(This != NULL);
    return ((ktxTextureInt*)This)->stream.data.file != NULL;
}


/**
 * @memberof ktxTexture
 * @~English
 * @brief Find the offset of an image within a ktxTexture's image data.
 *
 * As there is no such thing as a 3D cubemap we make the 3rd location parameter
 * do double duty.
 *
 * @param[in]     This      pointer to the ktxTexture object of interest.
 * @param[in]     level     mip level of the image.
 * @param[in]     layer     array layer of the image.
 * @param[in]     faceSlice cube map face or depth slice of the image.
 * @param[in,out] pOffset   pointer to location to store the offset.
 *
 * @return  KTX_SUCCESS on success, other KTX_* enum values on error.
 *
 * @exception KTX_INVALID_OPERATION
 *                         @p level, @p layer or @p faceSlice exceed the
 *                         dimensions of the texture.
 * @exception KTX_INVALID_VALID @p This is NULL.
 */
KTX_error_code
ktxTexture_GetImageOffset(ktxTexture* This, ktx_uint32_t level,
                          ktx_uint32_t layer, ktx_uint32_t faceSlice,
                          ktx_size_t* pOffset)
{
    if (This == NULL)
        return KTX_INVALID_VALUE;

    if (level >= This->numLevels || layer >= This->numLayers)
        return KTX_INVALID_OPERATION;
    
    if (This->isCubemap) {
        if (faceSlice >= This->numFaces)
            return KTX_INVALID_OPERATION;
    } else {
        ktx_uint32_t maxSlice = MAX(1, This->baseDepth >> level);
        if (faceSlice >= maxSlice)
            return KTX_INVALID_OPERATION;
    }
    
    // Get the size of the data up to the start of the indexed level.
    *pOffset = ktxTexture_dataSize(This, level);
    
    // All layers, faces & slices within a level are the same size.
    if (layer != 0) {
        ktx_size_t layerSize;
        layerSize = ktxTexture_layerSize(This, level);
        *pOffset += layer * layerSize;
    }
    if (faceSlice != 0) {
        ktx_size_t imageSize;
        imageSize = ktxTexture_GetImageSize(This, level);
#if (KTX_GL_UNPACK_ALIGNMENT != 4)
        if (This->isCubemap)
            _KTX_PAD4(imageSize); // Account for cubePadding.
#endif
        *pOffset += faceSlice * imageSize;
    }

    return KTX_SUCCESS;
}

/** @} */

