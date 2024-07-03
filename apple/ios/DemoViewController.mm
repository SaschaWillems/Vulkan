/*
 *  DemoViewController.mm
 *
 *  Copyright (c) 2016-2017 The Brenwill Workshop Ltd.
 *  This code is licensed under the MIT license (MIT) (http://opensource.org/licenses/MIT)
 */

#import "DemoViewController.h"
#import "AppDelegate.h"

#include "MVKExample.h"

const std::string getAssetPath() {
    return [NSBundle.mainBundle.resourcePath stringByAppendingString: @"/assets/"].UTF8String;
}

const std::string getShaderBasePath() {
	return [NSBundle.mainBundle.resourcePath stringByAppendingString: @"/shaders/"].UTF8String;
}

CALayer* layer;

#pragma mark -
#pragma mark DemoViewController

@implementation DemoViewController {
    MVKExample* _mvkExample;
    CADisplayLink* _displayLink;
    BOOL _viewHasAppeared;
	CGPoint _startPoint;
}

/** Since this is a single-view app, init Vulkan when the view is loaded. */
-(void) viewDidLoad {
	[super viewDidLoad];

	layer = [self.view layer];		// SRS - When creating a Vulkan Metal surface, need the layer backing the view

	layer.contentsScale = UIScreen.mainScreen.nativeScale;

	// SRS - Calculate UI overlay scale factor based on backing layer scale factor and device type for readable UIOverlay on iOS devices
	auto UIOverlayScale = layer.contentsScale * ([UIDevice currentDevice].userInterfaceIdiom == UIUserInterfaceIdiomPhone ? 2.0/3.0 : 1.0 );
	_mvkExample = new MVKExample(self.view, UIOverlayScale);
	
	// SRS - Enable AppDelegate to call into DemoViewController for handling app lifecycle events (e.g. termination)
	auto appDelegate = (AppDelegate *)UIApplication.sharedApplication.delegate;
	appDelegate.viewController = self;
	
    uint32_t fps = 60;
    _displayLink = [CADisplayLink displayLinkWithTarget: self selector: @selector(renderFrame)];
    [_displayLink setFrameInterval: 60 / fps];
    [_displayLink addToRunLoop: NSRunLoop.currentRunLoop forMode: NSDefaultRunLoopMode];

	// Setup double tap gesture to toggle virtual keyboard
    UITapGestureRecognizer* tapSelector = [[[UITapGestureRecognizer alloc]
                                           initWithTarget: self action: @selector(handleTapGesture:)] autorelease];
    tapSelector.numberOfTapsRequired = 2;
    tapSelector.cancelsTouchesInView = YES;
	tapSelector.requiresExclusiveTouchType = YES;
    [self.view addGestureRecognizer: tapSelector];

	// SRS - Setup pan gesture to detect and activate translation
	UIPanGestureRecognizer* panSelector = [[[UIPanGestureRecognizer alloc]
										   initWithTarget: self action: @selector(handlePanGesture:)] autorelease];
	panSelector.minimumNumberOfTouches = 2;
	panSelector.cancelsTouchesInView = YES;
	panSelector.requiresExclusiveTouchType = YES;
	[self.view addGestureRecognizer: panSelector];

	// SRS - Setup pinch gesture to detect and activate zoom
	UIPinchGestureRecognizer* pinchSelector = [[[UIPinchGestureRecognizer alloc]
										   initWithTarget: self action: @selector(handlePinchGesture:)] autorelease];
	pinchSelector.cancelsTouchesInView = YES;
	pinchSelector.requiresExclusiveTouchType = YES;
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

-(void) shutdownExample {
	[_displayLink invalidate];
	delete _mvkExample;
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
	unichar keychar = (text.length > 0) ? [text.lowercaseString characterAtIndex: 0] : 0;
	_mvkExample->keyPressed(keychar);
}

// The delete backward key has been pressed.
-(void) deleteBackward {
	_mvkExample->keyPressed(KEY_DELETE);
}


#pragma mark UITouch methods

-(CGPoint) getTouchLocalPoint:(UIEvent*) theEvent {
	UITouch *touch = [[theEvent allTouches] anyObject];
	CGPoint point = [touch locationInView: self.view];
	point.x *= self.view.contentScaleFactor;
	point.y *= self.view.contentScaleFactor;
	return point;
}

// SRS - Handle touch events
-(void) touchesBegan:(NSSet*) touches withEvent:(UIEvent*) theEvent {
	if (touches.count == 1) {
		auto point = [self getTouchLocalPoint: theEvent];
		_mvkExample->mouseDown(point.x, point.y);
	}
}

-(void) touchesMoved:(NSSet*) touches withEvent:(UIEvent*) theEvent {
	if (touches.count == 1) {
		auto point = [self getTouchLocalPoint: theEvent];
		_mvkExample->mouseDragged(point.x, point.y);
	}
}

-(void) touchesEnded:(NSSet*) touches withEvent:(UIEvent*) theEvent {
	_mvkExample->mouseUp();
}

-(void) touchesCancelled:(NSSet*) touches withEvent:(UIEvent*) theEvent {
	_mvkExample->mouseUp();
}

#pragma mark UIGesture methods

-(CGPoint) getGestureLocalPoint:(UIGestureRecognizer*) gestureRecognizer {
	CGPoint point = [gestureRecognizer locationInView: self.view];
	point.x *= self.view.contentScaleFactor;
	point.y *= self.view.contentScaleFactor;
	return point;
}

// SRS - Respond to pan gestures for translation
-(void) handlePanGesture: (UIPanGestureRecognizer*) gestureRecognizer {
	switch (gestureRecognizer.state) {
		case UIGestureRecognizerStateBegan: {
			_startPoint = [self getGestureLocalPoint: gestureRecognizer];
			_mvkExample->otherMouseDown(_startPoint.x, _startPoint.y);
			break;
		}
		case UIGestureRecognizerStateChanged: {
			auto translation = [gestureRecognizer translationInView: self.view];
			translation.x *= self.view.contentScaleFactor;
			translation.y *= self.view.contentScaleFactor;
			_mvkExample->mouseDragged(_startPoint.x + translation.x, _startPoint.y + translation.y);
			break;
		}
		default: {
			_mvkExample->otherMouseUp();
			break;
		}
	}
}

// SRS - Respond to pinch gestures for zoom
-(void) handlePinchGesture: (UIPinchGestureRecognizer*) gestureRecognizer {
	switch (gestureRecognizer.state) {
		case UIGestureRecognizerStateBegan: {
			_startPoint = [self getGestureLocalPoint: gestureRecognizer];
			_mvkExample->rightMouseDown(_startPoint.x, _startPoint.y);
			break;
		}
		case UIGestureRecognizerStateChanged: {
			_mvkExample->mouseDragged(_startPoint.x, _startPoint.y - self.view.frame.size.height * log(gestureRecognizer.scale));
			break;
		}
		default: {
			_mvkExample->rightMouseUp();
			break;
		}
	}
}

@end


#pragma mark -
#pragma mark DemoView

@implementation DemoView

/** Returns a Metal-compatible layer. */
+(Class) layerClass { return [CAMetalLayer class]; }

@end

