## Vulkan example base class
All examples are derived from a base class that encapsulates common used Vulkan functionality and all the setup stuff that's not necessary to repeat for each example. It also contains functions to load shaders and an easy wrapper to enable debugging via the validation layers.

If you want to create an example based on this base class, simply derive :

```cpp
#include "vulkanexamplebase.h"
...
class MyVulkanExample : public VulkanExampleBase
{
  ...
  VulkanExample()
  {
    width = 1024;
    height = 1024;
    zoom = -15;
    rotation = glm::vec3(-45.0, 22.5, 0.0);
    title = "My new Vulkan Example";
  }
}
```
##### Validation layers
The example base class offers a constructor overload for enabling a default set of Vulkan validation layers (for debugging purposes). If you want to use this functionality, simply use the construtor override :
```cpp
VulkanExample() : VulkanExampleBase(true)
{
  ...
}
```

This will cause a console window to be displayed containing all validation errors.

If you also want to display validation warnings, you need to uncomment the following line in "vulkandebug.cpp" (base directory) :

```cpp
if (flags & VK_DBG_REPORT_WARN_BIT)
{
  // Uncomment to see warnings
  // std::cout << "WARNING: " << "[" << pLayerPrefix << "] Code " << msgCode << " : " << pMsg << "\n";
}

```
