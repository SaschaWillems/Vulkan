//========================================================================
// GLFW 3.3 X11 - www.glfw.org
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

#include <unistd.h>
#include <signal.h>
#include <stdint.h>
#include <dlfcn.h>

#include <X11/Xlib.h>
#include <X11/keysym.h>
#include <X11/Xatom.h>
#include <X11/Xcursor/Xcursor.h>

// The XRandR extension provides mode setting and gamma control
#include <X11/extensions/Xrandr.h>

// The Xkb extension provides improved keyboard support
#include <X11/XKBlib.h>

// The Xinerama extension provides legacy monitor indices
#include <X11/extensions/Xinerama.h>

// The XInput extension provides raw mouse motion input
#include <X11/extensions/XInput2.h>

typedef XID xcb_window_t;
typedef XID xcb_visualid_t;
typedef struct xcb_connection_t xcb_connection_t;
typedef xcb_connection_t* (* PFN_XGetXCBConnection)(Display*);

typedef Bool (* PFN_XF86VidModeQueryExtension)(Display*,int*,int*);
typedef Bool (* PFN_XF86VidModeGetGammaRamp)(Display*,int,int,unsigned short*,unsigned short*,unsigned short*);
typedef Bool (* PFN_XF86VidModeSetGammaRamp)(Display*,int,int,unsigned short*,unsigned short*,unsigned short*);
typedef Bool (* PFN_XF86VidModeGetGammaRampSize)(Display*,int,int*);
#define XF86VidModeQueryExtension _glfw.x11.vidmode.QueryExtension
#define XF86VidModeGetGammaRamp _glfw.x11.vidmode.GetGammaRamp
#define XF86VidModeSetGammaRamp _glfw.x11.vidmode.SetGammaRamp
#define XF86VidModeGetGammaRampSize _glfw.x11.vidmode.GetGammaRampSize

typedef Status (* PFN_XIQueryVersion)(Display*,int*,int*);
typedef int (* PFN_XISelectEvents)(Display*,Window,XIEventMask*,int);
#define XIQueryVersion _glfw.x11.xi.QueryVersion
#define XISelectEvents _glfw.x11.xi.SelectEvents

typedef VkFlags VkXlibSurfaceCreateFlagsKHR;
typedef VkFlags VkXcbSurfaceCreateFlagsKHR;

typedef struct VkXlibSurfaceCreateInfoKHR
{
    VkStructureType             sType;
    const void*                 pNext;
    VkXlibSurfaceCreateFlagsKHR flags;
    Display*                    dpy;
    Window                      window;
} VkXlibSurfaceCreateInfoKHR;

typedef struct VkXcbSurfaceCreateInfoKHR
{
    VkStructureType             sType;
    const void*                 pNext;
    VkXcbSurfaceCreateFlagsKHR  flags;
    xcb_connection_t*           connection;
    xcb_window_t                window;
} VkXcbSurfaceCreateInfoKHR;

typedef VkResult (APIENTRY *PFN_vkCreateXlibSurfaceKHR)(VkInstance,const VkXlibSurfaceCreateInfoKHR*,const VkAllocationCallbacks*,VkSurfaceKHR*);
typedef VkBool32 (APIENTRY *PFN_vkGetPhysicalDeviceXlibPresentationSupportKHR)(VkPhysicalDevice,uint32_t,Display*,VisualID);
typedef VkResult (APIENTRY *PFN_vkCreateXcbSurfaceKHR)(VkInstance,const VkXcbSurfaceCreateInfoKHR*,const VkAllocationCallbacks*,VkSurfaceKHR*);
typedef VkBool32 (APIENTRY *PFN_vkGetPhysicalDeviceXcbPresentationSupportKHR)(VkPhysicalDevice,uint32_t,xcb_connection_t*,xcb_visualid_t);

#include "posix_tls.h"
#include "posix_time.h"
#include "xkb_unicode.h"
#include "glx_context.h"
#include "egl_context.h"
#include "osmesa_context.h"
#if defined(__linux__)
#include "linux_joystick.h"
#else
#include "null_joystick.h"
#endif

#define _glfw_dlopen(name) dlopen(name, RTLD_LAZY | RTLD_LOCAL)
#define _glfw_dlclose(handle) dlclose(handle)
#define _glfw_dlsym(handle, name) dlsym(handle, name)

#define _GLFW_EGL_NATIVE_WINDOW  ((EGLNativeWindowType) window->x11.handle)
#define _GLFW_EGL_NATIVE_DISPLAY ((EGLNativeDisplayType) _glfw.x11.display)

#define _GLFW_PLATFORM_WINDOW_STATE         _GLFWwindowX11  x11
#define _GLFW_PLATFORM_LIBRARY_WINDOW_STATE _GLFWlibraryX11 x11
#define _GLFW_PLATFORM_MONITOR_STATE        _GLFWmonitorX11 x11
#define _GLFW_PLATFORM_CURSOR_STATE         _GLFWcursorX11  x11


// X11-specific per-window data
//
typedef struct _GLFWwindowX11
{
    Colormap        colormap;
    Window          handle;
    XIC             ic;

    GLFWbool        overrideRedirect;
    GLFWbool        iconified;
    GLFWbool        maximized;

    // Cached position and size used to filter out duplicate events
    int             width, height;
    int             xpos, ypos;

    // The last received cursor position, regardless of source
    int             lastCursorPosX, lastCursorPosY;
    // The last position the cursor was warped to by GLFW
    int             warpCursorPosX, warpCursorPosY;

    // The time of the last KeyPress event
    Time            lastKeyTime;

} _GLFWwindowX11;

// X11-specific global data
//
typedef struct _GLFWlibraryX11
{
    Display*        display;
    int             screen;
    Window          root;

    // Helper window for IPC
    Window          helperWindowHandle;
    // Invisible cursor for hidden cursor mode
    Cursor          hiddenCursorHandle;
    // Context for mapping window XIDs to _GLFWwindow pointers
    XContext        context;
    // XIM input method
    XIM             im;
    // Most recent error code received by X error handler
    int             errorCode;
    // Clipboard string (while the selection is owned)
    char*           clipboardString;
    // Key name string
    char            keyName[5];
    // X11 keycode to GLFW key LUT
    short int       keycodes[256];
    // GLFW key to X11 keycode LUT
    short int       scancodes[GLFW_KEY_LAST + 1];
    // Where to place the cursor when re-enabled
    double          restoreCursorPosX, restoreCursorPosY;
    // The window whose disabled cursor mode is active
    _GLFWwindow*    disabledCursorWindow;

    // Window manager atoms
    Atom            WM_PROTOCOLS;
    Atom            WM_STATE;
    Atom            WM_DELETE_WINDOW;
    Atom            NET_WM_NAME;
    Atom            NET_WM_ICON_NAME;
    Atom            NET_WM_ICON;
    Atom            NET_WM_PID;
    Atom            NET_WM_PING;
    Atom            NET_WM_WINDOW_TYPE;
    Atom            NET_WM_WINDOW_TYPE_NORMAL;
    Atom            NET_WM_STATE;
    Atom            NET_WM_STATE_ABOVE;
    Atom            NET_WM_STATE_FULLSCREEN;
    Atom            NET_WM_STATE_MAXIMIZED_VERT;
    Atom            NET_WM_STATE_MAXIMIZED_HORZ;
    Atom            NET_WM_STATE_DEMANDS_ATTENTION;
    Atom            NET_WM_BYPASS_COMPOSITOR;
    Atom            NET_WM_FULLSCREEN_MONITORS;
    Atom            NET_ACTIVE_WINDOW;
    Atom            NET_FRAME_EXTENTS;
    Atom            NET_REQUEST_FRAME_EXTENTS;
    Atom            MOTIF_WM_HINTS;

    // Xdnd (drag and drop) atoms
    Atom            XdndAware;
    Atom            XdndEnter;
    Atom            XdndPosition;
    Atom            XdndStatus;
    Atom            XdndActionCopy;
    Atom            XdndDrop;
    Atom            XdndFinished;
    Atom            XdndSelection;
    Atom            XdndTypeList;
    Atom            text_uri_list;

    // Selection (clipboard) atoms
    Atom            TARGETS;
    Atom            MULTIPLE;
    Atom            CLIPBOARD;
    Atom            CLIPBOARD_MANAGER;
    Atom            SAVE_TARGETS;
    Atom            NULL_;
    Atom            UTF8_STRING;
    Atom            COMPOUND_STRING;
    Atom            ATOM_PAIR;
    Atom            GLFW_SELECTION;

    struct {
        GLFWbool    available;
        int         eventBase;
        int         errorBase;
        int         major;
        int         minor;
        GLFWbool    gammaBroken;
        GLFWbool    monitorBroken;
    } randr;

    struct {
        GLFWbool    available;
        GLFWbool    detectable;
        int         majorOpcode;
        int         eventBase;
        int         errorBase;
        int         major;
        int         minor;
    } xkb;

    struct {
        int         count;
        int         timeout;
        int         interval;
        int         blanking;
        int         exposure;
    } saver;

    struct {
        int         version;
        Window      source;
        Atom        format;
    } xdnd;

    struct {
        GLFWbool    available;
        int         major;
        int         minor;
    } xinerama;

    struct {
        void*       handle;
        PFN_XGetXCBConnection XGetXCBConnection;
    } x11xcb;

    struct {
        GLFWbool    available;
        void*       handle;
        int         eventBase;
        int         errorBase;
        PFN_XF86VidModeQueryExtension QueryExtension;
        PFN_XF86VidModeGetGammaRamp GetGammaRamp;
        PFN_XF86VidModeSetGammaRamp SetGammaRamp;
        PFN_XF86VidModeGetGammaRampSize GetGammaRampSize;
    } vidmode;

    struct {
        GLFWbool    available;
        void*       handle;
        int         majorOpcode;
        int         eventBase;
        int         errorBase;
        int         major;
        int         minor;
        PFN_XIQueryVersion QueryVersion;
        PFN_XISelectEvents SelectEvents;
    } xi;

} _GLFWlibraryX11;

// X11-specific per-monitor data
//
typedef struct _GLFWmonitorX11
{
    RROutput        output;
    RRCrtc          crtc;
    RRMode          oldMode;

    // Index of corresponding Xinerama screen,
    // for EWMH full screen window placement
    int             index;

} _GLFWmonitorX11;

// X11-specific per-cursor data
//
typedef struct _GLFWcursorX11
{
    Cursor handle;

} _GLFWcursorX11;


void _glfwPollMonitorsX11(void);
GLFWbool _glfwSetVideoModeX11(_GLFWmonitor* monitor, const GLFWvidmode* desired);
void _glfwRestoreVideoModeX11(_GLFWmonitor* monitor);

Cursor _glfwCreateCursorX11(const GLFWimage* image, int xhot, int yhot);

unsigned long _glfwGetWindowPropertyX11(Window window,
                                        Atom property,
                                        Atom type,
                                        unsigned char** value);

void _glfwGrabErrorHandlerX11(void);
void _glfwReleaseErrorHandlerX11(void);
void _glfwInputErrorX11(int error, const char* message);

void _glfwPushSelectionToManagerX11(void);

