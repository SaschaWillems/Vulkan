/*
 * DemoViewController.mm
 *
 * Copyright (c) 2014-2017 The Brenwill Workshop Ltd. All rights reserved.
 * http://www.brenwill.com
 */

#import "DemoViewController.h"

#include "examples.h"


const std::string VulkanExampleBase::getAssetPath() {
    return [NSBundle.mainBundle.resourcePath stringByAppendingString: @"/data/"].UTF8String;
}


#pragma mark -
#pragma mark DemoViewController

@implementation DemoViewController {
    VulkanExample* _vulkanExample;
    CADisplayLink* _displayLink;
}

/** Since this is a single-view app, init Vulkan when the view is loaded. */
-(void) viewDidLoad {
	[super viewDidLoad];

    _vulkanExample = new VulkanExample();
    _vulkanExample->initVulkan();
    _vulkanExample->setupWindow(self.view);
    _vulkanExample->initSwapchain();
    _vulkanExample->prepare();

    uint32_t fps = 60;
    _displayLink = [CADisplayLink displayLinkWithTarget: self selector: @selector(renderFrame)];
    [_displayLink setFrameInterval: 60 / fps];
    [_displayLink addToRunLoop: NSRunLoop.currentRunLoop forMode: NSDefaultRunLoopMode];
}

-(void) renderFrame {
    _vulkanExample->renderFrame();
}

-(void) dealloc {
    delete(_vulkanExample);
    [super dealloc];
}

@end


#pragma mark -
#pragma mark DemoView

@implementation DemoView

/** Returns a Metal-compatible layer. */
+(Class) layerClass { return [CAMetalLayer class]; }

@end

