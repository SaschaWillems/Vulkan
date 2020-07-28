/* -*- tab-width: 4; -*- */
/* vi: set sw=2 ts=4 expandtab: */

/* $Id: 11f526c2587823b49cde3b437746e95b57e3200e $ */

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
 * @file errstr.c
 * @~English
 *
 * @brief Function to return a string corresponding to a KTX error code.
 *
 * @author Mark Callow, HI Corporation
 */

#include "ktx.h"

static const char* const errorStrings[] = {
    "Operation succeeded",                            /* KTX_SUCCESS */
    "File data is inconsistent with KTX spec.",       /* KTX_FILE_DATA_ERROR */
    "File open failed",                               /* KTX_FILE_OPEN_FAILED */
    "Operation would exceed the max file size",       /* KTX_FILE_OVERFLOW */
    "File read error",                                /* KTX_FILE_READ_ERROR */
    "File seek error",                                /* KTX_FILE_SEEK_ERROR */
    "File does not have enough data for request",     /* KTX_FILE_UNEXPECTED_EOF */
    "File write error",                               /* KTX_FILE_WRITE_ERROR */
    "GL error occurred",                              /* KTX_GL_ERROR */
    "Operation not allowed in the current state",     /* KTX_INVALID_OPERATION */
    "Invalid parameter value",                        /* KTX_INVALID_VALUE */
    "Key not found",                                  /* KTX_NOT_FOUND */
    "Out of memory",                                  /* KTX_OUT_OF_MEMORY */
    "Not a KTX file",                                 /* KTX_UNKNOWN_FILE_FORMAT */
    "Texture type not supported by GL context"        /* KTX_UNSUPPORTED_TEXTURE_TYPE */
};
static const int lastErrorCode = (sizeof(errorStrings) / sizeof(char*)) - 1;


/**
 * @~English
 * @brief Return a string corresponding to a KTX error code.
 *
 * @param error     the error code for which to return a string
 *
 * @return pointer to the message string.
 *
 * @internal Use UTF-8 for translated message strings.
 * 
 * @author Mark Callow, HI Corporation
 */
const char* const ktxErrorString(KTX_error_code error)
{
    if (error > lastErrorCode)
        return "Unrecognized error code";
    return errorStrings[error];
}
