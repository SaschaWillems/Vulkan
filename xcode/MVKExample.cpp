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

void MVKExample::keyPressed(uint32_t keyCode) {
    _vulkanExample->keyPressed(keyCode);
}

MVKExample::MVKExample(void* view) {
    _vulkanExample = new VulkanExample();
    _vulkanExample->initVulkan();
    _vulkanExample->setupWindow(view);
    _vulkanExample->initSwapchain();
    _vulkanExample->prepare();
}

MVKExample::~MVKExample() {
    delete _vulkanExample;
}
