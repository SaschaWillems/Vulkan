/*
 *  AppDelegate.m
 *
 *  Copyright (c) 2016-2017 The Brenwill Workshop Ltd.
 *  This code is licensed under the MIT license (MIT) (http://opensource.org/licenses/MIT)
 */

#import "AppDelegate.h"
#import "DemoViewController.h"

@interface AppDelegate ()

@end

@implementation AppDelegate

- (void)applicationDidFinishLaunching:(NSNotification *)aNotification {
    // Insert code here to initialize your application
}

- (void)applicationWillTerminate:(NSNotification *)aNotification {
    // Insert code here to tear down your application
	[(DemoViewController *)_viewController shutdownExample];
}

- (BOOL)applicationShouldTerminateAfterLastWindowClosed:(NSApplication *)sender {
    return YES;
}

@end
