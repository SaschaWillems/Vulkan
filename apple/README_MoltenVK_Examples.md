<a class="site-logo" href="https://www.moltengl.com/moltenvk/" title="MoltenVK">
	<img src="images/MoltenVK-Logo-Banner.png" alt="MoltenVK Home" style="width:256px;height:auto">
</a>

# MoltenVK Vulkan Examples

Copyright (c) 2016-2024 [The Brenwill Workshop Ltd.](http://www.brenwill.com).
This document is licensed under the MIT license (MIT) (http://opensource.org/licenses/MIT)

*This document is written in [Markdown](http://en.wikipedia.org/wiki/Markdown) format.
For best results, use a Markdown reader.*


<a name="intro"></a>

Introduction
------------

The *Xcode* project in this folder builds and runs the *Vulkan* examples in this
repository on *iOS*, the *iOS Simulator*, and *macOS*, using the **MoltenVK** *Vulkan* driver.



<a name="installing-moltenvk"></a>

Installing MoltenVK
-------------------

These examples require **Vulkan SDK 1.3.275.0** or later.

Follow these instructions to install the latest Vulkan SDK containing **MoltenVK**:

1. [Download](https://sdk.lunarg.com/sdk/download/latest/mac/vulkan_sdk.dmg) the latest
   **Vulkan SDK** for macOS.  This includes the required **MoltenVK** library frameworks
   for *iOS*, the *iOS Simulator*, and *macOS*.  The latest getting started information can be found at [Getting Started](https://vulkan.lunarg.com/doc/sdk/latest/mac/getting_started.html).

2. Install the downloaded **vulkansdk-macos-*version*.dmg** package to the default location.

3. Open a *Terminal* session and navigate to the directory containing this document,
   remove the existing `MoltenVK.xcframework` symbolic link in this directory, and create
   a new symbolic link pointing to `MoltenVK.xcframework` within the **Vulkan SDK**:

   		cd path-to-this-directory
		rm MoltenVK.xcframework
		ln -s path-to-VulkanSDK/macOS/lib/MoltenVK.xcframework

<a name="running-examples"></a>

Running the Vulkan Examples
---------------------------

The single `examples.xcodeproj` *Xcode* project can be used to run any of the examples
in this repository on *iOS*, the *iOS Simulator*, or *macOS*. To do so, follow these instructions:

1. Open the `examples.xcodeproj` *Xcode* project using **Xcode 14** or later.  <ins>Earlier versions of *Xcode* are not supported and will not successfully build this project</ins>.

2. Specify which of the many examples within this respository you wish to run, by opening
   the `examples.h` file within *Xcode*, and following the instructions in the comments
   within that file to indicate which of the examples you wish to run. Some examples may not be supported on *iOS* or *macOS* - please see the comments.

3. Run either the `examples-iOS` or `examples-macOS` *Xcode Scheme* to run the example in *iOS*, the *iOS Simulator*, or *macOS* respectively.

4. Many of the examples include an option to press keys to control the display of features
   and scene components:

   - On *iOS*, use one- and/or two-finger gestures to rotate, translate, or zoom the scene. On *macOS*, use the left/center/right mouse buttons or mouse wheel to rotate, translate, or zoom the scene.
   - On the *iOS Simulator*, use the left mouse button to select or rotate the scene, Shift + Option + click and drag for translation, or Option + click and drag for zoom.
   - On *iOS*, double tap on the scene to display the keyboard. Double tap again on the scene to hide the keyboard.  On the *iOS Simulator* double click to show and hide the virtual keyboard (note: you may need to press ⌘(command) + ⇧(shift) + K to switch from the hardware keyboard to the virtual keyboard).
   - On both *iOS* and *macOS*, use the numeric keys (*1, 2, 3...*) instead of function keys (*F1, F2, F3...*).
   - On both *iOS* and *macOS*, use the regular keyboard *+* and *-* keys instead of the numpad *+* and *-* keys.
   - On both *iOS* and *macOS*, use the keyboard *p* key to pause and resume animation.
   - On both *iOS* and *macOS*, use the *delete* key instead of the *escape* key.

