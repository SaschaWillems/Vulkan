/*
 *  MVKExample.cpp
 *
 *  Copyright (c) 2016-2017 The Brenwill Workshop Ltd.
 *  This code is licensed under the MIT license (MIT) (http://opensource.org/licenses/MIT)
 */

#include <Carbon/Carbon.h>

#include "MVKExample.h"
#include "examples.h"

void MVKExample::displayLinkOutputCb() {                        // SRS - expose VulkanExampleBase::displayLinkOutputCb() to DemoViewController
    _vulkanExample->displayLinkOutputCb();
}

void MVKExample::setRefreshPeriod(double refreshPeriod) {       // SRS - set VulkanExampleBase::refreshPeriod from DemoViewController displayLink
    _vulkanExample->refreshPeriod = refreshPeriod;
}

void MVKExample::keyPressed(uint32_t keyCode) {
    _vulkanExample->keyPressed(keyCode);
}

void MVKExample::keyDown(uint32_t keyCode) {
    switch (keyCode)
    {
        case kVK_ANSI_P:
            _vulkanExample->paused = !_vulkanExample->paused;
            break;
        case kVK_ANSI_W:
            _vulkanExample->camera.keys.up = true;
            break;
        case kVK_ANSI_S:
            _vulkanExample->camera.keys.down = true;
            break;
        case kVK_ANSI_A:
            _vulkanExample->camera.keys.left = true;
            break;
        case kVK_ANSI_D:
            _vulkanExample->camera.keys.right = true;
            break;
        default:
            break;
    }
}

void MVKExample::keyUp(uint32_t keyCode) {
    switch (keyCode)
    {
        case kVK_ANSI_W:
            _vulkanExample->camera.keys.up = false;
            break;
        case kVK_ANSI_S:
            _vulkanExample->camera.keys.down = false;
            break;
        case kVK_ANSI_A:
            _vulkanExample->camera.keys.left = false;
            break;
        case kVK_ANSI_D:
            _vulkanExample->camera.keys.right = false;
            break;
        default:
            break;
    }
}

void MVKExample::mouseDown(double x, double y) {
    _vulkanExample->mousePos = glm::vec2(x, y);
    _vulkanExample->mouseButtons.left = true;
}

void MVKExample::mouseUp(double x, double y) {
    _vulkanExample->mousePos = glm::vec2(x, y);
    _vulkanExample->mouseButtons.left = false;
}

void MVKExample::rightMouseDown() {
    _vulkanExample->mouseButtons.right = true;
}

void MVKExample::rightMouseUp() {
    _vulkanExample->mouseButtons.right = false;
}

void MVKExample::otherMouseDown() {
    _vulkanExample->mouseButtons.middle = true;
}

void MVKExample::otherMouseUp() {
    _vulkanExample->mouseButtons.middle = false;
}

void MVKExample::mouseDragged(double x, double y) {
    _vulkanExample->mouseDragged(x, y);
}

void MVKExample::mouseMoved(double x, double y) {
    _vulkanExample->mouseDragged(x, y);
}

void MVKExample::scrollWheel(short wheelDelta) {
    _vulkanExample->camera.translate(glm::vec3(0.0f, 0.0f, wheelDelta * 0.05f * _vulkanExample->camera.movementSpeed));
}

MVKExample::MVKExample(void* view) {
    _vulkanExample = new VulkanExample();
	width = _vulkanExample->width;								// SRS - Get desired window size from VulkanExampleBase instance
	height = _vulkanExample->height;
    _vulkanExample->initVulkan();
    _vulkanExample->setupWindow(view);
    _vulkanExample->prepare();
	_vulkanExample->renderLoop();								// SRS - Init destWidth & destHeight, then fall through and return
}

MVKExample::~MVKExample() {
    delete _vulkanExample;
}
