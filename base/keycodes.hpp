/*
* Key codes for multiple platforms
*
* Copyright (C) 2016 by Sascha Willems - www.saschawillems.de
*
* This code is licensed under the MIT license (MIT) (http://opensource.org/licenses/MIT)
*/

#pragma once

#if defined(_WIN32)
#define KEY_ESCAPE VK_ESCAPE 
#define KEY_F1 VK_F1
#define KEY_F2 VK_F2
#define KEY_F3 VK_F3
#define KEY_F4 VK_F4
#define KEY_F5 VK_F5
#define KEY_W 0x57
#define KEY_A 0x41
#define KEY_S 0x53
#define KEY_D 0x44
#define KEY_P 0x50
#define KEY_SPACE 0x20
#define KEY_KPADD 0x6B
#define KEY_KPSUB 0x6D
#define KEY_B 0x42
#define KEY_F 0x46
#define KEY_L 0x4C
#define KEY_N 0x4E
#define KEY_O 0x4F
#define KEY_T 0x54

#elif defined(VK_USE_PLATFORM_ANDROID_KHR)
#define GAMEPAD_BUTTON_A 0x1000
#define GAMEPAD_BUTTON_B 0x1001
#define GAMEPAD_BUTTON_X 0x1002
#define GAMEPAD_BUTTON_Y 0x1003
#define GAMEPAD_BUTTON_L1 0x1004
#define GAMEPAD_BUTTON_R1 0x1005
#define GAMEPAD_BUTTON_START 0x1006
#define TOUCH_DOUBLE_TAP 0x1100

// for textoverlay example
#define KEY_SPACE 0x3E		// AKEYCODE_SPACE
#define KEY_KPADD 0x9D		// AKEYCODE_NUMPAD_ADD

#elif (defined(VK_USE_PLATFORM_IOS_MVK) || defined(VK_USE_PLATFORM_MACOS_MVK))
#if !defined(VK_EXAMPLE_XCODE_GENERATED)
// For iOS and macOS pre-configured Xcode example project: Use character keycodes
// - Use numeric keys as optional alternative to function keys
#define KEY_DELETE 0x7F
#define KEY_ESCAPE 0x1B
#define KEY_F1 0xF704		// NSF1FunctionKey
#define KEY_F2 0xF705		// NSF2FunctionKey
#define KEY_F3 0xF706		// NSF3FunctionKey
#define KEY_F4 0xF707		// NSF4FunctionKey
#define KEY_1 '1'
#define KEY_2 '2'
#define KEY_3 '3'
#define KEY_4 '4'
#define KEY_W 'w'
#define KEY_A 'a'
#define KEY_S 's'
#define KEY_D 'd'
#define KEY_P 'p'
#define KEY_SPACE ' '
#define KEY_KPADD '+'
#define KEY_KPSUB '-'
#define KEY_B 'b'
#define KEY_F 'f'
#define KEY_L 'l'
#define KEY_N 'n'
#define KEY_O 'o'
#define KEY_Q 'q'
#define KEY_T 't'
#define KEY_Z 'z'

#else // defined(VK_EXAMPLE_XCODE_GENERATED)
// For cross-platform cmake-generated Xcode project: Use ANSI keyboard keycodes
// - Use numeric keys as optional alternative to function keys
// - Use main keyboard plus/minus instead of keypad plus/minus
#include <Carbon/Carbon.h>
#define KEY_DELETE kVK_Delete
#define KEY_ESCAPE kVK_Escape
#define KEY_F1 kVK_F1
#define KEY_F2 kVK_F2
#define KEY_F3 kVK_F3
#define KEY_F4 kVK_F4
#define KEY_1 kVK_ANSI_1
#define KEY_2 kVK_ANSI_2
#define KEY_3 kVK_ANSI_3
#define KEY_4 kVK_ANSI_4
#define KEY_W kVK_ANSI_W
#define KEY_A kVK_ANSI_A
#define KEY_S kVK_ANSI_S
#define KEY_D kVK_ANSI_D
#define KEY_P kVK_ANSI_P
#define KEY_SPACE kVK_Space
#define KEY_KPADD kVK_ANSI_Equal
#define KEY_KPSUB kVK_ANSI_Minus
#define KEY_B kVK_ANSI_B
#define KEY_F kVK_ANSI_F
#define KEY_L kVK_ANSI_L
#define KEY_N kVK_ANSI_N
#define KEY_O kVK_ANSI_O
#define KEY_Q kVK_ANSI_Q
#define KEY_T kVK_ANSI_T
#define KEY_Z kVK_ANSI_Z
#endif

#elif defined(VK_USE_PLATFORM_DIRECTFB_EXT)
#define KEY_ESCAPE DIKS_ESCAPE
#define KEY_F1 DIKS_F1
#define KEY_F2 DIKS_F2
#define KEY_F3 DIKS_F3
#define KEY_F4 DIKS_F4
#define KEY_W DIKS_SMALL_W
#define KEY_A DIKS_SMALL_A
#define KEY_S DIKS_SMALL_S
#define KEY_D DIKS_SMALL_D
#define KEY_P DIKS_SMALL_P
#define KEY_SPACE DIKS_SPACE
#define KEY_KPADD DIKS_PLUS_SIGN
#define KEY_KPSUB DIKS_MINUS_SIGN
#define KEY_B DIKS_SMALL_B
#define KEY_F DIKS_SMALL_F
#define KEY_L DIKS_SMALL_L
#define KEY_N DIKS_SMALL_N
#define KEY_O DIKS_SMALL_O
#define KEY_T DIKS_SMALL_T

#elif defined(VK_USE_PLATFORM_WAYLAND_KHR)
#include <linux/input.h>

// todo: hack for bloom example
#define KEY_ESCAPE KEY_ESC
#define KEY_KPADD KEY_KPPLUS
#define KEY_KPSUB KEY_KPMINUS

#elif defined(__linux__) || defined(__FreeBSD__)
#define KEY_ESCAPE 0x9
#define KEY_F1 0x43
#define KEY_F2 0x44
#define KEY_F3 0x45
#define KEY_F4 0x46
#define KEY_W 0x19
#define KEY_A 0x26
#define KEY_S 0x27
#define KEY_D 0x28
#define KEY_P 0x21
#define KEY_SPACE 0x41
#define KEY_KPADD 0x56
#define KEY_KPSUB 0x52
#define KEY_B 0x38
#define KEY_F 0x29
#define KEY_L 0x2E
#define KEY_N 0x39
#define KEY_O 0x20
#define KEY_T 0x1C

#elif defined(VK_USE_PLATFORM_SCREEN_QNX)
#include <sys/keycodes.h>

#define KEY_ESCAPE KEYCODE_ESCAPE
#define KEY_F1     KEYCODE_F1
#define KEY_F2     KEYCODE_F2
#define KEY_F3     KEYCODE_F3
#define KEY_F4     KEYCODE_F4
#define KEY_W      KEYCODE_W
#define KEY_A      KEYCODE_A
#define KEY_S      KEYCODE_S
#define KEY_D      KEYCODE_D
#define KEY_P      KEYCODE_P
#define KEY_SPACE  KEYCODE_SPACE
#define KEY_KPADD  KEYCODE_KP_PLUS
#define KEY_KPSUB  KEYCODE_KP_MINUS
#define KEY_B      KEYCODE_B
#define KEY_F      KEYCODE_F
#define KEY_L      KEYCODE_L
#define KEY_N      KEYCODE_N
#define KEY_O      KEYCODE_O
#define KEY_T      KEYCODE_T

#endif
