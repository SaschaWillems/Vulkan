/*
 *  MVKExample.cpp
 *
 *  Copyright (c) 2016-2017 The Brenwill Workshop Ltd.
 *  This code is licensed under the MIT license (MIT) (http://opensource.org/licenses/MIT)
 */


#include "MVKExample.h"
#include "examples.h"

void MVKExample::renderFrame() {
	_vulkanExample->renderFrame();
}

void MVKExample::displayLinkOutputCb() {                        // SRS - expose VulkanExampleBase::displayLinkOutputCb() to DemoViewController
    _vulkanExample->displayLinkOutputCb();
}

void MVKExample::setRefreshPeriod(double refreshPeriod) {       // SRS - set VulkanExampleBase::refreshPeriod from DemoViewController displayLink
    _vulkanExample->refreshPeriod = refreshPeriod;
}

void MVKExample::keyPressed(uint32_t keyChar) {					// SRS - handle iOS virtual screen keyboard presses
	switch (keyChar)
	{
		case 'p':
		case 'P':
			_vulkanExample->paused = !_vulkanExample->paused;
			break;
		default:
			_vulkanExample->keyPressed(keyChar);
			break;
	}
}

void MVKExample::keyDown(uint32_t keyChar) {					// SRS - handle physical keyboard key down/up actions
    switch (keyChar)
    {
		case 'p':
		case 'P':
            _vulkanExample->paused = !_vulkanExample->paused;
            break;
		case 'w':
		case 'W':
		case 'z':	// for French AZERTY keyboards
		case 'Z':
            _vulkanExample->camera.keys.up = true;
            break;
		case 's':
		case 'S':
            _vulkanExample->camera.keys.down = true;
            break;
		case 'a':
		case 'A':
		case 'q':	// for French AZERTY keyboards
		case 'Q':
            _vulkanExample->camera.keys.left = true;
            break;
		case 'd':
		case 'D':
            _vulkanExample->camera.keys.right = true;
            break;
        default:
            break;
    }
}

void MVKExample::keyUp(uint32_t keyChar) {
    switch (keyChar)
    {
		case 'w':
		case 'W':
		case 'z':	// for French AZERTY keyboards
		case 'Z':
            _vulkanExample->camera.keys.up = false;
            break;
		case 's':
		case 'S':
            _vulkanExample->camera.keys.down = false;
            break;
		case 'a':
		case 'A':
		case 'q':	// for French AZERTY keyboards
		case 'Q':
            _vulkanExample->camera.keys.left = false;
            break;
		case 'd':
		case 'D':
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

void MVKExample::mouseUp() {
    _vulkanExample->mouseButtons.left = false;
}

void MVKExample::rightMouseDown(double x, double y) {
	_vulkanExample->mousePos = glm::vec2(x, y);
    _vulkanExample->mouseButtons.right = true;
}

void MVKExample::rightMouseUp() {
    _vulkanExample->mouseButtons.right = false;
}

void MVKExample::otherMouseDown(double x, double y) {
	_vulkanExample->mousePos = glm::vec2(x, y);
    _vulkanExample->mouseButtons.middle = true;
}

void MVKExample::otherMouseUp() {
    _vulkanExample->mouseButtons.middle = false;
}

void MVKExample::mouseDragged(double x, double y) {
    _vulkanExample->mouseDragged(x, y);
}

void MVKExample::scrollWheel(short wheelDelta) {
    _vulkanExample->camera.translate(glm::vec3(0.0f, 0.0f, wheelDelta * 0.05f * _vulkanExample->camera.movementSpeed));
}

MVKExample::MVKExample(void* view) {
    _vulkanExample = new VulkanExample();
    _vulkanExample->initVulkan();
    _vulkanExample->setupWindow(view);
    _vulkanExample->prepare();
	_vulkanExample->renderLoop();			// SRS - init VulkanExampleBase::destWidth & destHeight, then fall through and return
}

MVKExample::~MVKExample() {
    delete _vulkanExample;
}
