/*
 *  DemoViewController.mm
 *
 *  Copyright (c) 2016-2017 The Brenwill Workshop Ltd.
 *  This code is licensed under the MIT license (MIT) (http://opensource.org/licenses/MIT)
 */

#import "DemoViewController.h"
#import "AppDelegate.h"
#import <QuartzCore/CAMetalLayer.h>
#import <QuartzCore/CADisplayLink.h>

#include "MVKExample.h"

const std::string getAssetPath() {
    return [NSBundle.mainBundle.resourcePath stringByAppendingString: @"/assets/"].UTF8String;
}

const std::string getShaderBasePath() {
	return [NSBundle.mainBundle.resourcePath stringByAppendingString: @"/shaders/"].UTF8String;
}

CALayer* layer;
MVKExample* _mvkExample;

#pragma mark -
#pragma mark DemoViewController

@implementation DemoViewController {
	CADisplayLink* _displayLink;
}

/** Since this is a single-view app, initialize when the view is loaded. */
-(void) viewDidLoad {
	[super viewDidLoad];

	// SRS - Enable AppDelegate to call into DemoViewController for handling application lifecycle events (e.g. termination)
	auto appDelegate = (AppDelegate *)NSApplication.sharedApplication.delegate;
	appDelegate.viewController = self;
}

/** Initialize Vulkan and DisplayLink after the view appears - allows for better debug messaging. */
-(void) viewDidAppear {
	[super viewDidAppear];

	self.view.wantsLayer = YES;		// Back the view with a layer created by the makeBackingLayer method (called immediately on set)

	_mvkExample = new MVKExample(self.view, layer.contentsScale);	// SRS - Use backing layer scale factor for UIOverlay on macOS

	// SRS - Initialize DisplayLink after Vulkan is ready and set render loop to run at 60 fps
	const float fps = 60.0;
	_displayLink = [self.view displayLinkWithTarget: self selector: @selector(renderFrame:)];
	[_displayLink setPreferredFrameRateRange: CAFrameRateRangeMake( fps, fps, fps )];
	[_displayLink addToRunLoop: NSRunLoop.mainRunLoop forMode: NSRunLoopCommonModes];
}

-(void) renderFrame:(CADisplayLink *)sender {
	_mvkExample->displayLinkOutputCb();   // SRS - Call displayLinkOutputCb() to render/animate at displayLink frame rate
}

-(void) shutdownExample {
	_displayLink.paused = YES;
	_displayLink = nil;
    delete _mvkExample;
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
    layer = [self.class.layerClass layer];
    CGSize viewScale = [self convertSizeToBacking: CGSizeMake(1.0, 1.0)];
    layer.contentsScale = MIN(viewScale.width, viewScale.height);
    return layer;
}

// SRS - Activate mouse cursor tracking within the view, set view as window delegate, and center the window
- (void) viewDidMoveToWindow {
	auto trackingArea = [[NSTrackingArea alloc] initWithRect:[self bounds] options: (NSTrackingMouseMoved | NSTrackingActiveAlways | NSTrackingInVisibleRect) owner:self userInfo:nil];
	[self addTrackingArea: trackingArea];

	[self.window setDelegate: self.window.contentView];
	[self.window center];
}

-(BOOL) acceptsFirstResponder { return YES; }

// SRS - Handle keyboard events
-(void) keyDown:(NSEvent*) theEvent {
	NSString *text = [theEvent charactersIgnoringModifiers];
	unichar keychar = (text.length > 0) ? [text.lowercaseString characterAtIndex: 0] : 0;
	switch (keychar)
	{
		case KEY_DELETE:					// support keyboards with no escape key
		case KEY_ESCAPE:
			[NSApp terminate:nil];
			break;
		default:
			_mvkExample->keyDown(keychar);
			break;
	}
}

-(void) keyUp:(NSEvent*) theEvent {
	NSString *text = [theEvent charactersIgnoringModifiers];
	unichar keychar = (text.length > 0) ? [text.lowercaseString characterAtIndex: 0] : 0;
    _mvkExample->keyUp(keychar);
}

// SRS - Handle mouse events
-(NSPoint) getMouseLocalPoint:(NSEvent*) theEvent {
    NSPoint location = [theEvent locationInWindow];
    NSPoint point = [self convertPointToBacking:location];
    point.y = self.frame.size.height*self.window.backingScaleFactor - point.y;
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

-(void) mouseMoved:(NSEvent*) theEvent {
	auto point = [self getMouseLocalPoint:theEvent];
	_mvkExample->mouseDragged(point.x, point.y);
}

-(void) scrollWheel:(NSEvent*) theEvent {
    short wheelDelta = [theEvent deltaY];
    _mvkExample->scrollWheel(wheelDelta);
}

-(void) windowWillEnterFullScreen:(NSNotification *)notification {
	_mvkExample->fullScreen(true);
}

-(void) windowWillExitFullScreen:(NSNotification *)notification {
	_mvkExample->fullScreen(false);
}

@end
