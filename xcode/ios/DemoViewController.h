/*
 *  DemoViewController.h
 *
 *  Copyright (c) 2016-2017 The Brenwill Workshop Ltd.
 *  This code is licensed under the MIT license (MIT) (http://opensource.org/licenses/MIT)
 */

#import <UIKit/UIKit.h>


#pragma mark -
#pragma mark DemoViewController

/** The main view controller for the demo storyboard. */
@interface DemoViewController : UIViewController <UIKeyInput>
@end


#pragma mark -
#pragma mark DemoView

/** The Metal-compatibile view for the demo Storyboard. */
@interface DemoView : UIView
@end

