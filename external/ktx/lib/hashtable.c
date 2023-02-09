/* -*- tab-width: 4; -*- */
/* vi: set sw=2 ts=4 expandtab: */

/**
 * @internal
 * @file hashtable.c
 * @~English
 *
 * @brief Functions for backward compatibility with libktx v1
 *        hashtable API.
 *
 * @author Mark Callow, www.edgewise-consulting.com
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


#include "ktx.h"

/**
 * @memberof KTX_hash_table
 * @~English
 * @deprecated Use ktxHashList_Create().
 */
KTX_hash_table
ktxHashTable_Create(void) {
    ktxHashList* hl;
    (void)ktxHashList_Create(&hl);
    return hl;
}

/**
 * @memberof KTX_hash_table
 * @~English
 * @deprecated Use ktxHashList_Serialize().
 * @brief Serializes the hash table to a block of memory suitable for
 *        writing to a KTX file.
 */
KTX_error_code
ktxHashTable_Serialize(KTX_hash_table This,
                      unsigned int* kvdLen, unsigned char** kvd)
{
    return ktxHashList_Serialize(This, kvdLen, kvd);
}

/**
 * @memberof KTX_hash_table
 * @deprecated Use ktxHashList_Deserialize().
 * @~English
 * @brief Create a new hash table from a block of serialized key-value
 *        data read from a file.
 *
 * The caller is responsible for freeing the returned hash table.
 *
 * @note The bytes of the 32-bit key-value lengths within the serialized data
 *       are expected to be in native endianness.
 *
 * @param[in]        kvdLen      the length of the serialized key-value data.
 * @param[in]        pKvd        pointer to the serialized key-value data.
 * @param[in,out]    pHt         @p *pHt is set to point to the created hash
 *                               table.
 *
 * @return KTX_SUCCESS or one of the following error codes.
 *
 * @exception KTX_INVALID_VALUE if @p pKvd or @p pHt is NULL or kvdLen == 0.
 * @exception KTX_OUT_OF_MEMORY there was not enough memory to create the hash
 *                              table.
 */
KTX_error_code
ktxHashTable_Deserialize(unsigned int kvdLen, void* pKvd, KTX_hash_table* pHt)
{
    ktxHashList* pHl;
    KTX_error_code result;
    result = ktxHashList_Create(&pHl);
    if (result != KTX_SUCCESS)
        return result;

    result = ktxHashList_Deserialize(pHl, kvdLen, pKvd);
    if (result == KTX_SUCCESS)
        *pHt = pHl;
    return result;
}  

