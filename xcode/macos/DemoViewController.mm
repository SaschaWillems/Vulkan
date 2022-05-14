/*
 *  DemoViewController.mm
 *
 *  Copyright (c) 2016-2017 The Brenwill Workshop Ltd.
 *  This code is licensed under the MIT license (MIT) (http://opensource.org/licenses/MIT)
 */

#import "DemoViewController.h"
#import <QuartzCore/CAMetalLayer.h>

#include "MVKExample.h"

const std::string getAssetPath() {
    return [NSBundle.mainBundle.resourcePath stringByAppendingString: @"/data/"].UTF8String;
}

/** Rendering loop callback function for use with a CVDisplayLink. */
static CVReturn DisplayLinkCallback(CVDisplayLinkRef displayLink,
                                    const CVTimeStamp* now,
                                    const CVTimeStamp* outputTime,
                                    CVOptionFlags flagsIn,
                                    CVOptionFlags* flagsOut,
                                    void* target) {
    //((MVKExample*)target)->renderFrame();
    ((MVKExample*)target)->displayLinkOutputCb();   // SRS - Call displayLinkOutputCb() to animate frames vs. renderFrame() for static image
    return kCVReturnSuccess;
}

MVKExample* _mvkExample;

#pragma mark -
#pragma mark DemoViewController

@implementation DemoViewController {
    CVDisplayLinkRef _displayLink;
}

/** Since this is a single-view app, initialize Vulkan during view loading. */
-(void) viewDidLoad {
	[super viewDidLoad];

	self.view.wantsLayer = YES;		// Back the view with a layer created by the makeBackingLayer method.

    _mvkExample = new MVKExample(self.view);

    CVDisplayLinkCreateWithActiveCGDisplays(&_displayLink);
    CVDisplayLinkSetOutputCallback(_displayLink, &DisplayLinkCallback, _mvkExample);
    CVDisplayLinkStart(_displayLink);
}

// SRS - Center the window and set VulkanExampleBase::refreshPeriod from the active displayLink
-(void) viewWillAppear {
    [super viewWillAppear];

	NSWindow* window = self.view.window;
	[window center];

    _mvkExample->setRefreshPeriod(CVDisplayLinkGetActualOutputVideoRefreshPeriod(_displayLink));
}

-(void) dealloc {
    CVDisplayLinkRelease(_displayLink);
    delete _mvkExample;
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
-(CALayer*) makeBackingLayer {
    CALayer* layer = [self.class.layerClass layer];
    CGSize viewScale = [self convertSizeToBacking: CGSizeMake(1.0, 1.0)];
    layer.contentsScale = MIN(viewScale.width, viewScale.height);
    return layer;
}

-(BOOL) acceptsFirstResponder { return YES; }

// SRS - Handle keyboard events
-(void) keyDown:(NSEvent*) theEvent {
	NSString *text = [theEvent charactersIgnoringModifiers];
	unichar keychar = (text.length > 0) ? [text.lowercaseString characterAtIndex: 0] : 0;
    _mvkExample->keyDown(keychar);
}

-(void) keyUp:(NSEvent*) theEvent {
	NSString *text = [theEvent charactersIgnoringModifiers];
	unichar keychar = (text.length > 0) ? [text.lowercaseString characterAtIndex: 0] : 0;
    _mvkExample->keyUp(keychar);
}

// SRS - Handle mouse events
-(NSPoint) getMouseLocalPoint:(NSEvent*) theEvent {
    NSPoint location = [theEvent locationInWindow];
    NSPoint point = [self convertPoint:location fromView:nil];
    point.y = self.frame.size.height - point.y;
    return point;
}

-(void) mouseDown:(NSEvent*) theEvent {
    auto point = [self getMouseLocalPoint:theEvent];
    _mvkExample->mouseDown(point.x, point.y);
}

-(void) mouseUp:(NSEvent*) theEvent {
    _mvkExample->mouseUp();
}

-(void) rightMouseDown:(NSEvent*) theEvent {
	auto point = [self getMouseLocalPoint:theEvent];
    _mvkExample->rightMouseDown(point.x, point.y);
}

-(void) rightMouseUp:(NSEvent*) theEvent {
    _mvkExample->rightMouseUp();
}

-(void) otherMouseDown:(NSEvent*) theEvent {
	auto point = [self getMouseLocalPoint:theEvent];
    _mvkExample->otherMouseDown(point.x, point.y);
}

-(void) otherMouseUp:(NSEvent*) theEvent {
    _mvkExample->otherMouseUp();
}

-(void) mouseDragged:(NSEvent*) theEvent {
    auto point = [self getMouseLocalPoint:theEvent];
    _mvkExample->mouseDragged(point.x, point.y);
}

-(void) rightMouseDragged:(NSEvent*) theEvent {
    auto point = [self getMouseLocalPoint:theEvent];
    _mvkExample->mouseDragged(point.x, point.y);
}

-(void) otherMouseDragged:(NSEvent*) theEvent {
    auto point = [self getMouseLocalPoint:theEvent];
    _mvkExample->mouseDragged(point.x, point.y);
}

-(void) scrollWheel:(NSEvent*) theEvent {
    short wheelDelta = [theEvent deltaY];
    _mvkExample->scrollWheel(wheelDelta);
}

@end
