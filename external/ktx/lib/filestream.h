/* -*- tab-width: 4; -*- */
/* vi: set sw=2 ts=4 expandtab: */

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

/*
 * Author: Maksim Kolesin from original code
 * by Mark Callow and Georg Kolling
 */

#ifndef FILESTREAM_H
#define FILESTREAM_H

#include "ktx.h"
#include "stream.h"

/*
 * ktxFileInit: Initialize a ktxStream to a ktxFileStream with a FILE object
 */
KTX_error_code ktxFileStream_construct(ktxStream* str, FILE* file,
                                       ktx_bool_t closeFileOnDestruct);

void ktxFileStream_destruct(ktxStream* str);

#endif /* FILESTREAM_H */
