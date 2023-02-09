/* -*- tab-width: 4; -*- */
/* vi: set sw=2 ts=4 expandtab: */

/* $Id: d858975c68f39438233f78cbf217f70fc5ddd316 $ */

/*
 * Copyright (c) 2010 The Khronos Group Inc.
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

#include "KHR/khrplatform.h"

/*
 * SwapEndian16: Swaps endianness in an array of 16-bit values
 */
void
_ktxSwapEndian16(khronos_uint16_t* pData16, int count)
{
    int i;
    for (i = 0; i < count; ++i)
    {
        khronos_uint16_t x = *pData16;
        *pData16++ = (x << 8) | (x >> 8);
    }
}

/*
 * SwapEndian32: Swaps endianness in an array of 32-bit values
 */
void 
_ktxSwapEndian32(khronos_uint32_t* pData32, int count)
{
    int i;
    for (i = 0; i < count; ++i)
    {
        khronos_uint32_t x = *pData32;
        *pData32++ = (x << 24) | ((x & 0xFF00) << 8) | ((x & 0xFF0000) >> 8) | (x >> 24);
    }
}


