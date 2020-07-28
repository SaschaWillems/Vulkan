/* -*- tab-width: 4; -*- */
/* vi: set sw=2 ts=4 expandtab: */

#ifndef KTX_H_A55A6F00956F42F3A137C11929827FE1
#define KTX_H_A55A6F00956F42F3A137C11929827FE1

/*
 * ©2010-2018 The Khronos Group, Inc.
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
 *
 * See the accompanying LICENSE.md for licensing details for all files in
 * the KTX library and KTX loader tests.
 */

/**
 * @file
 * @~English
 *
 * @brief Declares the public functions and structures of the
 *        KTX API.
 *
 * @author Mark Callow, Edgewise Consulting and while at HI Corporation
 * @author Based on original work by Georg Kolling, Imagination Technology
 *
 * @version 3.0
 *
 * @todo Find a way so that applications do not have to define KTX_OPENGL{,_ES*}
 *       when using the library.
 */

#include <stdio.h>
#include <stdbool.h>

/* To avoid including <KHR/khrplatform.h> define our own types. */
typedef unsigned char ktx_uint8_t;
typedef bool ktx_bool_t;
#ifdef _MSC_VER
typedef unsigned short ktx_uint16_t;
typedef   signed short ktx_int16_t;
typedef unsigned int   ktx_uint32_t;
typedef   signed int   ktx_int32_t;
typedef       size_t   ktx_size_t;
#else
#include <stdint.h>
typedef uint16_t ktx_uint16_t;
typedef  int16_t ktx_int16_t;
typedef uint32_t ktx_uint32_t;
typedef  int32_t ktx_int32_t;
typedef   size_t ktx_size_t;
#endif

/* This will cause compilation to fail if size of uint32 != 4. */
typedef unsigned char ktx_uint32_t_SIZE_ASSERT[sizeof(ktx_uint32_t) == 4];

/*
 * This #if allows libktx to be compiled with strict c99. It avoids
 * compiler warnings or even errors when a gl.h is already included.
 * "Redefinition of (type) is a c11 feature". Obviously this doesn't help if
 * gl.h comes after. However nobody has complained about the unguarded typedefs
 * since they were introduced so this is unlikely to be a problem in practice.
 * Presumably everybody is using platform default compilers not c99 or else
 * they are using C++.
 */
#if !defined(GL_NO_ERROR)
  /*
   * To avoid having to including gl.h ...
   */
  typedef unsigned char GLboolean;
  typedef unsigned int GLenum;
  typedef int GLint;
  typedef int GLsizei;
  typedef unsigned int GLuint;
  typedef unsigned char GLubyte;
#endif

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @~English
 * @brief Key String for standard orientation value.
 */
#define KTX_ORIENTATION_KEY "KTXorientation"
/**
 * @~English
 * @brief Standard format for 2D orientation value.
 */
#define KTX_ORIENTATION2_FMT "S=%c,T=%c"
/**
 * @~English
 * @brief Standard format for 3D orientation value.
 */
#define KTX_ORIENTATION3_FMT "S=%c,T=%c,R=%c"
/**
 * @~English
 * @brief Required unpack alignment
 */
#define KTX_GL_UNPACK_ALIGNMENT 4
    
#define KTX_TRUE  true
#define KTX_FALSE false

/**
 * @~English
 * @brief Error codes returned by library functions.
 */
typedef enum KTX_error_code_t {
    KTX_SUCCESS = 0,         /*!< Operation was successful. */
    KTX_FILE_DATA_ERROR,     /*!< The data in the file is inconsistent with the spec. */
    KTX_FILE_OPEN_FAILED,    /*!< The target file could not be opened. */
    KTX_FILE_OVERFLOW,       /*!< The operation would exceed the max file size. */
    KTX_FILE_READ_ERROR,     /*!< An error occurred while reading from the file. */
    KTX_FILE_SEEK_ERROR,     /*!< An error occurred while seeking in the file. */
    KTX_FILE_UNEXPECTED_EOF, /*!< File does not have enough data to satisfy request. */
    KTX_FILE_WRITE_ERROR,    /*!< An error occurred while writing to the file. */
    KTX_GL_ERROR,            /*!< GL operations resulted in an error. */
    KTX_INVALID_OPERATION,   /*!< The operation is not allowed in the current state. */
    KTX_INVALID_VALUE,       /*!< A parameter value was not valid */
    KTX_NOT_FOUND,           /*!< Requested key was not found */
    KTX_OUT_OF_MEMORY,       /*!< Not enough memory to complete the operation. */
    KTX_UNKNOWN_FILE_FORMAT, /*!< The file not a KTX file */
    KTX_UNSUPPORTED_TEXTURE_TYPE, /*!< The KTX file specifies an unsupported texture type. */
} KTX_error_code;

#define KTX_IDENTIFIER_REF  { 0xAB, 0x4B, 0x54, 0x58, 0x20, 0x31, 0x31, 0xBB, 0x0D, 0x0A, 0x1A, 0x0A }
#define KTX_ENDIAN_REF      (0x04030201)
#define KTX_ENDIAN_REF_REV  (0x01020304)
#define KTX_HEADER_SIZE     (64)

/**
 * @~English
 * @brief Result codes returned by library functions.
 */
 typedef enum KTX_error_code_t ktxResult;

/**
 * @class ktxHashList
 * @~English
 * @brief Opaque handle to a ktxHashList.
 */
typedef struct ktxKVListEntry* ktxHashList;

/**
 * @class ktxTexture
 * @~English
 * @brief Class representing a texture.
 *
 * ktxTextures should be created only by one of the ktxTexture_Create*
 * functions and these fields should be considered read-only.
 */
typedef struct {
    ktx_uint32_t glFormat; /*!< Format of the texture data, e.g., GL_RGB. */
    ktx_uint32_t glInternalformat; /*!< Internal format of the texture data,
                                        e.g., GL_RGB8. */
    ktx_uint32_t glBaseInternalformat; /*!< Base format of the texture data,
                                            e.g., GL_RGB. */
    ktx_uint32_t glType; /*!< Type of the texture data, e.g, GL_UNSIGNED_BYTE.*/
      ktx_bool_t isArray; /*!< KTX_TRUE if the texture is an array texture, i.e,
                               a GL_TEXTURE_*_ARRAY target is to be used. */
      ktx_bool_t isCubemap; /*!< KTX_TRUE if the texture is a cubemap or
                                 cubemap array. */
      ktx_bool_t isCompressed; /*!< KTX_TRUE if @c glInternalFormat is that of
                                    a compressed texture. */
      ktx_bool_t generateMipmaps; /*!< KTX_TRUE if mipmaps should be generated
                                       for the texture by ktxTexture_GLUpload()
                                       or ktx_Texture_VkUpload(). */
    ktx_uint32_t baseWidth;  /*!< Width of the base level of the texture. */
    ktx_uint32_t baseHeight; /*!< Height of the base level of the texture. */
    ktx_uint32_t baseDepth;  /*!< Depth of the base level of the texture. */
    ktx_uint32_t numDimensions; /*!< Number of dimensions in the texture: 1, 2
                                     or 3. */
    ktx_uint32_t numLevels; /*!< Number of mip levels in the texture. Should be
                                 1, if @c generateMipmaps is KTX_TRUE. Can be
                                 less than a full pyramid but always starts at
                                 the base level. */
    ktx_uint32_t numLayers; /*!< Number of array layers in the texture. */
    ktx_uint32_t numFaces; /*!< Number of faces, 6 for cube maps, 1 otherwise.*/
     ktxHashList kvDataHead; /*!< Head of the hash list of metadata. */
    ktx_uint32_t kvDataLen; /*!< Length of the metadata, if it has been
                                 extracted in its raw form, otherwise 0. */
    ktx_uint8_t* kvData; /*!< Pointer to the metadata, if it has been extracted
                              in its raw form, otherwise NULL. */
      ktx_size_t dataSize; /*!< Length of the image data in bytes. */
    ktx_uint8_t* pData; /*!< Pointer to the image data. */
} ktxTexture;

    
/**
 * @memberof ktxTexture
 * @~English
 * @brief Structure for passing texture information to ktxTexture_Create().
 *
 * @sa ktxTexture_Create()
 */
typedef struct
{
    ktx_uint32_t glInternalformat; /*!< Internal format for the texture, e.g.,
                                        GL_RGB8. */
    ktx_uint32_t baseWidth;  /*!< Width of the base level of the texture. */
    ktx_uint32_t baseHeight; /*!< Height of the base level of the texture. */
    ktx_uint32_t baseDepth;  /*!< Depth of the base level of the texture. */
    ktx_uint32_t numDimensions; /*!< Number of dimensions in the texture, 1, 2
                                     or 3. */
    ktx_uint32_t numLevels; /*!< Number of mip levels in the texture. Should be
                                 1 if @c generateMipmaps is KTX_TRUE; */
    ktx_uint32_t numLayers; /*!< Number of array layers in the texture. */
    ktx_uint32_t numFaces;  /*!< Number of faces: 6 for cube maps, 1 otherwise. */
    ktx_bool_t   isArray;  /*!< Set to KTX_TRUE if the texture is to be an
                                array texture. Means OpenGL will use a
                                GL_TEXTURE_*_ARRAY target. */
    ktx_bool_t   generateMipmaps; /*!< Set to KTX_TRUE if mipmaps should be
                                       generated for the texture when loading
                                       into OpenGL. */
} ktxTextureCreateInfo;

/**
 * @memberof ktxTexture
 * @~English
 * @brief Enum for requesting, or not, allocation of storage for images.
 *
 * @sa ktxTexture_Create()
 */
typedef enum {
    KTX_TEXTURE_CREATE_NO_STORAGE = 0,  /*!< Don't allocate any image storage. */
    KTX_TEXTURE_CREATE_ALLOC_STORAGE = 1 /*!< Allocate image storage. */
} ktxTextureCreateStorageEnum;

/**
 * @memberof ktxTexture
 * @~English
 * @brief Flags for requesting services during creation.
 *
 * @sa ktxTexture_CreateFrom*
 */
enum ktxTextureCreateFlagBits {
    KTX_TEXTURE_CREATE_NO_FLAGS = 0x00,
    KTX_TEXTURE_CREATE_LOAD_IMAGE_DATA_BIT = 0x01,
                                   /*!< Load the images from the KTX source. */
    KTX_TEXTURE_CREATE_RAW_KVDATA_BIT = 0x02,
                                   /*!< Load the raw key-value data instead of
                                        creating a @c ktxHashList from it. */
    KTX_TEXTURE_CREATE_SKIP_KVDATA_BIT = 0x04
                                   /*!< Skip any key-value data. This overrides
                                        the RAW_KVDATA_BIT. */
};
/**
 * @memberof ktxTexture
 * @~English
 * @brief Type for TextureCreateFlags parameters.
 *
 * @sa ktxTexture_CreateFrom*()
 */
typedef ktx_uint32_t ktxTextureCreateFlags;

#define KTXAPIENTRY
#define KTXAPIENTRYP KTXAPIENTRY *
/**
 * @memberof ktxTexture
 * @~English
 * @brief Signature of function called by the <tt>ktxTexture_Iterate*</tt>
 *        functions to receive image data.
 *
 * The function parameters are used to pass values which change for each image.
 * Obtain values which are uniform across all images from the @c ktxTexture
 * object.
 *
 * @param [in] miplevel        MIP level from 0 to the max level which is
 *                             dependent on the texture size.
 * @param [in] face            usually 0; for cube maps, one of the 6 cube
 *                             faces in the order +X, -X, +Y, -Y, +Z, -Z,
 *                             0 to 5.
 * @param [in] width           width of the image.
 * @param [in] height          height of the image or, for 1D textures
 *                             textures, 1.
 * @param [in] depth           depth of the image or, for 1D & 2D
 *                             textures, 1.
 * @param [in] faceLodSize     number of bytes of data pointed at by
 *                             @p pixels.
 * @param [in] pixels          pointer to the image data.
 * @param [in,out] userdata    pointer for the application to pass data to and
 *                             from the callback function.
 */
/* Don't use KTXAPIENTRYP to avoid a Doxygen bug. */
typedef KTX_error_code (KTXAPIENTRY* PFNKTXITERCB)(int miplevel, int face,
                                               int width, int height, int depth,
                                               ktx_uint32_t faceLodSize,
                                               void* pixels,
                                               void* userdata);

/*
 * See the implementation files for the full documentation of the following
 * functions.
 */

/*
 * Creates an empty ktxTexture object with the characteristics described
 * by createInfo.
 */
KTX_error_code
ktxTexture_Create(ktxTextureCreateInfo* createInfo,
                  ktxTextureCreateStorageEnum storageAllocation,
                  ktxTexture** newTex);

/*
 * Creates a ktxTexture from a stdio stream reading from a KTX source.
 */
KTX_error_code
ktxTexture_CreateFromStdioStream(FILE* stdioStream,
                                 ktxTextureCreateFlags createFlags,
                                 ktxTexture** newTex);

/*
 * Creates a ktxTexture from a named file containing KTX data.
 */
KTX_error_code
ktxTexture_CreateFromNamedFile(const char* const filename,
                               ktxTextureCreateFlags createFlags,
                               ktxTexture** newTex);

/*
 * Creates a ktxTexture from a block of memory containing KTX-formatted data.
 */
KTX_error_code
ktxTexture_CreateFromMemory(const ktx_uint8_t* bytes, ktx_size_t size,
                            ktxTextureCreateFlags createFlags,
                            ktxTexture** newTex);
/*
 * Destroys a ktxTexture object.
 */
void
ktxTexture_Destroy(ktxTexture* This);

/*
 * Returns a pointer to the image data of a ktxTexture object.
 */
ktx_uint8_t*
ktxTexture_GetData(ktxTexture* This);

/*
 * Returns the offset of the image for the specified mip level, array layer
 * and face or depth slice within the image data of a ktxTexture object.
 */
KTX_error_code
ktxTexture_GetImageOffset(ktxTexture* This, ktx_uint32_t level,
                          ktx_uint32_t layer, ktx_uint32_t faceSlice,
                          ktx_size_t* pOffset);

/*
 * Returns the pitch of a row of an image at the specified level.
 * Similar to the rowPitch in a VkSubResourceLayout.
 */
 ktx_uint32_t
 ktxTexture_GetRowPitch(ktxTexture* This, ktx_uint32_t level);

 /*
  * Return the element size of the texture's images.
  */
 ktx_uint32_t
 ktxTexture_GetElementSize(ktxTexture* This);

/*
 * Returns the size of all the image data of a ktxTexture object in bytes.
 */
ktx_size_t
ktxTexture_GetSize(ktxTexture* This);
    
/*
 * Returns the size of an image at the specified level.
 */
 ktx_size_t
 ktxTexture_GetImageSize(ktxTexture* This, ktx_uint32_t level);

/*
 * Uploads the image data from a ktxTexture object to an OpenGL {,ES} texture
 * object.
 */
KTX_error_code
ktxTexture_GLUpload(ktxTexture* This, GLuint* pTexture, GLenum* pTarget,
                    GLenum* pGlerror);

/*
 * Loads the image data into a ktxTexture object from the KTX-formatted source.
 * Used when the image data was not loaded during ktxTexture_CreateFrom*.
 */
KTX_error_code
ktxTexture_LoadImageData(ktxTexture* This,
                         ktx_uint8_t* pBuffer,
                         ktx_size_t bufSize);

/*
 * Iterates over the already loaded level-faces in a ktxTexture object.
 * iterCb is called for each level-face.
 */
KTX_error_code
ktxTexture_IterateLevelFaces(ktxTexture* super, PFNKTXITERCB iterCb,
                             void* userdata);

/*
 * Iterates over the level-faces of a ktxTexture object, loading each from
 * the KTX-formatted source then calling iterCb.
 */
KTX_error_code
ktxTexture_IterateLoadLevelFaces(ktxTexture* super, PFNKTXITERCB iterCb,
                                 void* userdata);
/*
 * Iterates over the already loaded levels in a ktxTexture object.
 * iterCb is called for each level. The data passed to iterCb
 * includes all faces for each level.
 */
KTX_error_code
ktxTexture_IterateLevels(ktxTexture* This, PFNKTXITERCB iterCb,
                         void* userdata);

/*
 * Sets the image for the specified level, layer & faceSlice within a
 * ktxTexture object from packed image data in memory. The destination image
 * data is padded to the KTX specified row alignment of 4, if necessary.
 */
KTX_error_code
ktxTexture_SetImageFromMemory(ktxTexture* This,ktx_uint32_t level,
                              ktx_uint32_t layer, ktx_uint32_t faceSlice,
                              const ktx_uint8_t* src, ktx_size_t srcSize);

/*
 * Sets the image for the specified level, layer & faceSlice within a
 * ktxTexture object from a stdio stream reading from a KTX source. The
 * destination image data is padded to the KTX specified row alignment of 4,
 * if necessary.
 */
KTX_error_code
ktxTexture_SetImageFromStdioStream(ktxTexture* This, ktx_uint32_t level,
                                   ktx_uint32_t layer, ktx_uint32_t faceSlice,
                                   FILE* src, ktx_size_t srcSize);

/*
 * Write a ktxTexture object to a stdio stream in KTX format.
 */
KTX_error_code
ktxTexture_WriteToStdioStream(ktxTexture* This, FILE* dstsstr);

/*
 * Write a ktxTexture object to a named file in KTX format.
 */
KTX_error_code
ktxTexture_WriteToNamedFile(ktxTexture* This, const char* const dstname);

/*
 * Write a ktxTexture object to a block of memory in KTX format.
 */
KTX_error_code
ktxTexture_WriteToMemory(ktxTexture* This,
                         ktx_uint8_t** bytes, ktx_size_t* size);

/*
 * Returns a string corresponding to a KTX error code.
 */
const char* const ktxErrorString(KTX_error_code error);

KTX_error_code ktxHashList_Create(ktxHashList** ppHl);
void ktxHashList_Construct(ktxHashList* pHl);
void ktxHashList_Destroy(ktxHashList* head);
void ktxHashList_Destruct(ktxHashList* head);
/*
 * Adds a key-value pair to a hash list.
 */
KTX_error_code
ktxHashList_AddKVPair(ktxHashList* pHead, const char* key,
                      unsigned int valueLen, const void* value);

/*
 * Looks up a key and returns the value.
 */
KTX_error_code
ktxHashList_FindValue(ktxHashList* pHead, const char* key,
                      unsigned int* pValueLen, void** pValue);

/*
 * Serializes the hash table to a block of memory suitable for
 * writing to a KTX file.
 */
KTX_error_code
ktxHashList_Serialize(ktxHashList* pHead,
                      unsigned int* kvdLen, unsigned char** kvd);


/*
 * Creates a hash table from the serialized data read from a
 * a KTX file.
 */
KTX_error_code
ktxHashList_Deserialize(ktxHashList* pHead, unsigned int kvdLen, void* kvd);
    

/*===========================================================*
 * For Versions 1 and 2 compatibility                        *
 *===========================================================*/

/**
 * @~English
 * @deprecated Use struct ktxTexture
 * @brief Structure used by load functions to return texture dimensions
 */
typedef struct KTX_dimensions {
    GLsizei width;  /*!< Width in texels. */
    GLsizei height; /*!< Height in texels. */
    GLsizei depth;  /*!< Depth in texels. */
} KTX_dimensions;

/**
 * @~English
 * @brief Structure to pass texture information to ktxWriteKTX.
 * @deprecated Use ktxTexture_Create() and @c ktxTextureCreateInfo
 *
 * Retained for backward compatibility with Versions 1 & 2.
 */
typedef struct KTX_texture_info
{
    ktx_uint32_t glType;
    ktx_uint32_t glTypeSize;
    ktx_uint32_t glFormat;
    ktx_uint32_t glInternalFormat;
    ktx_uint32_t glBaseInternalFormat;
    ktx_uint32_t pixelWidth;
     ktx_uint32_t pixelHeight;
    ktx_uint32_t pixelDepth;
    ktx_uint32_t numberOfArrayElements;
    ktx_uint32_t numberOfFaces;
    ktx_uint32_t numberOfMipmapLevels;
} KTX_texture_info;

/**
 * @var KTX_texture_info::glType
 * @~English
 * @brief The type of the image data.
 *
 * Values are the same as in the @p type parameter of
 * glTexImage*D. Must be 0 for compressed images.
 */
/**
 * @var KTX_texture_info::glTypeSize;
 * @~English
 * @brief The data type size to be used in case of endianness
 *        conversion.
 *
 * This value is used in the event conversion is required when the
 * KTX file is loaded. It should be the size in bytes corresponding
 * to glType. Must be 1 for compressed images.
 */
/**
 * @var KTX_texture_info::glFormat;
 * @~English
 * @brief The format of the image(s).
 *
 * Values are the same as in the format parameter
 * of glTexImage*D. Must be 0 for compressed images.
 */
/**
 * @var KTX_texture_info::glInternalFormat;
 * @~English
 * @brief The internalformat of the image(s).
 *
 * Values are the same as for the internalformat parameter of
 * glTexImage*2D. Note: it will not be used when a KTX file
 * containing an uncompressed texture is loaded into OpenGL ES.
 */
/**
 * @var KTX_texture_info::glBaseInternalFormat;
 * @~English
 * @brief The base internalformat of the image(s)
 *
 * For non-compressed textures, should be the same as glFormat.
 * For compressed textures specifies the base internal, e.g.
 * GL_RGB, GL_RGBA.
 */
/**
 * @var KTX_texture_info::pixelWidth;
 * @~English
 * @brief Width of the image for texture level 0, in pixels.
 */
/**
 * @var KTX_texture_info::pixelHeight;
 * @~English
 * @brief Height of the texture image for level 0, in pixels.
 *
 * Must be 0 for 1D textures.
 */
/**
 * @var KTX_texture_info::pixelDepth;
 * @~English
 * @brief Depth of the texture image for level 0, in pixels.
 *
 * Must be 0 for 1D, 2D and cube textures.
 */
/**
 * @var KTX_texture_info::numberOfArrayElements;
 * @~English
 * @brief The number of array elements.
 *
 * Must be 0 if not an array texture.
 */
/**
 * @var KTX_texture_info::numberOfFaces;
 * @~English
 * @brief The number of cubemap faces.
 *
 * Must be 6 for cubemaps and cubemap arrays, 1 otherwise. Cubemap
 * faces must be provided in the order: +X, -X, +Y, -Y, +Z, -Z.
 */
/**
 * @var KTX_texture_info::numberOfMipmapLevels;
 * @~English
 * @brief The number of mipmap levels.
 *
 * 1 for non-mipmapped texture. 0 indicates that a full mipmap pyramid should
 * be generated from level 0 at load time (this is usually not allowed for
 * compressed formats). Mipmaps must be provided in order from largest size to
 * smallest size. The first mipmap level is always level 0.
 */

/**
 * @~English
 * @deprecated Use ktxTexture_Create() and @c ktxTexture_SetImageFromMemory()
 *             or ktxTexture_SetImageFromStdioStream().
 * @brief Structure used to pass image data to ktxWriteKTX.
 *
 * @sa ktxTextureCreate()
 * @sa ktxTexture_SetImageFromMemory()
 * @sa ktxTexture_SetImageFromStdioStream()
 *
 * Retained for backward compatibility with Versions 1 & 2.
 */
typedef struct KTX_image_info {
    GLsizei size;    /*!< Size of the image data in bytes. */
    GLubyte* data;  /*!< Pointer to the image data. */
} KTX_image_info;

/*
 * Loads a texture from a stdio FILE.
 */
KTX_error_code
ktxLoadTextureF(FILE*, GLuint* pTexture, GLenum* pTarget,
                KTX_dimensions* pDimensions, GLboolean* pIsMipmapped,
                GLenum* pGlerror,
                unsigned int* pKvdLen, unsigned char** ppKvd);

/*
 * Loads a texture from a KTX file on disk.
 */
KTX_error_code
ktxLoadTextureN(const char* const filename, GLuint* pTexture, GLenum* pTarget,
                KTX_dimensions* pDimensions, GLboolean* pIsMipmapped,
                GLenum* pGlerror,
                unsigned int* pKvdLen, unsigned char** ppKvd);

/*
 * Loads a texture from a KTX file in memory.
 */
KTX_error_code
ktxLoadTextureM(const void* bytes, GLsizei size, GLuint* pTexture,
                GLenum* pTarget, KTX_dimensions* pDimensions,
                GLboolean* pIsMipmapped, GLenum* pGlerror,
                unsigned int* pKvdLen, unsigned char** ppKvd);

/**
 * @class KTX_hash_table
 * @~English
 * @deprecated Use @c ktxHashList.
 * @brief Opaque handle to a hash table.
 */
typedef ktxHashList* KTX_hash_table;

/*
 * @deprecated Use ktxHashList_Create().
 */
KTX_hash_table ktxHashTable_Create(void);

/**
 * @~English
 * @deprecated Use ktxHashList_Destroy().
 * @brief Destroy a KTX_hash_table.
 *
 * Should be documented as a member of KTX_hash_table but a doxygen limitation
 * prevents that.
 */
#define ktxHashTable_Destroy(a) ktxHashList_Destroy(a);
/**
 * @~English
 * @deprecated Use ktxHashList_AddKVPair().
 * @brief Add a key-value pair to a KTX_hash_table.
 *
 * Should be documented as a member of KTX_hash_table but a doxygen limitation
 * prevents that.
 */
#define ktxHashTable_AddKVPair(a, b, c, d) ktxHashList_AddKVPair(a, b, c, d)
/**
 * @~English
 * @deprecated Use ktxHashList_FindValue().
 * @brief Looks up a key and returns its value.
 *
 * Should be documented as a member of KTX_hash_table but a doxygen limitation
 * prevents that.
 */
#define ktxHashTable_FindValue(a, b, c, d) ktxHashList_FindValue(a, b, c, d)

/*
 * @deprecated Use ktxHashList_Serialize().
 */
KTX_error_code
ktxHashTable_Serialize(KTX_hash_table This,
                       unsigned int* kvdLen, unsigned char** kvd);

/*
 * @deprecated Use ktxHashList_Deserialize().
 */
KTX_error_code
ktxHashTable_Deserialize(unsigned int kvdLen, void* pKvd, KTX_hash_table* pHt);

/*
 * Writes a KTX file using supplied data.
 */
KTX_error_code
ktxWriteKTXF(FILE* dst, const KTX_texture_info* imageInfo,
             GLsizei bytesOfKeyValueData, const void* keyValueData,
             GLuint numImages, KTX_image_info images[]);

/**
 * Writes a KTX file using supplied data.
 */
KTX_error_code
ktxWriteKTXN(const char* dstname, const KTX_texture_info* imageInfo,
             GLsizei bytesOfKeyValueData, const void* keyValueData,
             GLuint numImages, KTX_image_info images[]);

/*
 * Writes a KTX file into memory using supplied data.
 */
KTX_error_code
ktxWriteKTXM(unsigned char** dst, GLsizei* size,
             const KTX_texture_info* textureInfo, GLsizei bytesOfKeyValueData,
             const void* keyValueData, GLuint numImages,
             KTX_image_info images[]);

#ifdef __cplusplus
}
#endif

/**
@~English
@page libktx_history Revision History

@section v7 Version 3.0.1
Fixed:
@li GitHub issue #159: compile failure with recent Vulkan SDKs.
@li Incorrect mapping of GL DXT3 and DXT5 formats to Vulkan equivalents.
@li Incorrect BC4 blocksize.
@li Missing mapping of PVRTC formats from GL to Vulkan.
@li Incorrect block width and height calculations for sizes that are not
    a multiple of the block size.
@li Incorrect KTXorientation key in test images.

@section v6 Version 3.0
Added:
@li new ktxTexture object based API for reading KTX files without an OpenGL context.
@li Vulkan loader. @#include <ktxvulkan.h> to use it.

Changed:
@li ktx.h to not depend on KHR/khrplatform.h and GL{,ES*}/gl{corearb,}.h.
    Applications using OpenGL must now include these files themselves.
@li ktxLoadTexture[FMN], removing the hack of loading 1D textures as 2D textures
    when the OpenGL context does not support 1D textures.
    KTX_UNSUPPORTED_TEXTURE_TYPE is now returned.

@section v5 Version 2.0.2
Added:
@li Support for cubemap arrays.

Changed:
@li New build system

Fixed:
@li GitHub issue #40: failure to byte-swap key-value lengths.
@li GitHub issue #33: returning incorrect target when loading cubemaps.
@li GitHub PR #42: loading of texture arrays.
@li GitHub PR #41: compilation error when KTX_OPENGL_ES2=1 defined.
@li GitHub issue #39: stack-buffer-overflow in toktx
@li Don't use GL_EXTENSIONS on recent OpenGL versions.

@section v4 Version 2.0.1
Added:
@li CMake build files. Thanks to Pavel Rotjberg for the initial version.

Changed:
@li ktxWriteKTXF to check the validity of the type & format combinations
    passed to it.

Fixed:
@li Public Bugzilla <a href="http://www.khronos.org/bugzilla/show_bug.cgi?id=999">999</a>: 16-bit luminance texture cannot be written.
@li compile warnings from compilers stricter than MS Visual C++. Thanks to
    Pavel Rotjberg.

@section v3 Version 2.0
Added:
@li support for decoding ETC2 and EAC formats in the absence of a hardware
    decoder.
@li support for converting textures with legacy LUMINANCE, LUMINANCE_ALPHA,
    etc. formats to the equivalent R, RG, etc. format with an
    appropriate swizzle, when loading in OpenGL Core Profile contexts.
@li ktxErrorString function to return a string corresponding to an error code.
@li tests for ktxLoadTexture[FN] that run under OpenGL ES 3.0 and OpenGL 3.3.
    The latter includes an EGL on WGL wrapper that makes porting apps between
    OpenGL ES and OpenGL easier on Windows.
@li more texture formats to ktxLoadTexture[FN] and toktx tests.

Changed:
@li ktxLoadTexture[FMN] to discover the capabilities of the GL context at
    run time and load textures, or not, according to those capabilities.

Fixed:
@li failure of ktxWriteKTXF to pad image rows to 4 bytes as required by the KTX
    format.
@li ktxWriteKTXF exiting with KTX_FILE_WRITE_ERROR when attempting to write
    more than 1 byte of face-LOD padding.

Although there is only a very minor API change, the addition of ktxErrorString,
the functional changes are large enough to justify bumping the major revision
number.

@section v2 Version 1.0.1
Implemented ktxLoadTextureM.
Fixed the following:
@li Public Bugzilla <a href="http://www.khronos.org/bugzilla/show_bug.cgi?id=571">571</a>: crash when null passed for pIsMipmapped.
@li Public Bugzilla <a href="http://www.khronos.org/bugzilla/show_bug.cgi?id=572">572</a>: memory leak when unpacking ETC textures.
@li Public Bugzilla <a href="http://www.khronos.org/bugzilla/show_bug.cgi?id=573">573</a>: potential crash when unpacking ETC textures with unused padding pixels.
@li Public Bugzilla <a href="http://www.khronos.org/bugzilla/show_bug.cgi?id=576">576</a>: various small fixes.

Thanks to Krystian Bigaj for the ktxLoadTextureM implementation and these fixes.

@section v1 Version 1.0
Initial release.

*/

#endif /* KTX_H_A55A6F00956F42F3A137C11929827FE1 */
