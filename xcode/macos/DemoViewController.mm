/*
 * DemoViewController.mm
 *
 * Copyright (c) 2014-2017 The Brenwill Workshop Ltd. All rights reserved.
 * http://www.brenwill.com
 */

#import "DemoViewController.h"
#import <QuartzCore/CAMetalLayer.h>

#include "examples.h"


const std::string VulkanExampleBase::getAssetPath() {
    return [NSBundle.mainBundle.resourcePath stringByAppendingString: @"/data/"].UTF8String;
}

/** Rendering loop callback function for use with a CVDisplayLink. */
static CVReturn DisplayLinkCallback(CVDisplayLinkRef displayLink,
                                    const CVTimeStamp* now,
                                    const CVTimeStamp* outputTime,
                                    CVOptionFlags flagsIn,
                                    CVOptionFlags* flagsOut,
                                    void* target) {
    ((VulkanExample*)target)->renderFrame();
    return kCVReturnSuccess;
}


#pragma mark -
#pragma mark DemoViewController

@implementation DemoViewController {
    VulkanExample* _vulkanExample;
    CVDisplayLinkRef _displayLink;
}

/** Since this is a single-view app, initialize Vulkan during view loading. */
-(void) viewDidLoad {
	[super viewDidLoad];

	self.view.wantsLayer = YES;		// Back the view with a layer created by the makeBackingLayer method.

    _vulkanExample = new VulkanExample();
    _vulkanExample->initVulkan();
    _vulkanExample->setupWindow(self.view);
    _vulkanExample->initSwapchain();
    _vulkanExample->prepare();

    CVDisplayLinkCreateWithActiveCGDisplays(&_displayLink);
    CVDisplayLinkSetOutputCallback(_displayLink, &DisplayLinkCallback, _vulkanExample);
    CVDisplayLinkStart(_displayLink);
}

-(void) dealloc {
    CVDisplayLinkRelease(_displayLink);
    delete(_vulkanExample);
    [super dealloc];
}

// Handle keyboard input
-(void) keyDown:(NSEvent*) theEvent {
    _vulkanExample->keyPressed(theEvent.keyCode);
}

@end


#pragma mark -
#pragma mark DemoView

@implementation DemoView

/** Indicates that the view wants to draw using the backing layer instead of using drawRect:.  */
-(BOOL) wantsUpdateLayer { return YES; }

/** Returns a Metal-compatible layer. */
+(Class) layerClass { return [CAMetalLayer class]; }

/** If the wantsLayer property is set to YES, this method will be invoked to return a layer instance. */
-(CALayer*) makeBackingLayer {
    CALayer* layer = [self.class.layerClass layer];
    CGSize viewScale = [self convertSizeToBacking: CGSizeMake(1.0, 1.0)];
    layer.contentsScale = MIN(viewScale.width, viewScale.height);
    return layer;
}

-(BOOL) acceptsFirstResponder { return YES; }

@end
