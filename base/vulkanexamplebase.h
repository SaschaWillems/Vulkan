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

#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <glm/glm.hpp>
#include <string>
#include <array>

#include "vulkan/vulkan.h"

#include "vulkantools.h"
#include "vulkandebug.h"

#include "vulkanswapchain.hpp"
#include "vulkanTextureLoader.hpp"
#include "vulkanMeshLoader.hpp"

class VulkanExampleBase
{
private:	
	// Set to true when example is created with enabled validation layers
	bool enableValidation = false;
	// fps timer (one second interval)
	float fpsTimer = 0.0f;
	// Create application wide Vulkan instance
	VkResult createInstance(bool enableValidation);
	// Create logical Vulkan device based on physical device
	VkResult createDevice(VkDeviceQueueCreateInfo requestedQueues, bool enableValidation);
	// Get window title with example name, device, et.
	std::string getWindowTitle();
protected:
	// Last frame time, measured using a high performance timer (if available)
	float frameTimer = 1.0f;
	// Frame counter to display fps
	uint32_t frameCounter = 0;
	// Vulkan instance, stores all per-application states
	VkInstance instance;
	// Physical device (GPU) that Vulkan will ise
	VkPhysicalDevice physicalDevice;
	// Stores physical device properties (for e.g. checking device limits)
	VkPhysicalDeviceProperties deviceProperties;
	// Stores all available memory (type) properties for the physical device
	VkPhysicalDeviceMemoryProperties deviceMemoryProperties;
	// Logical device, application's view of the physical device (GPU)
	VkDevice device;
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
	// Command buffer for submitting a post present image barrier
	VkCommandBuffer postPresentCmdBuffer = VK_NULL_HANDLE;
	// Command buffer for submitting a pre present image barrier
	VkCommandBuffer prePresentCmdBuffer = VK_NULL_HANDLE;
	// Pipeline stage flags for the submit info structure
	VkPipelineStageFlags submitPipelineStages = VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT;
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
	VkDescriptorPool descriptorPool;
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

	// Defines a frame rate independent timer value clamped from -1.0...1.0
	// For use in animations, rotations, etc.
	float timer = 0.0f;
	// Multiplier for speeding up (or slowing down) the global timer
	float timerSpeed = 0.25f;
	
	bool paused = false;

	// Use to adjust mouse rotation speed
	float rotationSpeed = 1.0f;
	// Use to adjust mouse zoom speed
	float zoomSpeed = 1.0f;

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

	// OS specific 
#if defined(_WIN32)
	HWND window;
	HINSTANCE windowInstance;
#elif defined(__ANDROID__)
	android_app* androidApp;
	// true if application has focused, false if moved to background
	bool focused = false;
	// Gamepad state (only one)
	struct
	{
		struct
		{
			float x = 0.0f;
			float y = 0.0f;
			float z = 0.0f;
			float rz = 0.0f;
		} axes;
	} gamePadState;
#elif defined(__linux__)
	struct {
		bool left = false;
		bool right = false;
		bool middle = false;
	} mouseButtons;
	bool quit;
	xcb_connection_t *connection;
	xcb_screen_t *screen;
	xcb_window_t window;
	xcb_intern_atom_reply_t *atom_wm_delete_window;
#endif

	VulkanExampleBase(bool enableValidation);
	VulkanExampleBase() : VulkanExampleBase(false) {};
	~VulkanExampleBase();

	// Setup the vulkan instance, enable required extensions and connect to the physical device (GPU)
	void initVulkan(bool enableValidation);

#if defined(_WIN32)
	void setupConsole(std::string title);
	HWND setupWindow(HINSTANCE hinstance, WNDPROC wndproc);
	void handleMessages(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
#elif defined(__ANDROID__)
	static int32_t handleAppInput(struct android_app* app, AInputEvent* event);
	static void handleAppCommand(android_app* app, int32_t cmd);
#elif defined(__linux__)
	xcb_window_t setupWindow();
	void initxcbConnection();
	void handleEvent(const xcb_generic_event_t *event);
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

	// Get memory type for a given memory allocation (flags and bits)
	VkBool32 getMemoryType(uint32_t typeBits, VkFlags properties, uint32_t *typeIndex);

	// Creates a new (graphics) command pool object storing command buffers
	void createCommandPool();
	// Setup default depth and stencil views
	void setupDepthStencil();
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

	// Create a cache pool for rendering pipelines
	void createPipelineCache();

	// Prepare commonly used Vulkan functions
	virtual void prepare();

	// Load a SPIR-V shader
	VkPipelineShaderStageCreateInfo loadShader(std::string fileName, VkShaderStageFlagBits stage);
	
	// Create a buffer, fill it with data and bind buffer memory
	// Can be used for e.g. vertex or index buffer based on mesh data
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

	// Load a mesh (using ASSIMP) and create vulkan vertex and index buffers with given vertex layout
	void loadMesh(
		std::string fiename,
		vkMeshLoader::MeshBuffer *meshBuffer,
		std::vector<vkMeshLoader::VertexLayout> vertexLayout,
		float scale);

	// Start the main render loop
	void renderLoop();

	// Submit a pre present image barrier to the queue
	// Transforms the (framebuffer) image layout from color attachment to present(khr) for presenting to the swap chain
	void submitPrePresentBarrier(VkImage image);

	// Submit a post present image barrier to the queue
	// Transforms the (framebuffer) image layout back from present(khr) to color attachment layout
	void submitPostPresentBarrier(VkImage image);

	// Prepare a submit info structure containing
	// semaphores and submit buffer info for vkQueueSubmit
	VkSubmitInfo prepareSubmitInfo(
		std::vector<VkCommandBuffer> commandBuffers,
		VkPipelineStageFlags *pipelineStages);
};

