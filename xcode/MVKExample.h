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
	uint32_t width;									// SRS - expose VulkanExampleBase initial window size to DemoViewController
	uint32_t height;

    void displayLinkOutputCb();                     // SRS - expose VulkanExampleBase::displayLinkOutputCb() to DemoViewController
    void setRefreshPeriod(double refreshPeriod);    // SRS - set VulkanExampleBase::refreshPeriod from DemoViewController displayLink
    
    void keyPressed(uint32_t keyCode);              // SRS - expose keyboard events to DemoViewController
    void keyDown(uint32_t keyCode);
    void keyUp(uint32_t keyCode);
    
    void mouseDown(double x, double y);             // SRS - expose mouse events to DemoViewController
    void mouseUp(double x, double y);
    void rightMouseDown();
    void rightMouseUp();
    void otherMouseDown();
    void otherMouseUp();
    void mouseDragged(double x, double y);
    void mouseMoved(double x, double y);
    void scrollWheel(short wheelDelta);

    MVKExample(void* view);
    ~MVKExample();

protected:
    VulkanExampleBase* _vulkanExample;
};



