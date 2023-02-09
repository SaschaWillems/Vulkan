/* -*- tab-width: 4; -*- */
/* vi: set sw=2 ts=4 expandtab: */

/* $Id: df002b39ae6b9b3b995e7633ff96acf0d285edcc $ */

/*
 * ©2010-2018 The khronos Group, Inc.
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
 * @file checkheader.c
 * @~English
 *
 * @brief Function to verify a KTX file header
 *
 * @author Mark Callow, HI Corporation
 */

/*
 * Author: Georg Kolling, Imagination Technology with modifications
 * by Mark Callow, HI Corporation.
 */
#include <assert.h>
#include <string.h>

#include "ktx.h"
#include "ktxint.h"

/**
 * @internal
 * @~English
 * @brief Check a KTX file header.
 *
 * As well as checking that the header identifies a KTX file, the function
 * sanity checks the values and returns information about the texture in a
 * struct KTX_supplementary_info.
 *
 * @param pHeader   pointer to the KTX header to check
 * @param pSuppInfo pointer to a KTX_supplementary_info structure in which to
 *                  return information about the texture.
 * 
 * @author Georg Kolling, Imagination Technology
 * @author Mark Callow, HI Corporation
 */
KTX_error_code _ktxCheckHeader(KTX_header* pHeader,
                               KTX_supplemental_info* pSuppInfo)
{
    ktx_uint8_t identifier_reference[12] = KTX_IDENTIFIER_REF;
    ktx_uint32_t max_dim;
    
    assert(pHeader != NULL && pSuppInfo != NULL);

    /* Compare identifier, is this a KTX file? */
    if (memcmp(pHeader->identifier, identifier_reference, 12) != 0)
    {
        return KTX_UNKNOWN_FILE_FORMAT;
    }

    if (pHeader->endianness == KTX_ENDIAN_REF_REV)
    {
        /* Convert endianness of pHeader fields. */
        _ktxSwapEndian32(&pHeader->glType, 12);

        if (pHeader->glTypeSize != 1 &&
            pHeader->glTypeSize != 2 &&
            pHeader->glTypeSize != 4)
        {
            /* Only 8-, 16-, and 32-bit types supported so far. */
            return KTX_FILE_DATA_ERROR;
        }
    }
    else if (pHeader->endianness != KTX_ENDIAN_REF)
    {
        return KTX_FILE_DATA_ERROR;
    }

    /* Check glType and glFormat */
    pSuppInfo->compressed = 0;
    if (pHeader->glType == 0 || pHeader->glFormat == 0)
    {
        if (pHeader->glType + pHeader->glFormat != 0)
        {
            /* either both or none of glType, glFormat must be zero */
            return KTX_FILE_DATA_ERROR;
        }
        pSuppInfo->compressed = 1;
    }
    
    if (pHeader->glFormat == pHeader->glInternalformat) {
        // glInternalFormat is either unsized (which is no longer and should
        // never have been supported by libktx) or glFormat is sized.
        return KTX_FILE_DATA_ERROR;
    }

    /* Check texture dimensions. KTX files can store 8 types of textures:
       1D, 2D, 3D, cube, and array variants of these. There is currently
       no GL extension for 3D array textures. */
    if ((pHeader->pixelWidth == 0) ||
        (pHeader->pixelDepth > 0 && pHeader->pixelHeight == 0))
    {
        /* texture must have width */
        /* texture must have height if it has depth */
        return KTX_FILE_DATA_ERROR; 
    }

    
    if (pHeader->pixelDepth > 0)
    {
        if (pHeader->numberOfArrayElements > 0)
        {
            /* No 3D array textures yet. */
            return KTX_UNSUPPORTED_TEXTURE_TYPE;
        }
        pSuppInfo->textureDimension = 3;
    }
    else if (pHeader->pixelHeight > 0)
    {
        pSuppInfo->textureDimension = 2;
    }
    else
    {
        pSuppInfo->textureDimension = 1;
    }

    if (pHeader->numberOfFaces == 6)
    {
        if (pSuppInfo->textureDimension != 2)
        {
            /* cube map needs 2D faces */
            return KTX_FILE_DATA_ERROR;
        }
    }
    else if (pHeader->numberOfFaces != 1)
    {
        /* numberOfFaces must be either 1 or 6 */
        return KTX_FILE_DATA_ERROR;
    }
    
    /* Check number of mipmap levels */
    if (pHeader->numberOfMipmapLevels == 0)
    {
        pSuppInfo->generateMipmaps = 1;
        pHeader->numberOfMipmapLevels = 1;
    }
    else
    {
        pSuppInfo->generateMipmaps = 0;
    }

    /* This test works for arrays too because height or depth will be 0. */
    max_dim = MAX(MAX(pHeader->pixelWidth, pHeader->pixelHeight), pHeader->pixelDepth);
    if (max_dim < ((ktx_uint32_t)1 << (pHeader->numberOfMipmapLevels - 1)))
    {
        /* Can't have more mip levels than 1 + log2(max(width, height, depth)) */
        return KTX_FILE_DATA_ERROR;
    }

    return KTX_SUCCESS;
}
