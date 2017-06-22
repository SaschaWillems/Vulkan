/*
 *  MVKExample.h
 *
 *  Copyright (c) 2016-2017 The Brenwill Workshop Ltd.
 *  This code is licensed under the MIT license (MIT) (http://opensource.org/licenses/MIT)
 */

#pragma once

#include "vulkanexamplebase.h"

// Wrapper class for the SW VulkanExample instance.
class MVKExample {

public:
    void renderFrame();
    void keyPressed(uint32_t keyCode);

    MVKExample(void* view);
    ~MVKExample();

protected:
    VulkanExampleBase* _vulkanExample;
};



