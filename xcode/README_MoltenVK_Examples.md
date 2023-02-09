<a class="site-logo" href="https://www.moltengl.com/moltenvk/" title="MoltenVK">
	<img src="images/MoltenVK-Logo-Banner.png" alt="MoltenVK Home" style="width:256px;height:auto">
</a>

#MoltenVK Vulkan Examples

Copyright (c) 2016-2017 [The Brenwill Workshop Ltd.](http://www.brenwill.com).
This document is licensed under the MIT license (MIT) (http://opensource.org/licenses/MIT)

*This document is written in [Markdown](http://en.wikipedia.org/wiki/Markdown) format.
For best results, use a Markdown reader.*


<a name="intro"></a>

Introduction
------------

The *Xcode* project in this folder builds and runs the *Vulkan* examples in this
repository on *iOS* and *macOS*, using the **MoltenVK** *Vulkan* driver.



<a name="installing-moltenvk"></a>

Installing MoltenVK
-------------------

The examples in this repository can be run on *iOS* and *macOS* by using
the [**MoltenVK**](http://www.moltengl.com/moltenvk/) *Vulkan* driver.

These examples require **MoltenVK 0.18.0** or greater.

Follow these instructions to install **MoltenVK**:

1. [Download](https://moltengl.com/free-trial/) the **Molten** free evaluation trial.
   This free trial includes **MoltenVK**, is full-featured, and is not time-limited.
   You must purchase a license if you wish to use **MoltenVK** for a production
   application or game, but you can use the evaluation version to run these examples.

2. Unzip the **Molten** package, and move it to a folder outside this repository.

3. Open a *Terminal* session and navigate to the directory containing this document,
   remove the existing `MoltenVK` symbolic link in this directory, and create a new
   symbolic link pointing to the `MoltenVK` directory in the **Molten** package:

   		cd path-to-this-directory
		rm MoltenVK
		ln -s path-to-Molten-package/MoltenVK

<a name="running-examples"></a>

Running the Vulkan Examples
---------------------------

The single `examples.xcodeproj` *Xcode* project can be used to run any of the examples
in this repository on either *iOS* or *macOS*. To do so, follow these instructions:

1. Open the `examples.xcodeproj` *Xcode* project.

2. Specify which of the many examples within this respository you wish to run, by opening
   the `examples.h` file within *Xcode*, and following the instructions in the comments
   within that file to indicate which of the examples you wish to run.

3. Run either the `examples-iOS` or `examples-macOS` *Xcode Scheme* to run the example in *iOS*
   or *macOS*, repectively.

4. Many of the examples include an option to press keys to control the display of features
   and scene components:

   - On *iOS*, tap on the scene to display the keyboard. Tap again on the scene to hide the keyboard.
   - On both *iOS* and *macOS*, use the numeric keys (*1, 2, 3...*) instead of function keys (*F1, F2, F3...*).
   - On both *iOS* and *macOS*, use the regular keyboard *+* and *-* keys instead of the numpad *+* and *-* keys.
   - On both *iOS* and *macOS*, use the *delete* key instead of the *escape* key.

