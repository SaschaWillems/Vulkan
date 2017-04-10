/*
* Platform independent virtual mouse button flags
*
* Copyright (C) 2017 by Sascha Willems - www.saschawillems.de
*
* This code is licensed under the MIT license (MIT) (http://opensource.org/licenses/MIT)
*/

#pragma once
#ifndef VKS_BASE_VIRTUALMOUSEBUTTONFLAGS
#define VKS_BASE_VIRTUALMOUSEBUTTONFLAGS

#include <cstdint>

namespace vks
{
	//! A very simple version of platform independent mouse button flags
	struct VirtualMouseButtonFlags
	{
		enum Flags
		{
			Left = 0x01,
			Middle = 0x02,
			Right = 0x04,
		};

		uint32_t Value = 0;

		bool IsEnabled(const Flags flags) const
		{
			return (Value & flags) == flags;
		}
	};
}

#endif
