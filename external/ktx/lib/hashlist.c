/* -*- tab-width: 4; -*- */
/* vi: set sw=2 ts=4 expandtab: */

/*
 * Copyright (c) 2010-2018 The Khronos Group Inc.
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
 * @file hashlist.c
 * @~English
 *
 * @brief Functions for creating and using a hash list of key-value
 *        pairs.
 *
 * @author Mark Callow, HI Corporation
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

// This is to avoid compile warnings. strlen is defined as returning
// size_t and is used by the uthash macros. This avoids having to
// make changes to uthash and a bunch of casts in this file. The
// casts would be required because the key and value lengths in KTX
// are specified as 4 byte quantities so we can't change _keyAndValue
// below to use size_t.
#define strlen(x) ((unsigned int)strlen(x))

#include "uthash.h"

#include "ktx.h"
#include "ktxint.h"


/**
 * @internal
 * @struct ktxKVListEntry
 * @brief Hash list entry structure
 */
typedef struct ktxKVListEntry {
    unsigned int keyLen;    /*!< Length of the key */
    char* key;              /*!< Pointer to key string */
    unsigned int valueLen;  /*!< Length of the value */
    void* value;            /*!< Pointer to the value */
    UT_hash_handle hh;      /*!< handle used by UT hash */
} ktxKVListEntry;


/**
 * @memberof ktxHashList @public
 * @~English
 * @brief Construct an empty hash list for storing key-value pairs.
 *
 * @param [in] pHead pointer to the location to write the list head.
 */
void
ktxHashList_Construct(ktxHashList* pHead)
{
    *pHead = NULL;
}


/**
 * @memberof ktxHashList @public
 * @~English
 * @brief Destruct a hash list.
 *
 * All memory associated with the hash list's keys and values
 * is freed.
 *
 * @param [in] pHead pointer to the hash list to be destroyed.
 */
void
ktxHashList_Destruct(ktxHashList* pHead)
{
    ktxKVListEntry* kv;
    ktxKVListEntry* head = *pHead;
    
    for(kv = head; kv != NULL;) {
        ktxKVListEntry* tmp = (ktxKVListEntry*)kv->hh.next;
        HASH_DELETE(hh, head, kv);
        free(kv);
        kv = tmp;
    }
}
/**
 * @memberof ktxHashList @public
 * @~English
 * @brief Create an empty hash list for storing key-value pairs.
 *
 * @param [in,out] ppHl address of a variable in which to set a pointer to
 *                 the newly created hash list.
 *
 * @return KTX_SUCCESS or one of the following error codes.
 * @exception KTX_OUT_OF_MEMORY if not enough memory.
 */
KTX_error_code
ktxHashList_Create(ktxHashList** ppHl)
{
    ktxHashList* hl = (ktxHashList*)malloc(sizeof (ktxKVListEntry*));
    if (hl == NULL)
        return KTX_OUT_OF_MEMORY;
    
    ktxHashList_Construct(hl);
    *ppHl = hl;
    return KTX_SUCCESS;
}


/**
 * @memberof ktxHashList @public
 * @~English
 * @brief Destroy a hash list.
 *
 * All memory associated with the hash list's keys and values
 * is freed. The hash list is also freed.
 *
 * @param [in] pHead pointer to the hash list to be destroyed.
 */
void
ktxHashList_Destroy(ktxHashList* pHead)
{
    ktxHashList_Destruct(pHead);
    free(pHead);
}


/**
 * @memberof ktxHashList @public
 * @~English
 * @brief Add a key value pair to a hash list.
 *
 * @param [in] pHead    pointer to the head of the target hash list.
 * @param [in] key      pointer to the UTF8 NUL-terminated string to be used as the key.
 * @param [in] valueLen the number of bytes of data in @p value.
 * @param [in] value    pointer to the bytes of data constituting the value.
 *
 * @return KTX_SUCCESS or one of the following error codes.
 * @exception KTX_INVALID_VALUE if @p This, @p key or @p value are NULL, @p key is an
 *            empty string or @p valueLen == 0.
 */
KTX_error_code
ktxHashList_AddKVPair(ktxHashList* pHead, const char* key, unsigned int valueLen, const void* value)
{
    if (pHead && key && value && valueLen != 0) {
        unsigned int keyLen = (unsigned int)strlen(key) + 1;
        /* ktxKVListEntry* head = *(ktxKVListEntry**)This; */
        ktxKVListEntry* kv;

        if (keyLen == 1)
            return KTX_INVALID_VALUE;   /* Empty string */

        /* Allocate all the memory as a block */
        kv = (ktxKVListEntry*)malloc(sizeof(ktxKVListEntry) + keyLen + valueLen);
        /* Put key first */
        kv->key = (char *)kv + sizeof(ktxKVListEntry);
        kv->keyLen = keyLen;
        /* then value */
        kv->value = kv->key + keyLen;
        kv->valueLen = valueLen;
        memcpy(kv->key, key, keyLen);
        memcpy(kv->value, value, valueLen);

        HASH_ADD_KEYPTR( hh, *pHead, kv->key, kv->keyLen-1, kv);
        return KTX_SUCCESS;
    } else
        return KTX_INVALID_VALUE;
}


/**
 * @memberof ktxHashList @public
 * @~English
 * @brief Looks up a key in a hash list and returns the value.
 *
 * @param [in]     pHead        pointer to the head of the target hash list.
 * @param [in]     key          pointer to a UTF8 NUL-terminated string to find.
 * @param [in,out] pValueLen    @p *pValueLen is set to the number of bytes of
 *                              data in the returned value.
 * @param [in,out] ppValue      @p *ppValue is set to the point to the value for
 *                              @p key.
 *
 * @return KTX_SUCCESS or one of the following error codes.
 *
 * @exception KTX_INVALID_VALUE if @p This, @p key or @p pValueLen or @p ppValue
 *                              is NULL.
 * @exception KTX_NOT_FOUND     an entry matching @p key was not found.
 */
KTX_error_code
ktxHashList_FindValue(ktxHashList *pHead, const char* key, unsigned int* pValueLen, void** ppValue)
{
    if (pHead && key && pValueLen && ppValue) {
        ktxKVListEntry* kv;
        /* ktxKVListEntry* head = *(ktxKVListEntry**)This; */

        HASH_FIND_STR( *pHead, key, kv );  /* kv: output pointer */

        if (kv) {
            *pValueLen = kv->valueLen;
            *ppValue = kv->value;
            return KTX_SUCCESS;
        } else
            return KTX_NOT_FOUND;
    } else
        return KTX_INVALID_VALUE;
}


/**
 * @memberof ktxHashList @public
 * @~English
 * @brief Serialize a hash list to a block of data suitable for writing
 *        to a file.
 *
 * The caller is responsible for freeing the data block returned by this
 * function.
 *
 * @param [in]     pHead        pointer to the head of the target hash list.
 * @param [in,out] pKvdLen      @p *pKvdLen is set to the number of bytes of
 *                              data in the returned data block.
 * @param [in,out] ppKvd        @p *ppKvd is set to the point to the block of
 *                              memory containing the serialized data.
 *
 * @return KTX_SUCCESS or one of the following error codes.
 *
 * @exception KTX_INVALID_VALUE if @p This, @p pKvdLen or @p ppKvd is NULL.
 * @exception KTX_OUT_OF_MEMORY there was not enough memory to serialize the
 *                              data.
 */
KTX_error_code
ktxHashList_Serialize(ktxHashList* pHead,
                      unsigned int* pKvdLen, unsigned char** ppKvd)
{

    if (pHead && pKvdLen && ppKvd) {
        ktxKVListEntry* kv;
        unsigned int bytesOfKeyValueData = 0;
        unsigned int keyValueLen;
        unsigned char* sd;
        char padding[4] = {0, 0, 0, 0};

        for (kv = *pHead; kv != NULL; kv = kv->hh.next) {
            /* sizeof(sd) is to make space to write keyAndValueByteSize */
            keyValueLen = kv->keyLen + kv->valueLen + sizeof(ktx_uint32_t);
            /* Add valuePadding */
            keyValueLen = _KTX_PAD4(keyValueLen);
            bytesOfKeyValueData += keyValueLen;
        }
        sd = malloc(bytesOfKeyValueData);
        if (!sd)
            return KTX_OUT_OF_MEMORY;

        *pKvdLen = bytesOfKeyValueData;
        *ppKvd = sd;

        for (kv = *pHead; kv != NULL; kv = kv->hh.next) {
            int padLen;

            keyValueLen = kv->keyLen + kv->valueLen;
            *(ktx_uint32_t*)sd = keyValueLen;
            sd += sizeof(ktx_uint32_t);
            memcpy(sd, kv->key, kv->keyLen);
            sd += kv->keyLen;
            memcpy(sd, kv->value, kv->valueLen);
            sd += kv->valueLen;
            padLen = _KTX_PAD4_LEN(keyValueLen);
            memcpy(sd, padding, padLen);
            sd += padLen;
        }
        return KTX_SUCCESS;
    } else
        return KTX_INVALID_VALUE;
}


/**
 * @memberof ktxHashList @public
 * @~English
 * @brief Construct a hash list from a block of serialized key-value
 *        data read from a file.
 * @note The bytes of the 32-bit key-value lengths within the serialized data
 *       are expected to be in native endianness.
 *
 * @param [in]      pHead       pointer to the head of the target hash list.
 * @param [in]      kvdLen      the length of the serialized key-value data.
 * @param [in]      pKvd        pointer to the serialized key-value data.
 *                              table.
 *
 * @return KTX_SUCCESS or one of the following error codes.
 *
 * @exception KTX_INVALID_OPERATION if @p pHead does not point to an empty list.
 * @exception KTX_INVALID_VALUE if @p pKvd or @p pHt is NULL or kvdLen == 0.
 * @exception KTX_OUT_OF_MEMORY there was not enough memory to create the hash
 *                              table.
 */
KTX_error_code
ktxHashList_Deserialize(ktxHashList* pHead, unsigned int kvdLen, void* pKvd)
{
    char* src = pKvd;
    KTX_error_code result;

    if (kvdLen == 0 || pKvd == NULL || pHead == NULL)
        return KTX_INVALID_VALUE;
    
    if (*pHead != NULL)
        return KTX_INVALID_OPERATION;

    result = KTX_SUCCESS;
    while (result == KTX_SUCCESS && src < (char *)pKvd + kvdLen) {
        char* key;
        unsigned int keyLen;
        void* value;
        ktx_uint32_t keyAndValueByteSize = *((ktx_uint32_t*)src);

        src += sizeof(keyAndValueByteSize);
        key = src;
        keyLen = (unsigned int)strlen(key) + 1;
        value = key + keyLen;

        result = ktxHashList_AddKVPair(pHead, key, keyAndValueByteSize - keyLen,
                                       value);
        if (result == KTX_SUCCESS) {
            src += _KTX_PAD4(keyAndValueByteSize);
        }
    }
    return result;
}


