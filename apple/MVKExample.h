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
    void displayLinkOutputCb();                     // SRS - expose VulkanExampleBase::displayLinkOutputCb() to DemoViewController
    
    void keyPressed(uint32_t keyChar);              // SRS - expose keyboard events to DemoViewController
    void keyDown(uint32_t keyChar);
    void keyUp(uint32_t keyChar);
    
    void mouseDown(double x, double y);             // SRS - expose mouse events to DemoViewController
    void mouseUp();
    void rightMouseDown(double x, double y);
    void rightMouseUp();
    void otherMouseDown(double x, double y);
    void otherMouseUp();
    void mouseDragged(double x, double y);
    void scrollWheel(short wheelDelta);
	
	void fullScreen(bool fullscreen);				// SRS - expose VulkanExampleBase::settings.fullscreen to DemoView (macOS only)
	
    MVKExample(void* view, double scaleUI);			// SRS - support UIOverlay scaling parameter based on device and display type
    ~MVKExample();

protected:
    VulkanExampleBase* _vulkanExample;
};



