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
    //void renderFrame();                           // SRS - don't need to expose VulkanExampleBase::renderFrame() to DemoViewController
    void displayLinkOutputCb();                     // SRS - expose VulkanExampleBase::displayLinkOutputCb() to DemoViewController
    void getRefreshPeriod(double refreshPeriod);    // SRS - get the actual refresh period of the display
    
    void windowWillResize(float x, float y);        // SRS - expose window resize events to DemoViewController
    void windowDidResize();
    
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



