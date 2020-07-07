/* -*- tab-width: 4; -*- */
/* vi: set sw=2 ts=4 expandtab: */

/**
 * @internal
 * @file writer.c
 * @~English
 *
 * @brief Functions for creating KTX-format files from a set of images.
 *
 * @author Mark Callow, HI Corporation
 */

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

#ifdef _WIN32
#define _CRT_SECURE_NO_WARNINGS
#endif

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "ktx.h"
#include "ktxint.h"
#include "stream.h"
#include "filestream.h"
#include "memstream.h"

/**
 * @defgroup writer Writer
 * @brief Write KTX-formatted data.
 * @{
 */

/**
 * @internal
 * @memberof ktxTexture @private
 * @~English
 * @brief Set image for level, layer, faceSlice from a ktxStream source.
 *
 * @param[in] This      pointer to the target ktxTexture object.
 * @param[in] level     mip level of the image to set.
 * @param[in] layer     array layer of the image to set.
 * @param[in] faceSlice cube map face or depth slice of the image to set.
 * @param[in] src       ktxStream pointer to the source.
 * @param[in] srcSize   size of the source image in bytes.
 *
 * @return      KTX_SUCCESS on success, other KTX_* enum values on error.
 *
 * @exception KTX_INVALID_VALUE @p This or @p src is NULL.
 * @exception KTX_INVALID_VALUE @p srcSize != the expected image size for the
 *                              specified level, layer & faceSlice.
 * @exception KTX_INVALID_OPERATION
 *                              No storage was allocated when the texture was
 *                              created.
 */
KTX_error_code
ktxTexture_setImageFromStream(ktxTexture* This, ktx_uint32_t level,
                              ktx_uint32_t layer, ktx_uint32_t faceSlice,
                              ktxStream* src, ktx_size_t srcSize)
{
    ktx_uint32_t packedRowBytes, rowBytes, rowPadding, numRows;
    ktx_size_t packedBytes, unpackedBytes;
    ktx_size_t imageOffset;
#if (KTX_GL_UNPACK_ALIGNMENT != 4)
    ktx_uint32_t faceLodPadding;
#endif
    
    if (!This || !src)
        return KTX_INVALID_VALUE;
    
    if (!This->pData)
        return KTX_INVALID_OPERATION;
    
    ktxTexture_GetImageOffset(This, level, layer, faceSlice, &imageOffset);

    if (This->isCompressed) {
        packedBytes = ktxTexture_GetImageSize(This, level);
        rowPadding = 0;
        // These 2 are not used when rowPadding == 0. Quiets compiler warning.
        packedRowBytes = 0;
        rowBytes = 0;
    } else {
        ktxTexture_rowInfo(This, level, &numRows, &rowBytes, &rowPadding);
        unpackedBytes = rowBytes * numRows;
        if (rowPadding) {
            packedRowBytes = rowBytes - rowPadding;
            packedBytes = packedRowBytes * numRows;
        } else {
            packedRowBytes = rowBytes;
            packedBytes = unpackedBytes;
        }
    }
    
    if (srcSize != packedBytes)
        return KTX_INVALID_OPERATION;
    // The above will catch a flagrantly invalid srcSize. This is an
    // additional check of the internal calculations.
    assert (imageOffset + srcSize <= This->dataSize);
    
#if (KTX_GL_UNPACK_ALIGNMENT != 4)
    faceLodPadding = _KTX_PAD4_LEN(faceLodSize);
#endif
    
    if (rowPadding == 0) {
        /* Can copy whole image at once */
        src->read(src, This->pData + imageOffset, srcSize);
    } else {
        /* Copy the rows individually, padding each one */
        ktx_uint32_t row;
        ktx_uint8_t* dst = This->pData + imageOffset;
        ktx_uint8_t pad[4] = { 0, 0, 0, 0 };
        for (row = 0; row < numRows; row++) {
            ktx_uint32_t rowOffset = rowBytes * row;
            src->read(src, dst + rowOffset, packedRowBytes);
            memcpy(dst + rowOffset + packedRowBytes, pad, rowPadding);
        }
    }
#if (KTX_GL_UNPACK_ALIGNMENT != 4)
    /*
     * When KTX_GL_UNPACK_ALIGNMENT == 4, rows, and therefore everything else,
     * are always 4-byte aligned and faceLodPadding is always 0. It is always
     * 0 for compressed formats too because they all have multiple-of-4 block
     * sizes.
     */
    if (faceLodPadding)
        memcpy(This->pData + faceLodSize, pad, faceLodPadding);
#endif
    return KTX_SUCCESS;
}

/**
 * @memberof ktxTexture
 * @~English
 * @brief Set image for level, layer, faceSlice from a stdio stream source.
 *
 * Uncompressed images read from the stream are expected to have their rows
 * tightly packed as is the norm for most image file formats. The copied image
 * is padded as necessary to achieve the KTX-specified row alignment. No
 * padding is done if the ktxTexture's @c isCompressed field is @c KTX_TRUE.
 *
 * Level, layer, faceSlice rather than offset are specified to enable some
 * validation.
 *
 * @param[in] This      pointer to the target ktxTexture object.
 * @param[in] level     mip level of the image to set.
 * @param[in] layer     array layer of the image to set.
 * @param[in] faceSlice cube map face or depth slice of the image to set.
 * @param[in] src       stdio stream pointer to the source.
 * @param[in] srcSize   size of the source image in bytes.
 *
 * @return      KTX_SUCCESS on success, other KTX_* enum values on error.
 *
 * @exception KTX_INVALID_VALUE @p This or @p src is NULL.
 * @exception KTX_INVALID_VALUE @p srcSize != the expected image size for the
 *                              specified level, layer & faceSlice.
 * @exception KTX_INVALID_OPERATION
 *                              No storage was allocated when the texture was
 *                              created.
 */
KTX_error_code
ktxTexture_SetImageFromStdioStream(ktxTexture* This, ktx_uint32_t level,
                                   ktx_uint32_t layer, ktx_uint32_t faceSlice,
                                   FILE* src, ktx_size_t srcSize)
{
    ktxStream srcstr;
    KTX_error_code result;
    
    result = ktxFileStream_construct(&srcstr, src, KTX_FALSE);
    if (result != KTX_SUCCESS)
        return result;
    result = ktxTexture_setImageFromStream(This, level, layer, faceSlice,
                                           &srcstr, srcSize);
    ktxFileStream_destruct(&srcstr);
    return result;
}

/**
 * @memberof ktxTexture
 * @~English
 * @brief Set image for level, layer, faceSlice from an image in memory.
 *
 * Uncompressed images in memory are expected to have their rows tightly packed
 * as is the norm for most image file formats. The copied image is padded as
 * necessary to achieve the KTX-specified row alignment. No padding is done if
 * the ktxTexture's @c isCompressed field is @c KTX_TRUE.
 *
 * Level, layer, faceSlice rather than offset are specified to enable some
 * validation.
 *
 * @warning Do not use @c memcpy for this as it will not pad when necessary.
 *
 * @param[in] This      pointer to the target ktxTexture object.
 * @param[in] level     mip level of the image to set.
 * @param[in] layer     array layer of the image to set.
 * @param[in] faceSlice cube map face or depth slice of the image to set.
 * @param[in] src       pointer to the image source in memory.
 * @param[in] srcSize   size of the source image in bytes.
 *
 * @return      KTX_SUCCESS on success, other KTX_* enum values on error.
 *
 * @exception KTX_INVALID_VALUE @p This or @p src is NULL.
 * @exception KTX_INVALID_VALUE @p srcSize != the expected image size for the
 *                              specified level, layer & faceSlice.
 * @exception KTX_INVALID_OPERATION
 *                              No storage was allocated when the texture was
 *                              created.
 */
KTX_error_code
ktxTexture_SetImageFromMemory(ktxTexture* This, ktx_uint32_t level,
                              ktx_uint32_t layer, ktx_uint32_t faceSlice,
                              const ktx_uint8_t* src, ktx_size_t srcSize)
{
    ktxStream srcstr;
    KTX_error_code result;
    
    result = ktxMemStream_construct_ro(&srcstr, src, srcSize);
    if (result != KTX_SUCCESS)
        return result;
    result = ktxTexture_setImageFromStream(This, level, layer, faceSlice,
                                           &srcstr, srcSize);
    ktxMemStream_destruct(&srcstr);
    return result;
}

/**
 * @internal
 * @memberof ktxTexture @private
 * @~English
 * @brief Write a ktxTexture object to a ktxStream in KTX format.
 *
 * @param[in] This      pointer to the target ktxTexture object.
 * @param[in] dststr    destination ktxStream.
 *
 * @return      KTX_SUCCESS on success, other KTX_* enum values on error.
 *
 * @exception KTX_INVALID_VALUE @p This or @p dststr is NULL.
 * @exception KTX_INVALID_OPERATION
 *                              The ktxTexture does not contain any image data.
 * @exception KTX_FILE_OVERFLOW The file exceeded the maximum size supported by
 *                              the system.
 * @exception KTX_FILE_WRITE_ERROR
 *                              An error occurred while writing the file.
 */
static KTX_error_code
ktxTexture_writeToStream(ktxTexture* This, ktxStream* dststr)
{
    KTX_header header = KTX_IDENTIFIER_REF;
    KTX_error_code result = KTX_SUCCESS;
    ktx_uint32_t kvdLen;
    ktx_uint8_t* pKvd;
    ktx_uint32_t level, levelOffset;
    
    if (!dststr) {
        return KTX_INVALID_VALUE;
    }
    
    if (This->pData == NULL)
        return KTX_INVALID_OPERATION;

    //endianess int.. if this comes out reversed, all of the other ints will too.
    header.endianness = KTX_ENDIAN_REF;
    header.glInternalformat = This->glInternalformat;
    header.glFormat = This->glFormat;
    header.glBaseInternalformat = This->glBaseInternalformat;
    header.glType = This->glType;
    header.glTypeSize = ktxTexture_glTypeSize(This);
    header.pixelWidth = This->baseWidth;
    header.pixelHeight = This->baseHeight;
    header.pixelDepth = This->baseDepth;
    header.numberOfArrayElements = This->isArray ? This->numLayers : 0;
    assert (This->isCubemap ? This->numFaces == 6 : This->numFaces == 1);
    header.numberOfFaces = This->numFaces;
    assert (This->generateMipmaps ? This->numLevels == 1 : This->numLevels >= 1);
    header.numberOfMipmapLevels = This->generateMipmaps ? 0 : This->numLevels;
    
    ktxHashList_Serialize(&This->kvDataHead, &kvdLen, &pKvd);
    header.bytesOfKeyValueData = kvdLen;

    //write header
    result = dststr->write(dststr, &header, sizeof(KTX_header), 1);
    if (result != KTX_SUCCESS)
        return result;
    
    //write keyValueData
    if (kvdLen != 0) {
        assert(pKvd != NULL);

        result = dststr->write(dststr, pKvd, 1, kvdLen);
        free(pKvd);
        if (result != KTX_SUCCESS)
            return result;
    }
    
    /* Write the image data */
    for (level = 0, levelOffset=0; level < This->numLevels; ++level)
    {
        ktx_uint32_t faceLodSize, layer, levelDepth, numImages;
        ktx_size_t imageSize;
        
        faceLodSize = (ktx_uint32_t)ktxTexture_faceLodSize(This, level);
        imageSize = ktxTexture_GetImageSize(This, level);
        levelDepth = MAX(1, This->baseDepth >> level);
        if (This->isCubemap && !This->isArray)
            numImages = This->numFaces;
        else
            numImages = This->isCubemap ? This->numFaces : levelDepth;
        
        result = dststr->write(dststr, &faceLodSize, sizeof(faceLodSize), 1);
        if (result != KTX_SUCCESS)
            goto cleanup;

        for (layer = 0; layer < This->numLayers; layer++) {
            ktx_uint32_t faceSlice;
            
            for (faceSlice = 0; faceSlice < numImages; faceSlice++) {
                result = dststr->write(dststr, This->pData + levelOffset,
                                       imageSize, 1);
                levelOffset += (ktx_uint32_t)imageSize;
            }
        }
    }
    
cleanup:
    return result;
}

/**
 * @memberof ktxTexture
 * @~English
 * @brief Write a ktxTexture object to a stdio stream in KTX format.
 *
 * @param[in] This      pointer to the target ktxTexture object.
 * @param[in] dstsstr   destination stdio stream.
 *
 * @return      KTX_SUCCESS on success, other KTX_* enum values on error.
 *
 * @exception KTX_INVALID_VALUE @p This or @p dstsstr is NULL.
 * @exception KTX_INVALID_OPERATION
 *                              The ktxTexture does not contain any image data.
 * @exception KTX_FILE_OVERFLOW The file exceeded the maximum size supported by
 *                              the system.
 * @exception KTX_FILE_WRITE_ERROR
 *                              An error occurred while writing the file.
 */
KTX_error_code
ktxTexture_WriteToStdioStream(ktxTexture* This, FILE* dstsstr)
{
    ktxStream stream;
    KTX_error_code result = KTX_SUCCESS;
    
    if (!This)
        return KTX_INVALID_VALUE;
    
    result = ktxFileStream_construct(&stream, dstsstr, KTX_FALSE);
    if (result != KTX_SUCCESS)
        return result;
    
    return ktxTexture_writeToStream(This, &stream);
}

/**
 * @memberof ktxTexture
 * @~English
 * @brief Write a ktxTexture object to a named file in KTX format.
 *
 * @param[in] This      pointer to the target ktxTexture object.
 * @param[in] dstname   destination file name.
 *
 * @return      KTX_SUCCESS on success, other KTX_* enum values on error.
 *
 * @exception KTX_INVALID_VALUE @p This or @p dstname is NULL.
 * @exception KTX_INVALID_OPERATION
 *                              The ktxTexture does not contain any image data.
 * @exception KTX_FILE_OVERFLOW The file exceeded the maximum size supported by
 *                              the system.
 * @exception KTX_FILE_WRITE_ERROR
 *                              An error occurred while writing the file.
 */
KTX_error_code
ktxTexture_WriteToNamedFile(ktxTexture* This, const char* const dstname)
{
    KTX_error_code result;
    FILE* dst;

    if (!This)
        return KTX_INVALID_VALUE;

    dst = fopen(dstname, "wb");
    if (dst) {
        result = ktxTexture_WriteToStdioStream(This, dst);
        fclose(dst);
    } else
        result = KTX_FILE_OPEN_FAILED;
    
    return result;
}

/**
 * @memberof ktxTexture
 * @~English
 * @brief Write a ktxTexture object to block of memory in KTX format.
 *
 * Memory is allocated by the function and the caller is responsible for
 * freeing it.
 *
 * @param[in]     This       pointer to the target ktxTexture object.
 * @param[in,out] ppDstBytes pointer to location to write the address of
 *                           the destination memory. The Application is
 *                           responsible for freeing this memory.
 * @param[in,out] pSize      pointer to location to write the size in bytes of
 *                           the KTX data.
 *
 * @return      KTX_SUCCESS on success, other KTX_* enum values on error.
 *
 * @exception KTX_INVALID_VALUE @p This, @p ppDstBytes or @p pSize is NULL.
 * @exception KTX_INVALID_OPERATION
 *                              The ktxTexture does not contain any image data.
 * @exception KTX_FILE_OVERFLOW The file exceeded the maximum size supported by
 *                              the system.
 * @exception KTX_FILE_WRITE_ERROR
 *                              An error occurred while writing the file.
 */
KTX_error_code
ktxTexture_WriteToMemory(ktxTexture* This,
                         ktx_uint8_t** ppDstBytes, ktx_size_t* pSize)
{
    struct ktxStream dststr;
    KTX_error_code result;
    ktx_size_t strSize;

    if (!This || !ppDstBytes || !pSize)
        return KTX_INVALID_VALUE;

    *ppDstBytes = NULL;
    
    result = ktxMemStream_construct(&dststr, KTX_FALSE);
    if (result != KTX_SUCCESS)
        return result;
    
    result = ktxTexture_writeToStream(This, &dststr);
    if(result != KTX_SUCCESS)
    {
        ktxMemStream_destruct(&dststr);
        return result;
    }
    
    ktxMemStream_getdata(&dststr, ppDstBytes);
    dststr.getsize(&dststr, &strSize);
    *pSize = (GLsizei)strSize;
    /* This function does not free the memory pointed at by the
     * value obtained from ktxMemStream_getdata() thanks to the
     * KTX_FALSE passed to the constructor above.
     */
    ktxMemStream_destruct(&dststr);
    return KTX_SUCCESS;

}

/** @} */

