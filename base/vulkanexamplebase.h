/*
* Vulkan Example base class
*
* Copyright (C) 2016 by Sascha Willems - www.saschawillems.de
*
* This code is licensed under the MIT license (MIT) (http://opensource.org/licenses/MIT)
*/

#pragma once

#ifdef _WIN32
#pragma comment(linker, "/subsystem:windows")
#include <windows.h>
#include <fcntl.h>
#include <io.h>
#elif defined(__ANDROID__)
#include <android/native_activity.h>
#include <android/asset_manager.h>
#include <android_native_app_glue.h>
#include "vulkanandroid.h"
#elif defined(__linux__)
#include <xcb/xcb.h>
#endif

#include <iostream>
#include <chrono>

#include <glm/glm.hpp>
#include <string>
#include <array>

#include <vulkan/vulkan.h>
#include <GLFW/glfw3.h>

#include "keycodes.hpp"
#include "vulkantools.h"
#include "vulkandebug.h"

#include "vulkandevice.hpp"
#include "vulkanswapchain.hpp"
#include "vulkanTextureLoader.hpp"
#include "vulkanMeshLoader.hpp"
#include "vulkantextoverlay.hpp"
#include "camera.hpp"

// Function pointer for getting physical device fetures to be enabled
typedef VkPhysicalDeviceFeatures (*PFN_GetEnabledFeatures)();

class VulkanExampleBase
{
private:	
	// Set to true when example is created with enabled validation layers
	bool enableValidation = false;
	// Set to true if v-sync will be forced for the swapchain
	bool enableVSync = false;
	// Device features enabled by the example
	// If not set, no additional features are enabled (may result in validation layer errors)
	VkPhysicalDeviceFeatures enabledFeatures = {};
	// fps timer (one second interval)
	float fpsTimer = 0.0f;
	// Create application wide Vulkan instance
	VkResult createInstance(bool enableValidation);
	// Get window title with example name, device, et.
	std::string getWindowTitle();
	/** brief Indicates that the view (position, rotation) has changed and */
	bool viewUpdated = false;
	// Destination dimensions for resizing the window
	uint32_t destWidth;
	uint32_t destHeight;
	bool resizing = false;
	// Called if the window is resized and some resources have to be recreatesd
	void windowResize(const glm::uvec2& size);
protected:
	// Last frame time, measured using a high performance timer (if available)
	float frameTimer = 1.0f;
	// Frame counter to display fps
	uint32_t frameCounter = 0;
	uint32_t lastFPS = 0;
	// Vulkan instance, stores all per-application states
	VkInstance instance;
	// Physical device (GPU) that Vulkan will ise
	VkPhysicalDevice physicalDevice;
	// Stores physical device properties (for e.g. checking device limits)
	VkPhysicalDeviceProperties deviceProperties;
	// Stores phyiscal device features (for e.g. checking if a feature is available)
	VkPhysicalDeviceFeatures deviceFeatures;
	// Stores all available memory (type) properties for the physical device
	VkPhysicalDeviceMemoryProperties deviceMemoryProperties;
	/** @brief Logical device, application's view of the physical device (GPU) */
	// todo: getter? should always point to VulkanDevice->device
	VkDevice device;
	/** @brief Encapsulated physical and logical vulkan device */
	vk::VulkanDevice *vulkanDevice;
	// Handle to the device graphics queue that command buffers are submitted to
	VkQueue queue;
	// Color buffer format
	VkFormat colorformat = VK_FORMAT_B8G8R8A8_UNORM;
	// Depth buffer format
	// Depth format is selected during Vulkan initialization
	VkFormat depthFormat;
	// Command buffer pool
	VkCommandPool cmdPool;
	// Command buffer used for setup
	VkCommandBuffer setupCmdBuffer = VK_NULL_HANDLE;
	/** @brief Pipeline stages used to wait at for graphics queue submissions */
	VkPipelineStageFlags submitPipelineStages = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
	// Contains command buffers and semaphores to be presented to the queue
	VkSubmitInfo submitInfo;
	// Command buffers used for rendering
	std::vector<VkCommandBuffer> drawCmdBuffers;
	// Global render pass for frame buffer writes
	VkRenderPass renderPass;
	// List of available frame buffers (same as number of swap chain images)
	std::vector<VkFramebuffer>frameBuffers;
	// Active frame buffer index
	uint32_t currentBuffer = 0;
	// Descriptor set pool
	VkDescriptorPool descriptorPool = VK_NULL_HANDLE;
	// List of shader modules created (stored for cleanup)
	std::vector<VkShaderModule> shaderModules;
	// Pipeline cache object
	VkPipelineCache pipelineCache;
	// Wraps the swap chain to present images (framebuffers) to the windowing system
	VulkanSwapChain swapChain;
	// Synchronization semaphores
	struct {
		// Swap chain image presentation
		VkSemaphore presentComplete;
		// Command buffer submission and execution
		VkSemaphore renderComplete;
		// Text overlay submission and execution
		VkSemaphore textOverlayComplete;
	} semaphores;
	// Simple texture loader
	vkTools::VulkanTextureLoader *textureLoader = nullptr;
	// Returns the base asset path (for shaders, models, textures) depending on the os
	const std::string getAssetPath();
public: 
	bool prepared = false;
	uint32_t width = 1280;
	uint32_t height = 720;

	VkClearColorValue defaultClearColor = { { 0.025f, 0.025f, 0.025f, 1.0f } };

	float zoom = 0;

	static std::vector<const char*> args;

	// Defines a frame rate independent timer value clamped from -1.0...1.0
	// For use in animations, rotations, etc.
	float timer = 0.0f;
	// Multiplier for speeding up (or slowing down) the global timer
	float timerSpeed = 0.25f;
	
	bool paused = false;

	bool enableTextOverlay = false;
	VulkanTextOverlay *textOverlay;

	// Use to adjust mouse rotation speed
	float rotationSpeed = 1.0f;
	// Use to adjust mouse zoom speed
	float zoomSpeed = 1.0f;

	Camera camera;

	glm::vec3 rotation = glm::vec3();
	glm::vec3 cameraPos = glm::vec3();
	glm::vec2 mousePos;

	std::string title = "Vulkan Example";
	std::string name = "vulkanExample";

	struct 
	{
		VkImage image;
		VkDeviceMemory mem;
		VkImageView view;
	} depthStencil;

	// Gamepad state (only one pad supported)
	struct
	{
		glm::vec2 axisLeft = glm::vec2(0.0f);
		glm::vec2 axisRight = glm::vec2(0.0f);
	} gamePadState;

	// OS specific 
#if defined(__ANDROID__)
	android_app* androidApp;
	// true if application has focused, false if moved to background
	bool focused = false;
#else
    GLFWwindow* window{ nullptr };
#endif

	// Default ctor
	VulkanExampleBase(bool enableValidation, PFN_GetEnabledFeatures enabledFeaturesFn = nullptr);

	// dtor
	~VulkanExampleBase();

	// Setup the vulkan instance, enable required extensions and connect to the physical device (GPU)
	void initVulkan(bool enableValidation);

#if defined(__ANDROID__)
	static int32_t handleAppInput(struct android_app* app, AInputEvent* event);
	static void handleAppCommand(android_app* app, int32_t cmd);
#else
	void setupWindow();
#endif

    // Pure virtual render function (override in derived class)
	virtual void render() = 0;
	// Called when view change occurs
	// Can be overriden in derived class to e.g. update uniform buffers 
	// Containing view dependant matrices
	virtual void viewChanged();
	// Called if a key is pressed
	// Can be overriden in derived class to do custom key handling
	virtual void keyPressed(uint32_t keyCode);
	virtual void keyReleased(uint32_t keyCode);
    virtual void mouseMoved(const glm::vec2& newPos);
    virtual void mouseScrolled(float delta);

    // Called when the window has been resized
	// Can be overriden in derived class to recreate or rebuild resources attached to the frame buffer / swapchain
	virtual void windowResized();
	// Pure virtual function to be overriden by the dervice class
	// Called in case of an event where e.g. the framebuffer has to be rebuild and thus
	// all command buffers that may reference this
	virtual void buildCommandBuffers();

	// Creates a new (graphics) command pool object storing command buffers
	void createCommandPool();
	// Setup default depth and stencil views
	virtual void setupDepthStencil();
	// Create framebuffers for all requested swap chain images
	// Can be overriden in derived class to setup a custom framebuffer (e.g. for MSAA)
	virtual void setupFrameBuffer();
	// Setup a default render pass
	// Can be overriden in derived class to setup a custom render pass (e.g. for MSAA)
	virtual void setupRenderPass();

	// Connect and prepare the swap chain
	void initSwapchain();
	// Create swap chain images
	void setupSwapChain();

	// Check if command buffers are valid (!= VK_NULL_HANDLE)
	bool checkCommandBuffers();
	// Create command buffers for drawing commands
	void createCommandBuffers();
	// Destroy all command buffers and set their handles to VK_NULL_HANDLE
	// May be necessary during runtime if options are toggled 
	void destroyCommandBuffers();
	// Create command buffer for setup commands
	void createSetupCommandBuffer();
	// Finalize setup command bufferm submit it to the queue and remove it
	void flushSetupCommandBuffer();

	// Command buffer creation
	// Creates and returns a new command buffer
	VkCommandBuffer createCommandBuffer(VkCommandBufferLevel level, bool begin);
	// End the command buffer, submit it to the queue and free (if requested)
	// Note : Waits for the queue to become idle
	void flushCommandBuffer(VkCommandBuffer commandBuffer, VkQueue queue, bool free);

	// Create a cache pool for rendering pipelines
	void createPipelineCache();

	// Prepare commonly used Vulkan functions
	virtual void prepare();

	// Load a SPIR-V shader
	VkPipelineShaderStageCreateInfo loadShader(std::string fileName, VkShaderStageFlagBits stage);
	
	// Create a buffer, fill it with data (if != NULL) and bind buffer memory
	VkBool32 createBuffer(
		VkBufferUsageFlags usageFlags,
		VkMemoryPropertyFlags memoryPropertyFlags,
		VkDeviceSize size,
		void *data,
		VkBuffer *buffer,
		VkDeviceMemory *memory);
	// This version always uses HOST_VISIBLE memory
	VkBool32 createBuffer(
		VkBufferUsageFlags usage,
		VkDeviceSize size,
		void *data,
		VkBuffer *buffer,
		VkDeviceMemory *memory);
	// Overload that assigns buffer info to descriptor
	VkBool32 createBuffer(
		VkBufferUsageFlags usage,
		VkDeviceSize size,
		void *data,
		VkBuffer *buffer,
		VkDeviceMemory *memory,
		VkDescriptorBufferInfo *descriptor);
	// Overload to pass memory property flags
	VkBool32 createBuffer(
		VkBufferUsageFlags usage,
		VkMemoryPropertyFlags memoryPropertyFlags,
		VkDeviceSize size,
		void *data,
		VkBuffer *buffer,
		VkDeviceMemory *memory,
		VkDescriptorBufferInfo *descriptor);

	// Load a mesh (using ASSIMP) and create vulkan vertex and index buffers with given vertex layout
	void loadMesh(
		std::string fiename, 
		vkMeshLoader::MeshBuffer *meshBuffer, 
		std::vector<vkMeshLoader::VertexLayout> vertexLayout, 
		float scale);
	void loadMesh(
		std::string filename, 
		vkMeshLoader::MeshBuffer *meshBuffer, 
		std::vector<vkMeshLoader::VertexLayout> 
		vertexLayout, 
		vkMeshLoader::MeshCreateInfo *meshCreateInfo);

	// Start the main render loop
	void renderLoop();

	void updateTextOverlay();

	// Called when the text overlay is updating
	// Can be overriden in derived class to add custom text to the overlay
	virtual void getOverlayText(VulkanTextOverlay * textOverlay);

	// Prepare the frame for workload submission
	// - Acquires the next image from the swap chain 
	// - Sets the default wait and signal semaphores
	void prepareFrame();

	// Submit the frames' workload 
	// - Submits the text overlay (if enabled)
	void submitFrame();

#if defined(__ANDROID__)

#else
    static void KeyboardHandler(GLFWwindow* window, int key, int scancode, int action, int mods);
    static void MouseHandler(GLFWwindow* window, int button, int action, int mods);
    static void MouseMoveHandler(GLFWwindow* window, double posx, double posy);
    static void MouseScrollHandler(GLFWwindow* window, double xoffset, double yoffset);
    static void FramebufferSizeHandler(GLFWwindow* window, int width, int height);
    static void CloseHandler(GLFWwindow* window);
#endif
};

// OS specific macros for the example main entry points
#if defined(__ANDROID__)
// Android entry point
// A note on app_dummy(): This is required as the compiler may otherwise remove the main entry point of the application
#define VULKAN_EXAMPLE_MAIN()																		\
VulkanExample *vulkanExample;																		\
void android_main(android_app* state)																\
{																									\
	app_dummy();																					\
	vulkanExample = new VulkanExample();															\
	state->userData = vulkanExample;																\
	state->onAppCmd = VulkanExample::handleAppCommand;												\
	state->onInputEvent = VulkanExample::handleAppInput;											\
	vulkanExample->androidApp = state;																\
	vulkanExample->renderLoop();																	\
	delete(vulkanExample);																			\
}

#elif defined(_DIRECT2DISPLAY)
// Linux entry point with direct to display wsi
// todo: extract command line arguments
#define VULKAN_EXAMPLE_MAIN()																		\
VulkanExample *vulkanExample;																		\
static void handleEvent()                                											\
{																									\
}																									\
int main(const int argc, const char *argv[])													    \
{																									\
	vulkanExample = new VulkanExample();															\
	vulkanExample->initSwapchain();																	\
	vulkanExample->prepare();																		\
	vulkanExample->renderLoop();																	\
	delete(vulkanExample);																			\
	return 0;																						\
}
#elif defined(__linux__)
// Linux entry point
// todo: extract command line arguments
#else

#ifdef WIN32
#define ENTRY_POINT int CALLBACK WinMain( HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow)
#define CAPTURE_ARGS for (size_t i = 0; i < __argc; i++) { VulkanExample::args.push_back(__argv[i]); };
#pragma warning(disable: 4267)
#pragma warning(disable: 4244)
#pragma warning(disable: 4018)
// double to float truncation
#pragma warning(disable: 4305)
#else
#define ENTRY_POINT int main(int argc, const char** argv)
#define CAPTURE_ARGS for (size_t i = 0; i < argc; i++) { VulkanExample::args.push_back(argv[i]); };
#endif

// Desktop entry point
#define VULKAN_EXAMPLE_MAIN()	            \
VulkanExample *vulkanExample;				\
ENTRY_POINT									\
{											\
	CAPTURE_ARGS                            \
	vulkanExample = new VulkanExample();	\
	vulkanExample->setupWindow();			\
	vulkanExample->initSwapchain();			\
	vulkanExample->prepare();				\
	vulkanExample->renderLoop();			\
	delete(vulkanExample);					\
	return 0;								\
}																									
#endif

