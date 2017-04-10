/*
* Platform indepdenent virtual key codes
*
* Copyright (C) 2017 by Sascha Willems - www.saschawillems.de
*
* This code is licensed under the MIT license (MIT) (http://opensource.org/licenses/MIT)
*/

#pragma once
#ifndef VKS_BASE_VIRTUALKEYCODE
#define VKS_BASE_VIRTUALKEYCODE

namespace vks
{
	//! A very simple version of platform independent virtual keys
	enum class VirtualKeyCode
	{
		Undefined = 0,

		Escape = 0x001B,
		Space = 0x0020,

		A = 'A',
		B = 'B',
		C = 'C',
		D = 'D',
		E = 'E',
		F = 'F',
		G = 'G',
		H = 'H',
		I = 'I',
		J = 'J',
		K = 'K',
		L = 'L',
		M = 'M',
		N = 'N',
		O = 'O',
		P = 'P',
		Q = 'Q',
		R = 'R',
		S = 'S',
		T = 'T',
		U = 'U',
		V = 'V',
		W = 'W',
		X = 'X',
		Y = 'Y',
		Z = 'Z',

		// Game pad mappings
		GamePadButtonA = 0xF100,
		GamePadButtonB = 0xF101,
		GamePadButtonC = 0xF102,
		GamePadButtonX = 0xF103,
		GamePadButtonY = 0xF104,
		GamePadButtonZ = 0xF105,
		GamePadButtonLeftShoulder1 = 0xF106,
		GamePadButtonRightShoulder1 = 0xF107,
		GamePadButtonLeftShoulder2 = 0xF108,
		GamePadButtonRightShoulder2 = 0xF109,
		GamePadButtonLeftThumb = 0xF10A,
		GamePadButtonRightThumb = 0xF10B,
		GamePadButtonStart = 0xF10C,
		GamePadButtonSelect = 0xF10D,
		GamePadButtonMode = 0xF10E,
		GamePadButtonBack = 0xF10F,
		GamePadDpadUp = 0xF110,
		GamePadDpadDown = 0xF111,
		GamePadDpadLeft = 0xF112,
		GamePadDpadRight = 0xF113,
		GamePadDpadCenter = 0xF114,

		F1 = 0xF401,
		F2 = 0xF402,
		F3 = 0xF403,
		F4 = 0xF404,
		F5 = 0xF405,

		Add = 0xF500,
		Subtract = 0xF501,

		TouchDoubleTap = 0xF800
	};
}

#endif
