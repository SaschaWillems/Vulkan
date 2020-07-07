/* -*- tab-width: 4; -*- */
/* vi: set sw=2 ts=4 expandtab textwidth=70: */

/*
 * ©2017 Mark Callow.
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
 * @file vk_funcs.c
 * @~English
 *
 * @brief Retrieve Vulkan function pointers needed by libktx
 */

#if defined(KTX_USE_FUNCPTRS_FOR_VULKAN)

#define UNIX 0
#define MACOS 0
#define WINDOWS 0

#if defined(_WIN32)
#undef WINDOWS
#define WINDOWS 1
#endif
#if defined(__FreeBSD__) || defined(__NetBSD__) || defined(__OpenBSD__) || defined(__bsdi__) || defined(__DragonFly__)
#undef UNIX
#define UNIX 1
#endif
#if defined(linux) || defined(__linux) || defined(__linux__)
#undef UNIX
#define UNIX 1
#endif
#if defined(__APPLE__) && defined(__x86_64__)
#undef MACOS
#define MACOS 1
#endif

#if (IOS + MACOS + UNIX + WINDOWS) > 1
#error "Multiple OS\'s defined"
#endif 

#if WINDOWS
#define WINDOWS_LEAN_AND_MEAN
#include <windows.h>
#else
#include <dlfcn.h>
#endif
#include "ktx.h"
#include "vk_funcs.h"

#if WINDOWS
HMODULE ktxVulkanLibary;
#define LoadLibrary LoadLibrary
#define LoadProcAddr GetProcAddress
#elif MACOS || UNIX
#define LoadLibrary dlopen
#define LoadProcAddr dlsym
void* ktxVulkanLibrary;
#else
#error "Don\'t know how to load symbols on this OS."
#endif

#if WINDOWS
#define VULKANLIB "vulkan-1.dll"
#elif MACOS
#define VULKANLIB "vulkan.framework/vulkan"
#elif UNIX
#define VULKANLIB "libvulkan.so"
#endif

static PFN_vkGetInstanceProcAddr vkGetInstanceProcAddr;

/* Define pointers for functions libktx is using. */
#define VK_FUNCTION(fun) PFN_##fun ktx_##fun;

#include "vk_funclist.inl"

#undef VK_FUNCTION

#if 0
// The Vulkan spec. recommends using vkGetInstanceProcAddr over dlsym
// (or whatever). Doing so would require a backward incompatible
// change to the libktx API to provide the VkInstance. We have no
// choice but dlsym. We can't use vkGetDeviceProcAddr because libktx
// also uses none-device-level functions.
#define VK_FUNCTION(fun)                                                     \
  if ( !(ktx_##fun = (PFN_##fun)vkGetInstanceProcAddr(instance, #fun )) ) {  \
    fprintf(stderr, "Could not load Vulkan command: %s!\n", #fun);          \
    return KTX_FALSE;                                             \
  }
#else
#if defined(__GNUC__)
// This strange casting is because dlsym returns a void* thus is not
// compatible with ISO C which forbids conversion of object pointers
// to function pointers. The cast masks the conversion from the
// compiler thus no warning even though -pedantic is set. Since the
// platform supports dlsym, conversion to function pointers must
// work, despite the mandated ISO C warning.
#define VK_FUNCTION(fun)                                                   \
  if ( !(*(void **)(&ktx_##fun) = LoadProcAddr(ktxVulkanLibrary, #fun)) ) {\
    fprintf(stderr, "Could not load Vulkan command: %s!\n", #fun);         \
    return KTX_FALSE;                                                      \
  }
#else
#define VK_FUNCTION(fun)                                                   \
  if ( !(ktx_##fun = (PFN_##fun)LoadProcAddr(ktxVulkanLibrary, #fun)) ) {  \
    fprintf(stderr, "Could not load Vulkan command: %s!\n", #fun);         \
    return KTX_FALSE;                                                      \
  }
#endif
#endif

ktx_bool_t
ktxVulkanLoadLibrary(void)
{
    if (ktxVulkanLibrary)
        return KTX_TRUE;

    ktxVulkanLibrary = LoadLibrary(VULKANLIB, RTLD_LAZY);
    if (ktxVulkanLibrary == NULL) {
        fprintf(stderr, "Could not load Vulkan library.\n");
        return(KTX_FALSE);
    }

#if 0
    vkGetInstanceProcAddr =
            (PFN_vkGetInstanceProcAddr)LoadProcAddr(ktxVulkanLibrary,
                                                  "vkGetInstanceProcAddr");
    if (!vkGetInstanceProcAddr) {
       fprintf(stderr, "Could not load Vulkan command: %s!\n",
               "vkGetInstanceProcAddr");
       return(KTX_FALSE);
    }
#endif

#include "vk_funclist.inl"

    return KTX_TRUE;
}

#undef VK_FUNCTION

#else

extern
#if defined(__GNUC__)
__attribute__((unused))
#endif
int keepISOCompilersHappy;

#endif /* KTX_USE_FUNCPTRS_FOR_VULKAN */
