/*
* Key codes for multiple platforms
*
* Copyright (C) 2016 by Sascha Willems - www.saschawillems.de
*
* This code is licensed under the MIT license (MIT) (http://opensource.org/licenses/MIT)
*/

#pragma once

#if defined(__ANDROID__)
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
#else 
#define KEY_ESCAPE GLFW_KEY_ESCAPE 
#define KEY_F1 GLFW_KEY_F1
#define KEY_F2 GLFW_KEY_F2
#define KEY_F3 GLFW_KEY_F3
#define KEY_F4 GLFW_KEY_F4
#define KEY_W GLFW_KEY_W
#define KEY_A GLFW_KEY_A
#define KEY_S GLFW_KEY_S
#define KEY_D GLFW_KEY_D
#define KEY_P GLFW_KEY_P
#define KEY_SPACE GLFW_KEY_SPACE
#define KEY_KPADD GLFW_KEY_KP_ADD
#define KEY_KPSUB GLFW_KEY_KP_SUBTRACT
#define KEY_B GLFW_KEY_B
#define KEY_F GLFW_KEY_F
#define KEY_L GLFW_KEY_L
#define KEY_N GLFW_KEY_N
#define KEY_O GLFW_KEY_O
#define KEY_T GLFW_KEY_T
#endif

// todo: Android gamepad keycodes outside of define for now
#define GAMEPAD_BUTTON_A 0x1000
#define GAMEPAD_BUTTON_B 0x1001
#define GAMEPAD_BUTTON_X 0x1002
#define GAMEPAD_BUTTON_Y 0x1003
#define GAMEPAD_BUTTON_L1 0x1004
#define GAMEPAD_BUTTON_R1 0x1005
#define GAMEPAD_BUTTON_START 0x1006
