## Swap chain class


```cpp
#include "base/vulkanswapchain.hpp"
```

The swap chain class connects to the platform specific surface and creates a collection of images to be presented to the windowing system.

A swap chain can hold an arbitrary number of images that can be presented to the windowing system once rendering to them is finished. At minimum it will contain two images, which is then similar to OpenGL's double buffering using a back and front buffer.

### Prerequisites

In order to use the swap chain you need to explicitly request the platform specific surface extension on at instance level :

```cpp
std::vector<const char*> enabledExtensions = { VK_KHR_SURFACE_EXTENSION_NAME };

// For windows
enabledExtensions.push_back(VK_KHR_WIN32_SURFACE_EXTENSION_NAME);

VkInstanceCreateInfo instanceCreateInfo = {};
...
instanceCreateInfo.enabledExtensionCount = enabledExtensions.size();
instanceCreateInfo.ppEnabledExtensionNames = enabledExtensions.data();
```

And the swap chain extension at device level :

```cpp
std::vector<const char*> enabledExtensions = { VK_KHR_SWAPCHAIN_EXTENSION_NAME };

VkDeviceCreateInfo deviceCreateInfo = {};
...
deviceCreateInfo.enabledExtensionCount = enabledExtensions.size();
deviceCreateInfo.ppEnabledExtensionNames = enabledExtensions.data();
```

### Connecting
```cpp
VulkanSwapChain::connect();
```

### Initializing the surface
```cpp
VulkanSwapChain::initSurface();
```
This will create the platform specific surface (parameters depend on OS) that's connected to your window. The function also selects a supported color format and space to be used for the swap chain images and gets indices for the graphics and presenting queue that the images will be submitted to.

### Creating the swap chain (images)
```cpp
VulkanSwapChain::create();
```
Creates the swap chain (and destroys it if it's to be recreated, e.g. for window resize) and also creates the images and image views to be used.

Note that you need to pass a valid command buffer as this also does the initial image transitions for the created images :
```cpp
//
vkTools::setImageLayout(
  cmdBuffer,
  buffers[i].image,
  VK_IMAGE_ASPECT_COLOR_BIT,
  VK_IMAGE_LAYOUT_UNDEFINED,
  VK_IMAGE_LAYOUT_PRESENT_SRC_KHR);
```

### Using the swap chain
Once everything is setup, the swap chain can used in your render loop :
```cpp
...
// Get next image in the swap chain
err = swapChain.acquireNextImage(presentCompleteSemaphore, &currentBuffer);
assert(!err);

// Submit command buffers
VkSubmitInfo submitInfo = vkTools::initializers::submitInfo();
...
err = vkQueueSubmit(queue, 1, &submitInfo, VK_NULL_HANDLE);
assert(!err);

// Present current swap chain image to the (graphics) and presenting queue
err = swapChain.queuePresent(queue, currentBuffer);
assert(!err);
...

// The post present barrier is an important part of the render loop
// It adds an image barrier to the queue that transforms the currentBuffer
// swap chain image from VK_IMAGE_LAYOUT_PRESENT_SRC_KHR to VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL
// so it can be used as a color attachment
submitPostPresentBarrier(swapChain.buffers[currentBuffer].image);
...
```
