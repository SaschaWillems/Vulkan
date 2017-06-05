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
    BOOL _viewHasAppeared;
}

/** Since this is a single-view app, init Vulkan when the view is loaded. */
-(void) viewDidLoad {
	[super viewDidLoad];

    self.view.contentScaleFactor = UIScreen.mainScreen.nativeScale;

    _vulkanExample = new VulkanExample();
    _vulkanExample->initVulkan();
    _vulkanExample->setupWindow(self.view);
    _vulkanExample->initSwapchain();
    _vulkanExample->prepare();

    uint32_t fps = 60;
    _displayLink = [CADisplayLink displayLinkWithTarget: self selector: @selector(renderFrame)];
    [_displayLink setFrameInterval: 60 / fps];
    [_displayLink addToRunLoop: NSRunLoop.currentRunLoop forMode: NSDefaultRunLoopMode];

    // Setup tap gesture to toggle virtual keyboard
    UITapGestureRecognizer* tapSelector = [[UITapGestureRecognizer alloc]
                                           initWithTarget: self action: @selector(handleTapGesture:)];
    tapSelector.numberOfTapsRequired = 1;
    tapSelector.cancelsTouchesInView = YES;
    [self.view addGestureRecognizer: tapSelector];

    _viewHasAppeared = NO;
}

-(void) viewDidAppear: (BOOL) animated {
    [super viewDidAppear: animated];
    _viewHasAppeared = YES;
}

-(BOOL) canBecomeFirstResponder { return _viewHasAppeared; }

-(void) renderFrame {
    _vulkanExample->renderFrame();
}

-(void) dealloc {
    delete(_vulkanExample);
    [super dealloc];
}

// Toggle the display of the virtual keyboard
-(void) toggleKeyboard {
    if (self.isFirstResponder) {
        [self resignFirstResponder];
    } else {
        [self becomeFirstResponder];
    }
}

// Display and hide the keyboard by tapping on the view
-(void) handleTapGesture: (UITapGestureRecognizer*) gestureRecognizer {
    if (gestureRecognizer.state == UIGestureRecognizerStateEnded) {
        [self toggleKeyboard];
    }
}

// Handle keyboard input
-(void) handleKeyboardInput: (unichar) keycode {
    _vulkanExample->keyPressed(keycode);
}


#pragma mark UIKeyInput methods

// Returns whether text is available
-(BOOL) hasText { return YES; }

// A key on the keyboard has been pressed.
-(void) insertText: (NSString*) text {
    unichar keycode = (text.length > 0) ? [text characterAtIndex: 0] : 0;
    [self handleKeyboardInput: keycode];
}

// The delete backward key has been pressed.
-(void) deleteBackward {
    [self handleKeyboardInput: 0x33];
}


@end


#pragma mark -
#pragma mark DemoView

@implementation DemoView

/** Returns a Metal-compatible layer. */
+(Class) layerClass { return [CAMetalLayer class]; }

@end

