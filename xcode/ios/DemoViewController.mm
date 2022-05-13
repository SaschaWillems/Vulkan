/*
 *  DemoViewController.mm
 *
 *  Copyright (c) 2016-2017 The Brenwill Workshop Ltd.
 *  This code is licensed under the MIT license (MIT) (http://opensource.org/licenses/MIT)
 */

#import "DemoViewController.h"

#include "MVKExample.h"


const std::string getAssetPath() {
    return [NSBundle.mainBundle.resourcePath stringByAppendingString: @"/data/"].UTF8String;
}


#pragma mark -
#pragma mark DemoViewController

@implementation DemoViewController {
    MVKExample* _mvkExample;
    CADisplayLink* _displayLink;
    BOOL _viewHasAppeared;
}

/** Since this is a single-view app, init Vulkan when the view is loaded. */
-(void) viewDidLoad {
	[super viewDidLoad];

    self.view.contentScaleFactor = UIScreen.mainScreen.nativeScale;

    _mvkExample = new MVKExample(self.view);

    uint32_t fps = 60;
    _displayLink = [CADisplayLink displayLinkWithTarget: self selector: @selector(renderFrame)];
    [_displayLink setFrameInterval: 60 / fps];
    [_displayLink addToRunLoop: NSRunLoop.currentRunLoop forMode: NSDefaultRunLoopMode];

	// SRS - set VulkanExampleBase::refreshPeriod to calibrate the frame animation rate
	_mvkExample->setRefreshPeriod( 1.0 / fps );

	// Setup double tap gesture to toggle virtual keyboard
    UITapGestureRecognizer* tapSelector = [[UITapGestureRecognizer alloc]
                                           initWithTarget: self action: @selector(handleTapGesture:)];
    tapSelector.numberOfTapsRequired = 2;
    tapSelector.cancelsTouchesInView = YES;
    [self.view addGestureRecognizer: tapSelector];

	// SRS - Setup pinch gesture to detect and activate zoom
	UIPinchGestureRecognizer* pinchSelector = [[UIPinchGestureRecognizer alloc]
										   initWithTarget: self action: @selector(handlePinchGesture:)];
	pinchSelector.cancelsTouchesInView = YES;
	[self.view addGestureRecognizer: pinchSelector];

    _viewHasAppeared = NO;
}

-(void) viewDidAppear: (BOOL) animated {
    [super viewDidAppear: animated];
    _viewHasAppeared = YES;
}

-(BOOL) canBecomeFirstResponder { return _viewHasAppeared; }

-(void) renderFrame {
	//_mvkExample->renderFrame();
	_mvkExample->displayLinkOutputCb();   // SRS - Call displayLinkOutputCb() to animate frames vs. renderFrame() for static image
}

-(void) dealloc {
    delete _mvkExample;
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

// Display and hide the keyboard by double tapping on the view
-(void) handleTapGesture: (UITapGestureRecognizer*) gestureRecognizer {
    if (gestureRecognizer.state == UIGestureRecognizerStateEnded) {
        [self toggleKeyboard];
    }
}


#pragma mark UIKeyInput methods

// Returns whether text is available
-(BOOL) hasText { return YES; }

// A key on the keyboard has been pressed.
-(void) insertText: (NSString*) text {
	unichar keychar = (text.length > 0) ? [text characterAtIndex: 0] : 0;
	_mvkExample->keyPressed(keychar);
}

// The delete backward key has been pressed.
-(void) deleteBackward {
	_mvkExample->keyPressed(0x7F);
}


#pragma mark UITouch methods

// SRS - Handle touch events
-(CGPoint) getTouchLocalPoint:(UIEvent*) theEvent {
	UITouch *touch = [[theEvent allTouches] anyObject];
	CGPoint point = [touch locationInView:self.view];
	point.x = point.x * self.view.contentScaleFactor;
	point.y = point.y * self.view.contentScaleFactor;
	return point;
}

-(void) touchesBegan:(NSSet*) touches withEvent:(UIEvent*) theEvent {
	auto point = [self getTouchLocalPoint:theEvent];
	if (touches.count == 1) {
		// Single touch for imgui select and camera rotation
		_mvkExample->mouseDown(point.x, point.y);
	}
	else {
		// Multi-touch for swipe translation (note: pinch gesture will cancel/override)
		_mvkExample->otherMouseDown(point.x, point.y);
	}
}

-(void) touchesMoved:(NSSet*) touches withEvent:(UIEvent*) theEvent {
	auto point = [self getTouchLocalPoint:theEvent];
	_mvkExample->mouseDragged(point.x, point.y);
}

-(void) touchesEnded:(NSSet*) touches withEvent:(UIEvent*) theEvent {
	_mvkExample->mouseUp();
	_mvkExample->otherMouseUp();
}

-(void) touchesCancelled:(NSSet*) touches withEvent:(UIEvent*) theEvent {
	_mvkExample->mouseUp();
	_mvkExample->otherMouseUp();
}

// SRS - Respond to pinch gestures for zoom
-(void) handlePinchGesture: (UIPinchGestureRecognizer*) gestureRecognizer {
	if (gestureRecognizer.state == UIGestureRecognizerStateBegan) {
		auto point = [gestureRecognizer locationInView:self.view];
		point.x = point.x * self.view.contentScaleFactor;
		point.y = point.y * self.view.contentScaleFactor;
		_mvkExample->rightMouseDown(point.x, point.y);
	}
	else if (gestureRecognizer.state == UIGestureRecognizerStateChanged) {
		auto point = [gestureRecognizer locationInView:self.view];
		point.x = point.x * self.view.contentScaleFactor;
		point.y = point.y * self.view.contentScaleFactor;
		_mvkExample->mouseDragged(point.x, point.y - gestureRecognizer.view.frame.size.height / 2.0 * log(gestureRecognizer.scale));
	}
	else if (gestureRecognizer.state == UIGestureRecognizerStateEnded) {
		_mvkExample->rightMouseUp();
	}
}

@end


#pragma mark -
#pragma mark DemoView

@implementation DemoView

/** Returns a Metal-compatible layer. */
+(Class) layerClass { return [CAMetalLayer class]; }

@end

