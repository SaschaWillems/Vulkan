//========================================================================
// GLFW 3.3 - www.glfw.org
//------------------------------------------------------------------------
// Copyright (c) 2002-2006 Marcus Geelnard
// Copyright (c) 2006-2016 Camilla Löwy <elmindreda@glfw.org>
//
// This software is provided 'as-is', without any express or implied
// warranty. In no event will the authors be held liable for any damages
// arising from the use of this software.
//
// Permission is granted to anyone to use this software for any purpose,
// including commercial applications, and to alter it and redistribute it
// freely, subject to the following restrictions:
//
// 1. The origin of this software must not be misrepresented; you must not
//    claim that you wrote the original software. If you use this software
//    in a product, an acknowledgment in the product documentation would
//    be appreciated but is not required.
//
// 2. Altered source versions must be plainly marked as such, and must not
//    be misrepresented as being the original software.
//
// 3. This notice may not be removed or altered from any source
//    distribution.
//
//========================================================================

#include "internal.h"

#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <stdarg.h>


// The global variables below comprise all global data in GLFW.
// Any other global variable is a bug.

// Global state shared between compilation units of GLFW
//
_GLFWlibrary _glfw = { GLFW_FALSE };

// These are outside of _glfw so they can be used before initialization and
// after termination
//
static int _glfwErrorCode;
static GLFWerrorfun _glfwErrorCallback;
static _GLFWinitconfig _glfwInitHints =
{
    GLFW_TRUE, // hat buttons
    {
        GLFW_TRUE, // menubar
        GLFW_TRUE  // chdir
    }
};

// Returns a generic string representation of the specified error
//
static const char* getErrorString(int error)
{
    switch (error)
    {
        case GLFW_NOT_INITIALIZED:
            return "The GLFW library is not initialized";
        case GLFW_NO_CURRENT_CONTEXT:
            return "There is no current context";
        case GLFW_INVALID_ENUM:
            return "Invalid argument for enum parameter";
        case GLFW_INVALID_VALUE:
            return "Invalid value for parameter";
        case GLFW_OUT_OF_MEMORY:
            return "Out of memory";
        case GLFW_API_UNAVAILABLE:
            return "The requested API is unavailable";
        case GLFW_VERSION_UNAVAILABLE:
            return "The requested API version is unavailable";
        case GLFW_PLATFORM_ERROR:
            return "An undocumented platform-specific error occurred";
        case GLFW_FORMAT_UNAVAILABLE:
            return "The requested format is unavailable";
        case GLFW_NO_WINDOW_CONTEXT:
            return "The specified window has no context";
        default:
            return "ERROR: UNKNOWN GLFW ERROR";
    }
}

// Terminate the library
//
static void terminate(void)
{
    int i;

    memset(&_glfw.callbacks, 0, sizeof(_glfw.callbacks));

    while (_glfw.windowListHead)
        glfwDestroyWindow((GLFWwindow*) _glfw.windowListHead);

    while (_glfw.cursorListHead)
        glfwDestroyCursor((GLFWcursor*) _glfw.cursorListHead);

    for (i = 0;  i < _glfw.monitorCount;  i++)
    {
        _GLFWmonitor* monitor = _glfw.monitors[i];
        if (monitor->originalRamp.size)
            _glfwPlatformSetGammaRamp(monitor, &monitor->originalRamp);
        _glfwFreeMonitor(monitor);
    }

    free(_glfw.monitors);
    _glfw.monitors = NULL;
    _glfw.monitorCount = 0;

    _glfwTerminateVulkan();
    _glfwPlatformTerminate();

    if (_glfwPlatformIsValidTls(&_glfw.error))
        _glfwErrorCode = (int) (intptr_t) _glfwPlatformGetTls(&_glfw.error);

    _glfwPlatformDestroyTls(&_glfw.context);
    _glfwPlatformDestroyTls(&_glfw.error);

    memset(&_glfw, 0, sizeof(_glfw));
}


//////////////////////////////////////////////////////////////////////////
//////                         GLFW event API                       //////
//////////////////////////////////////////////////////////////////////////

void _glfwInputError(int error, const char* format, ...)
{
    if (_glfw.initialized)
        _glfwPlatformSetTls(&_glfw.error, (void*) (intptr_t) error);
    else
        _glfwErrorCode = error;

    if (_glfwErrorCallback)
    {
        char buffer[8192];
        const char* description;

        if (format)
        {
            int count;
            va_list vl;

            va_start(vl, format);
            count = vsnprintf(buffer, sizeof(buffer), format, vl);
            va_end(vl);

            if (count < 0)
                buffer[sizeof(buffer) - 1] = '\0';

            description = buffer;
        }
        else
            description = getErrorString(error);

        _glfwErrorCallback(error, description);
    }
}


//////////////////////////////////////////////////////////////////////////
//////                        GLFW public API                       //////
//////////////////////////////////////////////////////////////////////////

GLFWAPI int glfwInit(void)
{
    if (_glfw.initialized)
        return GLFW_TRUE;

    memset(&_glfw, 0, sizeof(_glfw));
    _glfw.hints.init = _glfwInitHints;

    if (!_glfwPlatformInit())
    {
        terminate();
        return GLFW_FALSE;
    }

    if (!_glfwPlatformCreateTls(&_glfw.error))
        return GLFW_FALSE;
    if (!_glfwPlatformCreateTls(&_glfw.context))
        return GLFW_FALSE;

    _glfwPlatformSetTls(&_glfw.error, (void*) (intptr_t) _glfwErrorCode);

    _glfw.initialized = GLFW_TRUE;
    _glfw.timer.offset = _glfwPlatformGetTimerValue();

    // Not all window hints have zero as their default value
    glfwDefaultWindowHints();

    return GLFW_TRUE;
}

GLFWAPI void glfwTerminate(void)
{
    if (!_glfw.initialized)
        return;

    terminate();
}

GLFWAPI void glfwInitHint(int hint, int value)
{
    switch (hint)
    {
        case GLFW_JOYSTICK_HAT_BUTTONS:
            _glfwInitHints.hatButtons = value;
            return;
        case GLFW_COCOA_CHDIR_RESOURCES:
            _glfwInitHints.ns.chdir = value;
            return;
        case GLFW_COCOA_MENUBAR:
            _glfwInitHints.ns.menubar = value;
            return;
    }

    _glfwInputError(GLFW_INVALID_ENUM, "Invalid init hint %i", hint);
}

GLFWAPI void glfwGetVersion(int* major, int* minor, int* rev)
{
    if (major != NULL)
        *major = GLFW_VERSION_MAJOR;
    if (minor != NULL)
        *minor = GLFW_VERSION_MINOR;
    if (rev != NULL)
        *rev = GLFW_VERSION_REVISION;
}

GLFWAPI const char* glfwGetVersionString(void)
{
    return _glfwPlatformGetVersionString();
}

GLFWAPI int glfwGetError(void)
{
    int error;

    if (_glfw.initialized)
    {
        error = (int) (intptr_t) _glfwPlatformGetTls(&_glfw.error);
        _glfwPlatformSetTls(&_glfw.error, (void*) (intptr_t) GLFW_NO_ERROR);
    }
    else
    {
        error = _glfwErrorCode;
        _glfwErrorCode = GLFW_NO_ERROR;
    }

    return error;
}

GLFWAPI GLFWerrorfun glfwSetErrorCallback(GLFWerrorfun cbfun)
{
    _GLFW_SWAP_POINTERS(_glfwErrorCallback, cbfun);
    return cbfun;
}

