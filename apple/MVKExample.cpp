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

void MVKExample::keyPressed(uint32_t keyChar) {					// SRS - handle keyboard key presses only (e.g. Pause, Space, etc)
	switch (keyChar)
	{
		case KEY_P:
			_vulkanExample->paused = !_vulkanExample->paused;
			break;
		case KEY_1:												// SRS - support keyboards with no function keys
		case KEY_F1:
			_vulkanExample->ui.visible = !_vulkanExample->ui.visible;
			_vulkanExample->ui.updated = true;
			break;
		default:
			_vulkanExample->keyPressed(keyChar);
			break;
	}
}

void MVKExample::keyDown(uint32_t keyChar) {					// SRS - handle physical keyboard key down/up actions and presses
    switch (keyChar)
    {
		case KEY_W:
		case KEY_Z:	// for French AZERTY keyboards
            _vulkanExample->camera.keys.up = true;
            break;
		case KEY_S:
            _vulkanExample->camera.keys.down = true;
            break;
		case KEY_A:
		case KEY_Q:	// for French AZERTY keyboards
            _vulkanExample->camera.keys.left = true;
            break;
		case KEY_D:
            _vulkanExample->camera.keys.right = true;
            break;
        default:
			MVKExample::keyPressed(keyChar);
            break;
    }
}

void MVKExample::keyUp(uint32_t keyChar) {
    switch (keyChar)
    {
		case KEY_W:
		case KEY_Z:	// for French AZERTY keyboards
            _vulkanExample->camera.keys.up = false;
            break;
		case KEY_S:
            _vulkanExample->camera.keys.down = false;
            break;
		case KEY_A:
		case KEY_Q:	// for French AZERTY keyboards
            _vulkanExample->camera.keys.left = false;
            break;
		case KEY_D:
            _vulkanExample->camera.keys.right = false;
            break;
        default:
            break;
    }
}

void MVKExample::mouseDown(double x, double y) {
    _vulkanExample->mouseState.position = glm::vec2(x, y);
    _vulkanExample->mouseState.buttons.left = true;
}

void MVKExample::mouseUp() {
    _vulkanExample->mouseState.buttons.left = false;
}

void MVKExample::rightMouseDown(double x, double y) {
	_vulkanExample->mouseState.position = glm::vec2(x, y);
    _vulkanExample->mouseState.buttons.right = true;
}

void MVKExample::rightMouseUp() {
    _vulkanExample->mouseState.buttons.right = false;
}

void MVKExample::otherMouseDown(double x, double y) {
	_vulkanExample->mouseState.position = glm::vec2(x, y);
    _vulkanExample->mouseState.buttons.middle = true;
}

void MVKExample::otherMouseUp() {
    _vulkanExample->mouseState.buttons.middle = false;
}

void MVKExample::mouseDragged(double x, double y) {
    _vulkanExample->mouseDragged(x, y);
}

void MVKExample::scrollWheel(short wheelDelta) {
    _vulkanExample->camera.translate(glm::vec3(0.0f, 0.0f, wheelDelta * 0.05f * _vulkanExample->camera.movementSpeed));
	_vulkanExample->viewUpdated = true;
}

void MVKExample::fullScreen(bool fullscreen) {
	_vulkanExample->settings.fullscreen = fullscreen;
}

MVKExample::MVKExample(void* view, double scaleUI) {
    _vulkanExample = new VulkanExample();
    _vulkanExample->initVulkan();
    _vulkanExample->setupWindow(view);
	_vulkanExample->settings.vsync = true;		// SRS - set vsync flag since this iOS/macOS example app uses displayLink vsync rendering
	_vulkanExample->ui.scale = scaleUI;	        // SRS - set UIOverlay scale to maintain relative proportions/readability on retina displays
    _vulkanExample->prepare();
	_vulkanExample->renderLoop();				// SRS - this inits destWidth/destHeight/lastTimestamp/tPrevEnd, then falls through and returns
}

MVKExample::~MVKExample() {
	vkDeviceWaitIdle(_vulkanExample->vulkanDevice->logicalDevice);
    delete(_vulkanExample);
}
