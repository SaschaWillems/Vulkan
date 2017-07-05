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
#elif defined(__ANDROID__)
// Dummy key codes 
#define KEY_ESCAPE 0x0
#define KEY_F1 0x1
#define KEY_F2 0x2
#define KEY_F2 0x11
#define KEY_F2 0x12
#define KEY_F3 0x13
#define KEY_F4 0x14
#define KEY_W 0x3
#define KEY_A 0x4
#define KEY_S 0x5
#define KEY_D 0x6
#define KEY_P 0x7
#define KEY_SPACE 0x8
#define KEY_KPADD 0x9
#define KEY_KPSUB 0xA
#define KEY_B 0xB
#define KEY_F 0xC
#define KEY_L 0xD
#define KEY_N 0xE
#define KEY_O 0xF
#define KEY_T 0x10

#elif defined(VK_USE_PLATFORM_IOS_MVK)
// Use numeric keys instead of function keys.
// Use main keyboard plus/minus instead of keypad plus/minus
// Use Delete key instead of Escape key.
#define KEY_ESCAPE 0x33
#define KEY_F1 '1'
#define KEY_F2 '2'
#define KEY_F3 '3'
#define KEY_F4 '4'
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
#define KEY_T 't'

#elif defined(VK_USE_PLATFORM_MACOS_MVK)
// For compatibility with iOS UX and absent keypad on MacBook:
// - Use numeric keys instead of function keys
// - Use main keyboard plus/minus instead of keypad plus/minus
// - Use Delete key instead of Escape key
#define KEY_ESCAPE 0x33
#define KEY_F1 0x12
#define KEY_F2 0x13
#define KEY_F3 0x14
#define KEY_F4 0x15
#define KEY_W 0x0D
#define KEY_A 0x00
#define KEY_S 0x01
#define KEY_D 0x02
#define KEY_P 0x23
#define KEY_SPACE 0x31
#define KEY_KPADD 0x18
#define KEY_KPSUB 0x1B
#define KEY_B 0x0B
#define KEY_F 0x03
#define KEY_L 0x25
#define KEY_N 0x2D
#define KEY_O 0x1F
#define KEY_T 0x11

#elif defined(VK_USE_PLATFORM_WAYLAND_KHR)
#include <linux/input.h>

// todo: hack for bloom example
#define KEY_KPADD KEY_KPPLUS
#define KEY_KPSUB KEY_KPMINUS

#elif defined(__linux__)
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
#endif

// todo: Android gamepad keycodes outside of define for now
#define GAMEPAD_BUTTON_A		0x1000
#define GAMEPAD_BUTTON_B		0x1001
#define GAMEPAD_BUTTON_X		0x1002
#define GAMEPAD_BUTTON_Y		0x1003
#define GAMEPAD_BUTTON_L1		0x1004
#define GAMEPAD_BUTTON_R1		0x1005
#define GAMEPAD_BUTTON_START	0x1006
#define TOUCH_DOUBLE_TAP		0x1100
