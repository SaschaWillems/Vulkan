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

/** Resize the window to fit the size of the content as set by the sample code. */
-(void) viewWillAppear {
	[super viewWillAppear];

	CGSize vSz = self.view.bounds.size;
	NSWindow *window = self.view.window;
	NSRect wFrm = [window contentRectForFrameRect: window.frame];
	NSRect newWFrm = [window frameRectForContentRect: NSMakeRect(wFrm.origin.x, wFrm.origin.y, vSz.width, vSz.height)];
	[window setFrame: newWFrm display: YES animate: window.isVisible];
	[window center];
}

-(void) dealloc {
    CVDisplayLinkRelease(_displayLink);
    delete(_vulkanExample);
    [super dealloc];
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
-(CALayer*) makeBackingLayer { return [self.class.layerClass layer]; }

@end
