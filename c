[1mdiff --git a/base/vulkanexamplebase.cpp b/base/vulkanexamplebase.cpp[m
[1mindex ebcf2ab..d44a18c 100644[m
[1m--- a/base/vulkanexamplebase.cpp[m
[1m+++ b/base/vulkanexamplebase.cpp[m
[36m@@ -8,8 +8,7 @@[m
 [m
 #include "vulkanexamplebase.h"[m
 [m
[31m-VkResult [m
[31m-VulkanExampleBase::createInstance(bool aEnableValidation)[m
[32m+[m[32mVkResult VulkanExampleBase::createInstance(bool aEnableValidation)[m
 {[m
 	this->enableValidation = aEnableValidation;[m
 [m
[36m@@ -51,8 +50,7 @@[m [mVulkanExampleBase::createInstance(bool aEnableValidation)[m
 	return vkCreateInstance(&instanceCreateInfo, nullptr, &instance);[m
 }[m
 [m
[31m-VkResult [m
[31m-VulkanExampleBase::createDevice(VkDeviceQueueCreateInfo aRequestedQueues, bool aEnableValidation)[m
[32m+[m[32mVkResult VulkanExampleBase::createDevice(VkDeviceQueueCreateInfo aRequestedQueues, bool aEnableValidation)[m
 {[m
 	std::vector<const char*> enabledExtensions = { VK_KHR_SWAPCHAIN_EXTENSION_NAME };[m
 [m
[36m@@ -96,8 +94,7 @@[m [mbool VulkanExampleBase::checkCommandBuffers()[m
 	return true;[m
 }[m
 [m
[31m-void [m
[31m-VulkanExampleBase::createCommandBuffers()[m
[32m+[m[32mvoid VulkanExampleBase::createCommandBuffers()[m
 {[m
 	// Create one command buffer per frame buffer[m
 	// in the swap chain[m
[36m@@ -127,16 +124,14 @@[m [mVulkanExampleBase::createCommandBuffers()[m
 	assert(!vkRes);[m
 }[m
 [m
[31m-void [m
[31m-VulkanExampleBase::destroyCommandBuffers()[m
[32m+[m[32mvoid VulkanExampleBase::destroyCommandBuffers()[m
 {[m
 	vkFreeCommandBuffers(device, cmdPool, (uint32_t)drawCmdBuffers.size(), drawCmdBuffers.data());[m
 	vkFreeCommandBuffers(device, cmdPool, 1, &prePresentCmdBuffer);[m
 	vkFreeCommandBuffers(device, cmdPool, 1, &postPresentCmdBuffer);[m
 }[m
 [m
[31m-void [m
[31m-VulkanExampleBase::createSetupCommandBuffer()[m
[32m+[m[32mvoid VulkanExampleBase::createSetupCommandBuffer()[m
 {[m
 	if (setupCmdBuffer != VK_NULL_HANDLE)[m
 	{[m
[36m@@ -160,8 +155,7 @@[m [mVulkanExampleBase::createSetupCommandBuffer()[m
 	assert(!vkRes);[m
 }[m
 [m
[31m-void [m
[31m-VulkanExampleBase::flushSetupCommandBuffer()[m
[32m+[m[32mvoid VulkanExampleBase::flushSetupCommandBuffer()[m
 {[m
 	VkResult err;[m
 [m
[36m@@ -186,8 +180,7 @@[m [mVulkanExampleBase::flushSetupCommandBuffer()[m
 	setupCmdBuffer = VK_NULL_HANDLE; [m
 }[m
 [m
[31m-void [m
[31m-VulkanExampleBase::createPipelineCache()[m
[32m+[m[32mvoid VulkanExampleBase::createPipelineCache()[m
 {[m
 	VkPipelineCacheCreateInfo pipelineCacheCreateInfo = {};[m
 	pipelineCacheCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_CACHE_CREATE_INFO;[m
[36m@@ -195,8 +188,7 @@[m [mVulkanExampleBase::createPipelineCache()[m
 	assert(!err);[m
 }[m
 [m
[31m-void [m
[31m-VulkanExampleBase::prepare()[m
[32m+[m[32mvoid VulkanExampleBase::prepare()[m
 {[m
 	if (enableValidation) {[m
 		vkDebug::setupDebugging(instance, VK_DEBUG_REPORT_ERROR_BIT_EXT | VK_DEBUG_REPORT_WARNING_BIT_EXT, NULL);[m
[36m@@ -216,8 +208,7 @@[m [mVulkanExampleBase::prepare()[m
 	textureLoader = new vkTools::VulkanTextureLoader(physicalDevice, device, queue, cmdPool);[m
 }[m
 [m
[31m-VkPipelineShaderStageCreateInfo [m
[31m-VulkanExampleBase::loadShader(const char * fileName, VkShaderStageFlagBits stage)[m
[32m+[m[32mVkPipelineShaderStageCreateInfo VulkanExampleBase::loadShader(const char * fileName, VkShaderStageFlagBits stage)[m
 {[m
 	VkPipelineShaderStageCreateInfo shaderStage = {};[m
 	shaderStage.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;[m
[36m@@ -264,8 +255,7 @@[m [mVkBool32 VulkanExampleBase::createBuffer([m
 	return true;[m
 }[m
 [m
[31m-VkBool32 [m
[31m-VulkanExampleBase::createBuffer(VkBufferUsageFlags usage, VkDeviceSize size, void * data, VkBuffer * buffer, VkDeviceMemory * memory, VkDescriptorBufferInfo * descriptor)[m
[32m+[m[32mVkBool32 VulkanExampleBase::createBuffer(VkBufferUsageFlags usage, VkDeviceSize size, void * data, VkBuffer * buffer, VkDeviceMemory * memory, VkDescriptorBufferInfo * descriptor)[m
 {[m
 	VkBool32 res = createBuffer(usage, size, data, buffer, memory);[m
 	if (res)[m
[36m@@ -303,8 +293,7 @@[m [mvoid VulkanExampleBase::loadMesh([m
 }[m
 #endif[m
 [m
[31m-void [m
[31m-VulkanExampleBase::renderLoop()[m
[32m+[m[32mvoid VulkanExampleBase::renderLoop()[m
 {[m
 #if defined(_WIN32)[m
 	MSG msg;[m
[36m@@ -650,8 +639,7 @@[m [mVulkanExampleBase::~VulkanExampleBase()[m
 #endif[m
 }[m
 [m
[31m-void [m
[31m-VulkanExampleBase::initVulkan(bool aEnableValidation)[m
[32m+[m[32mvoid VulkanExampleBase::initVulkan(bool aEnableValidation)[m
 {[m
 	VkResult err;[m
 [m
[36m@@ -753,8 +741,7 @@[m [mVulkanExampleBase::initVulkan(bool aEnableValidation)[m
 [m
 #if defined(_WIN32)[m
 // Win32 : Sets up a console window and redirects standard output to it[m
[31m-void [m
[31m-VulkanExampleBase::setupConsole(std::string aTitle)[m
[32m+[m[32mvoid VulkanExampleBase::setupConsole(std::string aTitle)[m
 {[m
 	AllocConsole();[m
 	AttachConsole(GetCurrentProcessId());[m
[36m@@ -771,8 +758,7 @@[m [mVulkanExampleBase::setupConsole(std::string aTitle)[m
 	}[m
 }[m
 [m
[31m-HWND [m
[31m-VulkanExampleBase::setupWindow(HINSTANCE hinstance, WNDPROC wndproc)[m
[32m+[m[32mHWND VulkanExampleBase::setupWindow(HINSTANCE hinstance, WNDPROC wndproc)[m
 {[m
 	this->windowInstance = hinstance;[m
 [m
[36m@@ -884,8 +870,7 @@[m [mVulkanExampleBase::setupWindow(HINSTANCE hinstance, WNDPROC wndproc)[m
 	return window;[m
 }[m
 [m
[31m-void [m
[31m-VulkanExampleBase::handleMessages(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)[m
[32m+[m[32mvoid VulkanExampleBase::handleMessages(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)[m
 {[m
 	switch (uMsg) {[m
 	case WM_CLOSE:[m
[36m@@ -1120,8 +1105,7 @@[m [mvoid VulkanExampleBase::handleEvent(const xcb_generic_event_t *event)[m
 }[m
 #endif[m
 [m
[31m-void [m
[31m-VulkanExampleBase::viewChanged()[m
[32m+[m[32mvoid VulkanExampleBase::viewChanged()[m
 {[m
 	// Can be overrdiden in derived class[m
 }[m
[36m@@ -1131,8 +1115,7 @@[m [mvoid VulkanExampleBase::keyPressed(uint32_t keyCode)[m
 	// Can be overriden in derived class[m
 }[m
 [m
[31m-VkBool32 [m
[31m-VulkanExampleBase::getMemoryType(uint32_t typeBits, VkFlags properties, uint32_t * typeIndex)[m
[32m+[m[32mVkBool32 VulkanExampleBase::getMemoryType(uint32_t typeBits, VkFlags properties, uint32_t * typeIndex)[m
 {[m
 	for (uint32_t i = 0; i < 32; i++) {[m
 		if ((typeBits & 1) == 1) {[m
[36m@@ -1146,8 +1129,7 @@[m [mVulkanExampleBase::getMemoryType(uint32_t typeBits, VkFlags properties, uint32_t[m
 	return false;[m
 }[m
 [m
[31m-void [m
[31m-VulkanExampleBase::createCommandPool()[m
[32m+[m[32mvoid VulkanExampleBase::createCommandPool()[m
 {[m
 	VkCommandPoolCreateInfo cmdPoolInfo = {};[m
 	cmdPoolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;[m
[36m@@ -1157,8 +1139,7 @@[m [mVulkanExampleBase::createCommandPool()[m
 	assert(!vkRes);[m
 }[m
 [m
[31m-void [m
[31m-VulkanExampleBase::setupDepthStencil()[m
[32m+[m[32mvoid VulkanExampleBase::setupDepthStencil()[m
 {[m
 	VkImageCreateInfo image = {};[m
 	image.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;[m
[36m@@ -1217,8 +1198,7 @@[m [mVulkanExampleBase::setupDepthStencil()[m
 	assert(!err);[m
 }[m
 [m
[31m-void [m
[31m-VulkanExampleBase::setupFrameBuffer()[m
[32m+[m[32mvoid VulkanExampleBase::setupFrameBuffer()[m
 {[m
 	VkImageView attachments[2];[m
 [m
[36m@@ -1244,8 +1224,7 @@[m [mVulkanExampleBase::setupFrameBuffer()[m
 	}[m
 }[m
 [m
[31m-void [m
[31m-VulkanExampleBase::setupRenderPass()[m
[32m+[m[32mvoid VulkanExampleBase::setupRenderPass()[m
 {[m
 	VkAttachmentDescription attachments[2];[m
 	attachments[0].format = colorformat;[m
[36m@@ -1302,8 +1281,7 @@[m [mVulkanExampleBase::setupRenderPass()[m
 	assert(!err);[m
 }[m
 [m
[31m-void [m
[31m-VulkanExampleBase::initSwapchain()[m
[32m+[m[32mvoid VulkanExampleBase::initSwapchain()[m
 {[m
 #if defined(_WIN32)[m
 	swapChain.initSurface(windowInstance, window);[m
[36m@@ -1314,8 +1292,7 @@[m [mVulkanExampleBase::initSwapchain()[m
 #endif[m
 }[m
 [m
[31m-void [m
[31m-VulkanExampleBase::setupSwapChain()[m
[32m+[m[32mvoid VulkanExampleBase::setupSwapChain()[m
 {[m
 	swapChain.create(setupCmdBuffer, &ScreenProperties.Width, &ScreenProperties.Height);[m
 }[m
[1mdiff --git a/base/vulkanexamplebase.vcxproj b/base/vulkanexamplebase.vcxproj[m
[1mindex 4e48312..2eca9e2 100644[m
[1m--- a/base/vulkanexamplebase.vcxproj[m
[1m+++ b/base/vulkanexamplebase.vcxproj[m
[36m@@ -70,12 +70,12 @@[m
   </ImportGroup>[m
   <PropertyGroup Label="UserMacros" />[m
   <PropertyGroup Condition="'$(Configuration)|$(Platform)'=='Release|x64'">[m
[31m-    <OutDir>..\..\$(Platform).$(Configuration)\</OutDir>[m
[32m+[m[32m    <OutDir>$(SolutionDir)\bin\</OutDir>[m
     <IntDir>..\..\obj\$(Platform).$(Configuration)\$(ProjectName)\</IntDir>[m
   </PropertyGroup>[m
   <PropertyGroup Condition="'$(Configuration)|$(Platform)'=='Debug|x64'">[m
     <IntDir>..\..\obj\$(Platform).$(Configuration)\$(ProjectName)\</IntDir>[m
[31m-    <OutDir>..\..\$(Platform).$(Configuration)\</OutDir>[m
[32m+[m[32m    <OutDir>$(SolutionDir)\bin\</OutDir>[m
   </PropertyGroup>[m
   <ItemDefinitionGroup Condition="'$(Configuration)|$(Platform)'=='Debug|Win32'">[m
     <ClCompile>[m
[1mdiff --git a/bloom/bloom.vcxproj b/bloom/bloom.vcxproj[m
[1mindex 5963614..9173bc6 100644[m
[1m--- a/bloom/bloom.vcxproj[m
[1m+++ b/bloom/bloom.vcxproj[m
[36m@@ -38,11 +38,11 @@[m
   <PropertyGroup Label="UserMacros" />[m
   <PropertyGroup Condition="'$(Configuration)|$(Platform)'=='Debug|x64'">[m
     <LinkIncremental>true</LinkIncremental>[m
[31m-    <OutDir>..\..\$(Platform).$(Configuration)\</OutDir>[m
[32m+[m[32m    <OutDir>$(SolutionDir)\bin\</OutDir>[m
     <IntDir>..\..\obj\$(Platform).$(Configuration)\$(ProjectName)\</IntDir>[m
   </PropertyGroup>[m
   <PropertyGroup Condition="'$(Configuration)|$(Platform)'=='Release|x64'">[m
[31m-    <OutDir>..\..\$(Platform).$(Configuration)\</OutDir>[m
[32m+[m[32m    <OutDir>$(SolutionDir)\bin\</OutDir>[m
     <IntDir>..\..\obj\$(Platform).$(Configuration)\$(ProjectName)\</IntDir>[m
   </PropertyGroup>[m
   <ItemDefinitionGroup Condition="'$(Configuration)|$(Platform)'=='Debug|x64'">[m
[1mdiff --git a/computeparticles/computeparticles.vcxproj b/computeparticles/computeparticles.vcxproj[m
[1mindex 1b19c3c..27b8de9 100644[m
[1m--- a/computeparticles/computeparticles.vcxproj[m
[1m+++ b/computeparticles/computeparticles.vcxproj[m
[36m@@ -38,11 +38,11 @@[m
   <PropertyGroup Label="UserMacros" />[m
   <PropertyGroup Condition="'$(Configuration)|$(Platform)'=='Debug|x64'">[m
     <LinkIncremental>true</LinkIncremental>[m
[31m-    <OutDir>..\..\$(Platform).$(Configuration)\</OutDir>[m
[32m+[m[32m    <OutDir>$(SolutionDir)\bin\</OutDir>[m
     <IntDir>..\..\obj\$(Platform).$(Configuration)\$(ProjectName)\</IntDir>[m
   </PropertyGroup>[m
   <PropertyGroup Condition="'$(Configuration)|$(Platform)'=='Release|x64'">[m
[31m-    <OutDir>..\..\$(Platform).$(Configuration)\</OutDir>[m
[32m+[m[32m    <OutDir>$(SolutionDir)\bin\</OutDir>[m
     <IntDir>..\..\obj\$(Platform).$(Configuration)\$(ProjectName)\</IntDir>[m
   </PropertyGroup>[m
   <ItemDefinitionGroup Condition="'$(Configuration)|$(Platform)'=='Debug|x64'">[m
[1mdiff --git a/computeshader/computeshader.vcxproj b/computeshader/computeshader.vcxproj[m
[1mindex 75f8920..8dc300e 100644[m
[1m--- a/computeshader/computeshader.vcxproj[m
[1m+++ b/computeshader/computeshader.vcxproj[m
[36m@@ -38,11 +38,11 @@[m
   <PropertyGroup Label="UserMacros" />[m
   <PropertyGroup Condition="'$(Configuration)|$(Platform)'=='Debug|x64'">[m
     <LinkIncremental>true</LinkIncremental>[m
[31m-    <OutDir>..\..\$(Platform).$(Configuration)\</OutDir>[m
[32m+[m[32m    <OutDir>$(SolutionDir)\bin\</OutDir>[m
     <IntDir>..\..\obj\$(Platform).$(Configuration)\$(ProjectName)\</IntDir>[m
   </PropertyGroup>[m
   <PropertyGroup Condition="'$(Configuration)|$(Platform)'=='Release|x64'">[m
[31m-    <OutDir>..\..\$(Platform).$(Configuration)\</OutDir>[m
[32m+[m[32m    <OutDir>$(SolutionDir)\bin\</OutDir>[m
     <IntDir>..\..\obj\$(Platform).$(Configuration)\$(ProjectName)\</IntDir>[m
   </PropertyGroup>[m
   <ItemDefinitionGroup Condition="'$(Configuration)|$(Platform)'=='Debug|x64'">[m
[1mdiff --git a/deferred/deferred.vcxproj b/deferred/deferred.vcxproj[m
[1mindex 6bcefe1..c13c568 100644[m
[1m--- a/deferred/deferred.vcxproj[m
[1m+++ b/deferred/deferred.vcxproj[m
[36m@@ -38,11 +38,11 @@[m
   <PropertyGroup Label="UserMacros" />[m
   <PropertyGroup Condition="'$(Configuration)|$(Platform)'=='Debug|x64'">[m
     <LinkIncremental>true</LinkIncremental>[m
[31m-    <OutDir>..\..\$(Platform).$(Configuration)\</OutDir>[m
[32m+[m[32m    <OutDir>$(SolutionDir)\bin\</OutDir>[m
     <IntDir>..\..\obj\$(Platform).$(Configuration)\$(ProjectName)\</IntDir>[m
   </PropertyGroup>[m
   <PropertyGroup Condition="'$(Configuration)|$(Platform)'=='Release|x64'">[m
[31m-    <OutDir>..\..\$(Platform).$(Configuration)\</OutDir>[m
[32m+[m[32m    <OutDir>$(SolutionDir)\bin\</OutDir>[m
     <IntDir>..\..\obj\$(Platform).$(Configuration)\$(ProjectName)\</IntDir>[m
   </PropertyGroup>[m
   <ItemDefinitionGroup Condition="'$(Configuration)|$(Platform)'=='Debug|x64'">[m
[1mdiff --git a/displacement/displacement.vcxproj b/displacement/displacement.vcxproj[m
[1mindex 49b2442..4a030e9 100644[m
[1m--- a/displacement/displacement.vcxproj[m
[1m+++ b/displacement/displacement.vcxproj[m
[36m@@ -38,11 +38,11 @@[m
   <PropertyGroup Label="UserMacros" />[m
   <PropertyGroup Condition="'$(Configuration)|$(Platform)'=='Debug|x64'">[m
     <LinkIncremental>true</LinkIncremental>[m
[31m-    <OutDir>..\..\$(Platform).$(Configuration)\</OutDir>[m
[32m+[m[32m    <OutDir>$(SolutionDir)\bin\</OutDir>[m
     <IntDir>..\..\obj\$(Platform).$(Configuration)\$(ProjectName)\</IntDir>[m
   </PropertyGroup>[m
   <PropertyGroup Condition="'$(Configuration)|$(Platform)'=='Release|x64'">[m
[31m-    <OutDir>..\..\$(Platform).$(Configuration)\</OutDir>[m
[32m+[m[32m    <OutDir>$(SolutionDir)\bin\</OutDir>[m
     <IntDir>..\..\obj\$(Platform).$(Configuration)\$(ProjectName)\</IntDir>[m
   </PropertyGroup>[m
   <ItemDefinitionGroup Condition="'$(Configuration)|$(Platform)'=='Debug|x64'">[m
[1mdiff --git a/distancefieldfonts/distancefieldfonts.vcxproj b/distancefieldfonts/distancefieldfonts.vcxproj[m
[1mindex 2521365..1e657d7 100644[m
[1m--- a/distancefieldfonts/distancefieldfonts.vcxproj[m
[1m+++ b/distancefieldfonts/distancefieldfonts.vcxproj[m
[36m@@ -38,11 +38,11 @@[m
   <PropertyGroup Label="UserMacros" />[m
   <PropertyGroup Condition="'$(Configuration)|$(Platform)'=='Debug|x64'">[m
     <LinkIncremental>true</LinkIncremental>[m
[31m-    <OutDir>..\..\$(Platform).$(Configuration)\</OutDir>[m
[32m+[m[32m    <OutDir>$(SolutionDir)\bin\</OutDir>[m
     <IntDir>..\..\obj\$(Platform).$(Configuration)\$(ProjectName)\</IntDir>[m
   </PropertyGroup>[m
   <PropertyGroup Condition="'$(Configuration)|$(Platform)'=='Release|x64'">[m
[31m-    <OutDir>..\..\$(Platform).$(Configuration)\</OutDir>[m
[32m+[m[32m    <OutDir>$(SolutionDir)\bin\</OutDir>[m
     <IntDir>..\..\obj\$(Platform).$(Configuration)\$(ProjectName)\</IntDir>[m
   </PropertyGroup>[m
   <ItemDefinitionGroup Condition="'$(Configuration)|$(Platform)'=='Debug|x64'">[m
[1mdiff --git a/gears/gears.vcxproj b/gears/gears.vcxproj[m
[1mindex 36b738f..c5407a1 100644[m
[1m--- a/gears/gears.vcxproj[m
[1m+++ b/gears/gears.vcxproj[m
[36m@@ -38,11 +38,11 @@[m
   <PropertyGroup Label="UserMacros" />[m
   <PropertyGroup Condition="'$(Configuration)|$(Platform)'=='Debug|x64'">[m
     <LinkIncremental>true</LinkIncremental>[m
[31m-    <OutDir>..\..\$(Platform).$(Configuration)\</OutDir>[m
[32m+[m[32m    <OutDir>$(SolutionDir)\bin\</OutDir>[m
     <IntDir>..\..\obj\$(Platform).$(Configuration)\$(ProjectName)\</IntDir>[m
   </PropertyGroup>[m
   <PropertyGroup Condition="'$(Configuration)|$(Platform)'=='Release|x64'">[m
[31m-    <OutDir>..\..\$(Platform).$(Configuration)\</OutDir>[m
[32m+[m[32m    <OutDir>$(SolutionDir)\bin\</OutDir>[m
     <IntDir>..\..\obj\$(Platform).$(Configuration)\$(ProjectName)\</IntDir>[m
   </PropertyGroup>[m
   <ItemDefinitionGroup Condition="'$(Configuration)|$(Platform)'=='Debug|x64'">[m
[1mdiff --git a/geometryshader/geometryshader.vcxproj b/geometryshader/geometryshader.vcxproj[m
[1mindex 12889ce..49821df 100644[m
[1m--- a/geometryshader/geometryshader.vcxproj[m
[1m+++ b/geometryshader/geometryshader.vcxproj[m
[36m@@ -38,11 +38,11 @@[m
   <PropertyGroup Label="UserMacros" />[m
   <PropertyGroup Condition="'$(Configuration)|$(Platform)'=='Debug|x64'">[m
     <LinkIncremental>true</LinkIncremental>[m
[31m-    <OutDir>..\..\$(Platform).$(Configuration)\</OutDir>[m
[32m+[m[32m    <OutDir>$(SolutionDir)\bin\</OutDir>[m
     <IntDir>..\..\obj\$(Platform).$(Configuration)\$(ProjectName)\</IntDir>[m
   </PropertyGroup>[m
   <PropertyGroup Condition="'$(Configuration)|$(Platform)'=='Release|x64'">[m
[31m-    <OutDir>..\..\$(Platform).$(Configuration)\</OutDir>[m
[32m+[m[32m    <OutDir>$(SolutionDir)\bin\</OutDir>[m
     <IntDir>..\..\obj\$(Platform).$(Configuration)\$(ProjectName)\</IntDir>[m
   </PropertyGroup>[m
   <ItemDefinitionGroup Condition="'$(Configuration)|$(Platform)'=='Debug|x64'">[m
[1mdiff --git a/instancing/instancing.vcxproj b/instancing/instancing.vcxproj[m
[1mindex 777e41b..b848c68 100644[m
[1m--- a/instancing/instancing.vcxproj[m
[1m+++ b/instancing/instancing.vcxproj[m
[36m@@ -31,11 +31,11 @@[m
   <PropertyGroup Label="UserMacros" />[m
   <PropertyGroup Condition="'$(Configuration)|$(Platform)'=='Debug|x64'">[m
     <LinkIncremental>true</LinkIncremental>[m
[31m-    <OutDir>..\..\$(Platform).$(Configuration)\</OutDir>[m
[32m+[m[32m    <OutDir>$(SolutionDir)\bin\</OutDir>[m
     <IntDir>..\..\obj\$(Platform).$(Configuration)\$(ProjectName)\</IntDir>[m
   </PropertyGroup>[m
   <PropertyGroup Condition="'$(Configuration)|$(Platform)'=='Release|x64'">[m
[31m-    <OutDir>..\..\$(Platform).$(Configuration)\</OutDir>[m
[32m+[m[32m    <OutDir>$(SolutionDir)\bin\</OutDir>[m
     <IntDir>..\..\obj\$(Platform).$(Configuration)\$(ProjectName)\</IntDir>[m
   </PropertyGroup>[m
   <ItemDefinitionGroup Condition="'$(Configuration)|$(Platform)'=='Debug|x64'">[m
[1mdiff --git a/mesh/mesh.vcxproj b/mesh/mesh.vcxproj[m
[1mindex dc096d7..159d695 100644[m
[1m--- a/mesh/mesh.vcxproj[m
[1m+++ b/mesh/mesh.vcxproj[m
[36m@@ -31,11 +31,11 @@[m
   <PropertyGroup Label="UserMacros" />[m
   <PropertyGroup Condition="'$(Configuration)|$(Platform)'=='Debug|x64'">[m
     <LinkIncremental>true</LinkIncremental>[m
[31m-    <OutDir>..\..\$(Platform).$(Configuration)\</OutDir>[m
[32m+[m[32m    <OutDir>$(SolutionDir)\bin\</OutDir>[m
     <IntDir>..\..\obj\$(Platform).$(Configuration)\$(ProjectName)\</IntDir>[m
   </PropertyGroup>[m
   <PropertyGroup Condition="'$(Configuration)|$(Platform)'=='Release|x64'">[m
[31m-    <OutDir>..\..\$(Platform).$(Configuration)\</OutDir>[m
[32m+[m[32m    <OutDir>$(SolutionDir)\bin\</OutDir>[m
     <IntDir>..\..\obj\$(Platform).$(Configuration)\$(ProjectName)\</IntDir>[m
   </PropertyGroup>[m
   <ItemDefinitionGroup Condition="'$(Configuration)|$(Platform)'=='Debug|x64'">[m
[1mdiff --git a/multithreading/multithreading.vcxproj b/multithreading/multithreading.vcxproj[m
[1mindex d338a2f..b04aa0c 100644[m
[1m--- a/multithreading/multithreading.vcxproj[m
[1m+++ b/multithreading/multithreading.vcxproj[m
[36m@@ -31,11 +31,11 @@[m
   <PropertyGroup Label="UserMacros" />[m
   <PropertyGroup Condition="'$(Configuration)|$(Platform)'=='Debug|x64'">[m
     <LinkIncremental>true</LinkIncremental>[m
[31m-    <OutDir>..\..\$(Platform).$(Configuration)\</OutDir>[m
[32m+[m[32m    <OutDir>$(SolutionDir)\bin\</OutDir>[m
     <IntDir>..\..\obj\$(Platform).$(Configuration)\$(ProjectName)\</IntDir>[m
   </PropertyGroup>[m
   <PropertyGroup Condition="'$(Configuration)|$(Platform)'=='Release|x64'">[m
[31m-    <OutDir>..\..\$(Platform).$(Configuration)\</OutDir>[m
[32m+[m[32m    <OutDir>$(SolutionDir)\bin\</OutDir>[m
     <IntDir>..\..\obj\$(Platform).$(Configuration)\$(ProjectName)\</IntDir>[m
   </PropertyGroup>[m
   <ItemDefinitionGroup Condition="'$(Configuration)|$(Platform)'=='Debug|x64'">[m
[1mdiff --git a/occlusionquery/occlusionquery.vcxproj b/occlusionquery/occlusionquery.vcxproj[m
[1mindex 8c17f64..eab8f17 100644[m
[1m--- a/occlusionquery/occlusionquery.vcxproj[m
[1m+++ b/occlusionquery/occlusionquery.vcxproj[m
[36m@@ -31,11 +31,11 @@[m
   <PropertyGroup Label="UserMacros" />[m
   <PropertyGroup Condition="'$(Configuration)|$(Platform)'=='Debug|x64'">[m
     <LinkIncremental>true</LinkIncremental>[m
[31m-    <OutDir>..\..\$(Platform).$(Configuration)\</OutDir>[m
[32m+[m[32m    <OutDir>$(SolutionDir)\bin\</OutDir>[m
     <IntDir>..\..\obj\$(Platform).$(Configuration)\$(ProjectName)\</IntDir>[m
   </PropertyGroup>[m
   <PropertyGroup Condition="'$(Configuration)|$(Platform)'=='Release|x64'">[m
[31m-    <OutDir>..\..\$(Platform).$(Configuration)\</OutDir>[m
[32m+[m[32m    <OutDir>$(SolutionDir)\bin\</OutDir>[m
     <IntDir>..\..\obj\$(Platform).$(Configuration)\$(ProjectName)\</IntDir>[m
   </PropertyGroup>[m
   <ItemDefinitionGroup Condition="'$(Configuration)|$(Platform)'=='Debug|x64'">[m
[1mdiff --git a/offscreen/offscreen.vcxproj b/offscreen/offscreen.vcxproj[m
[1mindex 53a0692..ff1142e 100644[m
[1m--- a/offscreen/offscreen.vcxproj[m
[1m+++ b/offscreen/offscreen.vcxproj[m
[36m@@ -38,11 +38,11 @@[m
   <PropertyGroup Label="UserMacros" />[m
   <PropertyGroup Condition="'$(Configuration)|$(Platform)'=='Debug|x64'">[m
     <LinkIncremental>true</LinkIncremental>[m
[31m-    <OutDir>..\..\$(Platform).$(Configuration)\</OutDir>[m
[32m+[m[32m    <OutDir>$(SolutionDir)\bin\</OutDir>[m
     <IntDir>..\..\obj\$(Platform).$(Configuration)\$(ProjectName)\</IntDir>[m
   </PropertyGroup>[m
   <PropertyGroup Condition="'$(Configuration)|$(Platform)'=='Release|x64'">[m
[31m-    <OutDir>..\..\$(Platform).$(Configuration)\</OutDir>[m
[32m+[m[32m    <OutDir>$(SolutionDir)\bin\</OutDir>[m
     <IntDir>..\..\obj\$(Platform).$(Configuration)\$(ProjectName)\</IntDir>[m
   </PropertyGroup>[m
   <ItemDefinitionGroup Condition="'$(Configuration)|$(Platform)'=='Debug|x64'">[m
[1mdiff --git a/parallaxmapping/parallaxmapping.vcxproj b/parallaxmapping/parallaxmapping.vcxproj[m
[1mindex f7f9cbe..a99090d 100644[m
[1m--- a/parallaxmapping/parallaxmapping.vcxproj[m
[1m+++ b/parallaxmapping/parallaxmapping.vcxproj[m
[36m@@ -31,11 +31,11 @@[m
   <PropertyGroup Label="UserMacros" />[m
   <PropertyGroup Condition="'$(Configuration)|$(Platform)'=='Debug|x64'">[m
     <LinkIncremental>true</LinkIncremental>[m
[31m-    <OutDir>..\..\$(Platform).$(Configuration)\</OutDir>[m
[32m+[m[32m    <OutDir>$(SolutionDir)\bin\</OutDir>[m
     <IntDir>..\..\obj\$(Platform).$(Configuration)\$(ProjectName)\</IntDir>[m
   </PropertyGroup>[m
   <PropertyGroup Condition="'$(Configuration)|$(Platform)'=='Release|x64'">[m
[31m-    <OutDir>..\..\$(Platform).$(Configuration)\</OutDir>[m
[32m+[m[32m    <OutDir>$(SolutionDir)\bin\</OutDir>[m
     <IntDir>..\..\obj\$(Platform).$(Configuration)\$(ProjectName)\</IntDir>[m
   </PropertyGroup>[m
   <ItemDefinitionGroup Condition="'$(Configuration)|$(Platform)'=='Debug|x64'">[m
[1mdiff --git a/particlefire/particlefire.vcxproj b/particlefire/particlefire.vcxproj[m
[1mindex 87b4c5c..6a79c26 100644[m
[1m--- a/particlefire/particlefire.vcxproj[m
[1m+++ b/particlefire/particlefire.vcxproj[m
[36m@@ -31,11 +31,11 @@[m
   <PropertyGroup Label="UserMacros" />[m
   <PropertyGroup Condition="'$(Configuration)|$(Platform)'=='Debug|x64'">[m
     <LinkIncremental>true</LinkIncremental>[m
[31m-    <OutDir>..\..\$(Platform).$(Configuration)\</OutDir>[m
[32m+[m[32m    <OutDir>$(SolutionDir)\bin\</OutDir>[m
     <IntDir>..\..\obj\$(Platform).$(Configuration)\$(ProjectName)\</IntDir>[m
   </PropertyGroup>[m
   <PropertyGroup Condition="'$(Configuration)|$(Platform)'=='Release|x64'">[m
[31m-    <OutDir>..\..\$(Platform).$(Configuration)\</OutDir>[m
[32m+[m[32m    <OutDir>$(SolutionDir)\bin\</OutDir>[m
     <IntDir>..\..\obj\$(Platform).$(Configuration)\$(ProjectName)\</IntDir>[m
   </PropertyGroup>[m
   <ItemDefinitionGroup Condition="'$(Configuration)|$(Platform)'=='Debug|x64'">[m
[1mdiff --git a/pipelines/pipelines.vcxproj b/pipelines/pipelines.vcxproj[m
[1mindex fe864d4..bdf0686 100644[m
[1m--- a/pipelines/pipelines.vcxproj[m
[1m+++ b/pipelines/pipelines.vcxproj[m
[36m@@ -38,11 +38,11 @@[m
   <PropertyGroup Label="UserMacros" />[m
   <PropertyGroup Condition="'$(Configuration)|$(Platform)'=='Debug|x64'">[m
     <LinkIncremental>true</LinkIncremental>[m
[31m-    <OutDir>..\..\$(Platform).$(Configuration)\</OutDir>[m
[32m+[m[32m    <OutDir>$(SolutionDir)\bin\</OutDir>[m
     <IntDir>..\..\obj\$(Platform).$(Configuration)\$(ProjectName)\</IntDir>[m
   </PropertyGroup>[m
   <PropertyGroup Condition="'$(Configuration)|$(Platform)'=='Release|x64'">[m
[31m-    <OutDir>..\..\$(Platform).$(Configuration)\</OutDir>[m
[32m+[m[32m    <OutDir>$(SolutionDir)\bin\</OutDir>[m
     <IntDir>..\..\obj\$(Platform).$(Configuration)\$(ProjectName)\</IntDir>[m
   </PropertyGroup>[m
   <ItemDefinitionGroup Condition="'$(Configuration)|$(Platform)'=='Debug|x64'">[m
[1mdiff --git a/pushconstants/pushconstants.vcxproj b/pushconstants/pushconstants.vcxproj[m
[1mindex 2360378..94d141a 100644[m
[1m--- a/pushconstants/pushconstants.vcxproj[m
[1m+++ b/pushconstants/pushconstants.vcxproj[m
[36m@@ -31,11 +31,11 @@[m
   <PropertyGroup Label="UserMacros" />[m
   <PropertyGroup Condition="'$(Configuration)|$(Platform)'=='Debug|x64'">[m
     <LinkIncremental>true</LinkIncremental>[m
[31m-    <OutDir>..\..\$(Platform).$(Configuration)\</OutDir>[m
[32m+[m[32m    <OutDir>$(SolutionDir)\bin\</OutDir>[m
     <IntDir>..\..\obj\$(Platform).$(Configuration)\$(ProjectName)\</IntDir>[m
   </PropertyGroup>[m
   <PropertyGroup Condition="'$(Configuration)|$(Platform)'=='Release|x64'">[m
[31m-    <OutDir>..\..\$(Platform).$(Configuration)\</OutDir>[m
[32m+[m[32m    <OutDir>$(SolutionDir)\bin\</OutDir>[m
     <IntDir>..\..\obj\$(Platform).$(Configuration)\$(ProjectName)\</IntDir>[m
   </PropertyGroup>[m
   <ItemDefinitionGroup Condition="'$(Configuration)|$(Platform)'=='Debug|x64'">[m
[1mdiff --git a/radialblur/radialblur.vcxproj b/radialblur/radialblur.vcxproj[m
[1mindex e542d06..f150899 100644[m
[1m--- a/radialblur/radialblur.vcxproj[m
[1m+++ b/radialblur/radialblur.vcxproj[m
[36m@@ -38,11 +38,11 @@[m
   <PropertyGroup Label="UserMacros" />[m
   <PropertyGroup Condition="'$(Configuration)|$(Platform)'=='Debug|x64'">[m
     <LinkIncremental>true</LinkIncremental>[m
[31m-    <OutDir>..\..\$(Platform).$(Configuration)\</OutDir>[m
[32m+[m[32m    <OutDir>$(SolutionDir)\bin\</OutDir>[m
     <IntDir>..\..\obj\$(Platform).$(Configuration)\$(ProjectName)\</IntDir>[m
   </PropertyGroup>[m
   <PropertyGroup Condition="'$(Configuration)|$(Platform)'=='Release|x64'">[m
[31m-    <OutDir>..\..\$(Platform).$(Configuration)\</OutDir>[m
[32m+[m[32m    <OutDir>$(SolutionDir)\bin\</OutDir>[m
     <IntDir>..\..\obj\$(Platform).$(Configuration)\$(ProjectName)\</IntDir>[m
   </PropertyGroup>[m
   <ItemDefinitionGroup Condition="'$(Configuration)|$(Platform)'=='Debug|x64'">[m
[1mdiff --git a/shadowmapping/shadowmapping.vcxproj b/shadowmapping/shadowmapping.vcxproj[m
[1mindex d2405f9..3e43a1e 100644[m
[1m--- a/shadowmapping/shadowmapping.vcxproj[m
[1m+++ b/shadowmapping/shadowmapping.vcxproj[m
[36m@@ -38,11 +38,11 @@[m
   <PropertyGroup Label="UserMacros" />[m
   <PropertyGroup Condition="'$(Configuration)|$(Platform)'=='Debug|x64'">[m
     <LinkIncremental>true</LinkIncremental>[m
[31m-    <OutDir>..\..\$(Platform).$(Configuration)\</OutDir>[m
[32m+[m[32m    <OutDir>$(SolutionDir)\bin\</OutDir>[m
     <IntDir>..\..\obj\$(Platform).$(Configuration)\$(ProjectName)\</IntDir>[m
   </PropertyGroup>[m
   <PropertyGroup Condition="'$(Configuration)|$(Platform)'=='Release|x64'">[m
[31m-    <OutDir>..\..\$(Platform).$(Configuration)\</OutDir>[m
[32m+[m[32m    <OutDir>$(SolutionDir)\bin\</OutDir>[m
     <IntDir>..\..\obj\$(Platform).$(Configuration)\$(ProjectName)\</IntDir>[m
   </PropertyGroup>[m
   <ItemDefinitionGroup Condition="'$(Configuration)|$(Platform)'=='Debug|x64'">[m
[1mdiff --git a/shadowmappingomni/shadowmappingomni.vcxproj b/shadowmappingomni/shadowmappingomni.vcxproj[m
[1mindex fbf76b8..938c18e 100644[m
[1m--- a/shadowmappingomni/shadowmappingomni.vcxproj[m
[1m+++ b/shadowmappingomni/shadowmappingomni.vcxproj[m
[36m@@ -38,11 +38,11 @@[m
   <PropertyGroup Label="UserMacros" />[m
   <PropertyGroup Condition="'$(Configuration)|$(Platform)'=='Debug|x64'">[m
     <LinkIncremental>true</LinkIncremental>[m
[31m-    <OutDir>..\..\$(Platform).$(Configuration)\</OutDir>[m
[32m+[m[32m    <OutDir>$(SolutionDir)\bin\</OutDir>[m
     <IntDir>..\..\obj\$(Platform).$(Configuration)\$(ProjectName)\</IntDir>[m
   </PropertyGroup>[m
   <PropertyGroup Condition="'$(Configuration)|$(Platform)'=='Release|x64'">[m
[31m-    <OutDir>..\..\$(Platform).$(Configuration)\</OutDir>[m
[32m+[m[32m    <OutDir>$(SolutionDir)\bin\</OutDir>[m
     <IntDir>..\..\obj\$(Platform).$(Configuration)\$(ProjectName)\</IntDir>[m
   </PropertyGroup>[m
   <ItemDefinitionGroup Condition="'$(Configuration)|$(Platform)'=='Debug|x64'">[m
[1mdiff --git a/skeletalanimation/skeletalanimation.vcxproj b/skeletalanimation/skeletalanimation.vcxproj[m
[1mindex 3b4c9c9..38fd870 100644[m
[1m--- a/skeletalanimation/skeletalanimation.vcxproj[m
[1m+++ b/skeletalanimation/skeletalanimation.vcxproj[m
[36m@@ -31,11 +31,11 @@[m
   <PropertyGroup Label="UserMacros" />[m
   <PropertyGroup Condition="'$(Configuration)|$(Platform)'=='Debug|x64'">[m
     <LinkIncremental>true</LinkIncremental>[m
[31m-    <OutDir>..\..\$(Platform).$(Configuration)\</OutDir>[m
[32m+[m[32m    <OutDir>$(SolutionDir)\bin\</OutDir>[m
     <IntDir>..\..\obj\$(Platform).$(Configuration)\$(ProjectName)\</IntDir>[m
   </PropertyGroup>[m
   <PropertyGroup Condition="'$(Configuration)|$(Platform)'=='Release|x64'">[m
[31m-    <OutDir>..\..\$(Platform).$(Configuration)\</OutDir>[m
[32m+[m[32m    <OutDir>$(SolutionDir)\bin\</OutDir>[m
     <IntDir>..\..\obj\$(Platform).$(Configuration)\$(ProjectName)\</IntDir>[m
   </PropertyGroup>[m
   <ItemDefinitionGroup Condition="'$(Configuration)|$(Platform)'=='Debug|x64'">[m
[1mdiff --git a/sphericalenvmapping/sphericalenvmapping.vcxproj b/sphericalenvmapping/sphericalenvmapping.vcxproj[m
[1mindex 7965b47..f1cc82d 100644[m
[1m--- a/sphericalenvmapping/sphericalenvmapping.vcxproj[m
[1m+++ b/sphericalenvmapping/sphericalenvmapping.vcxproj[m
[36m@@ -31,11 +31,11 @@[m
   <PropertyGroup Label="UserMacros" />[m
   <PropertyGroup Condition="'$(Configuration)|$(Platform)'=='Debug|x64'">[m
     <LinkIncremental>true</LinkIncremental>[m
[31m-    <OutDir>..\..\$(Platform).$(Configuration)\</OutDir>[m
[32m+[m[32m    <OutDir>$(SolutionDir)\bin\</OutDir>[m
     <IntDir>..\..\obj\$(Platform).$(Configuration)\$(ProjectName)\</IntDir>[m
   </PropertyGroup>[m
   <PropertyGroup Condition="'$(Configuration)|$(Platform)'=='Release|x64'">[m
[31m-    <OutDir>..\..\$(Platform).$(Configuration)\</OutDir>[m
[32m+[m[32m    <OutDir>$(SolutionDir)\bin\</OutDir>[m
     <IntDir>..\..\obj\$(Platform).$(Configuration)\$(ProjectName)\</IntDir>[m
   </PropertyGroup>[m
   <ItemDefinitionGroup Condition="'$(Configuration)|$(Platform)'=='Debug|x64'">[m
[1mdiff --git a/tessellation/tessellation.vcxproj b/tessellation/tessellation.vcxproj[m
[1mindex ee6c4b9..1bf3ab0 100644[m
[1m--- a/tessellation/tessellation.vcxproj[m
[1m+++ b/tessellation/tessellation.vcxproj[m
[36m@@ -38,11 +38,11 @@[m
   <PropertyGroup Label="UserMacros" />[m
   <PropertyGroup Condition="'$(Configuration)|$(Platform)'=='Debug|x6