/*
* Vulkan Example base class
*
* Copyright (C) 2016 by Sascha Willems - www.saschawillems.de
*
* This code is licensed under the MIT license (MIT) (http://opensource.org/licenses/MIT)
*/

#ifndef __VULKANEXAMPLEBASE_H__
#define __VULKANEXAMPLEBASE_H__


#define ENABLE_VALIDATION false

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

#define deg_to_rad(deg) deg * float(M_PI / 180)


class IVulkanFramework
{
public:
	IVulkanFramework(){};
	virtual ~IVulkanFramework(){};

	virtual std::string		getAssetPath	()=0;
	//virtual int32_t		prepare			()=0;	// Prepare commonly used Vulkan functions
	//virtual int32_t		()=0;
};

class CVulkanFramework;

class IVulkanGame
{
public:
	IVulkanGame(){};
	virtual ~IVulkanGame(){};

	//virtual int32_t			init(IVulkanFramework* pFramework)=0; at the moment we don't support the IVulkanFramework interface so just work with the CVulkanFramework class instead.
	virtual int32_t			init(CVulkanFramework* pFramework)=0;
	virtual int32_t			prepare							()=0;	// Prepare commonly used Vulkan functions
	virtual int32_t			render							()=0;
	virtual void			keyPressed		(uint32_t keyCode)=0;
	virtual void			viewChanged						()=0;

};

#define DEFINE_VULKAN_GAME_CREATE_AND_RELEASE_FUNCTIONS()									\
int32_t createVulkanGame(IVulkanGame** ppCreated)											\
{																							\
	VulkanExample* newVulkanExample = new VulkanExample(), *oldVulkanExample=reinterpret_cast<VulkanExample*>(*ppCreated);	\
	*ppCreated = newVulkanExample;															\
	if(oldVulkanExample)																	\
		delete (oldVulkanExample);															\
	return 0;																				\
}																							\
																							\
int32_t releaseVulkanGame(IVulkanGame** ppInstance)		\
{														\
	/*VulkanExample* oldVulkanExample=reinterpret_cast<VulkanExample*>(*ppInstance);*/			\
	IVulkanGame* oldVulkanExample=*ppInstance;			\
	*ppInstance = 0;									\
	if(oldVulkanExample)								\
		delete (oldVulkanExample);						\
	return 0;											\
}														\

class CBaseVulkanGame : public IVulkanGame
{
public:
	CBaseVulkanGame (){};
	virtual ~CBaseVulkanGame (){};

	virtual int32_t			init(CVulkanFramework* pFramework)
	{
		m_pFramework = pFramework;
		return 0;
	};
	virtual int32_t			prepare				()=0;	// Prepare commonly used Vulkan functions
	virtual int32_t			render				()=0;

protected:
	CVulkanFramework*		m_pFramework;	
};
struct SScreenRect
{
	SScreenRect( void ):
		Width	(1280),
		Height	(720)
	{}
	uint32_t Width;
	uint32_t Height;
};

extern "C"
{
	int32_t	createVulkanGame	(IVulkanGame** ppCreated );
	int32_t	releaseVulkanGame	(IVulkanGame** ppInstance);
	typedef int32_t (*CREATEVULKANGAME )(IVulkanGame** ppCreated );
	typedef int32_t (*RELEASEVULKANGAME)(IVulkanGame** ppInstance);
};

struct VulkanDepthStencil
{
	VkImage image;
	VkDeviceMemory mem;
	VkImageView view;
};

class CVulkanFramework : public IVulkanFramework
{
private:	
	float								fpsTimer			= 0;
	bool								enableValidation	= false;			// Set to true when example is created with enabled validation layers
	VkResult							createInstance(bool enableValidation);	// Create application wide Vulkan instance
	VkResult							createDevice(
		VkDeviceQueueCreateInfo requestedQueues, 
		bool enableValidation
	);	// Create logical Vulkan device based on physical device
	std::string							getWindowTitle();
protected:
	float								frameTimer					= 0;	// Last frame time, measured using a high performance timer (if available)
	uint32_t							frameCounter				= 0;	// Frame counter to display fps
	VkInstance							instance					= 0;	// Vulkan instance, stores all per-application states
	VkPhysicalDeviceMemoryProperties	deviceMemoryProperties;				// Stores all available memory (type) properties for the physical device
	VkFormat							colorformat					= VK_FORMAT_B8G8R8A8_UNORM;	// Color buffer format
	VkFormat							depthFormat;						// Depth buffer format - Depth format is selected during Vulkan initialization
	VkCommandBuffer						prePresentCmdBuffer			= VK_NULL_HANDLE;	// Command buffer for submitting a pre present image barrier
	VkPipelineStageFlags				submitPipelineStages		= VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT;	// Pipeline stage flags for the submit info structure
	std::vector<VkShaderModule>			shaderModules;									// List of shader modules created (stored for cleanup)
	VkPhysicalDeviceProperties			deviceProperties;								// Stores physical device properties (for e.g. checking device limits)


	//--------------------------------------------------------------
	IVulkanGame*						m_pVulkanExample			= nullptr;	
public:	// these were made public in order to enable decoupling of the CVulkanFramework class
	VulkanSwapChain						swapChain;										// Wraps the swap chain to present images (framebuffers) to the windowing system
	VkDevice							device						= 0;	// Logical device, application's view of the physical device (GPU)
	VkPipelineCache						pipelineCache				= VK_NULL_HANDLE;	// Pipeline cache object
	VkRenderPass						renderPass					= VK_NULL_HANDLE;	// Global render pass for frame buffer writes
	VkDescriptorPool					descriptorPool				= VK_NULL_HANDLE;	// Descriptor set pool
	VkQueue								queue						= 0;	// Handle to the device graphics queue that command buffers are submitted to
	VkCommandPool						cmdPool						= VK_NULL_HANDLE;	// Command buffer pool
	uint32_t							currentBuffer				= 0;				// Active frame buffer index
	VkSubmitInfo						submitInfo;										// Contains command buffers and semaphores to be presented to the queue
	VkCommandBuffer						setupCmdBuffer				= VK_NULL_HANDLE;	// Command buffer used for setup
	VkCommandBuffer						postPresentCmdBuffer		= VK_NULL_HANDLE;	// Command buffer for submitting a post present image barrier
	std::vector<VkCommandBuffer>		drawCmdBuffers;									// Command buffers used for rendering
	std::vector<VkFramebuffer>			frameBuffers;									// List of available frame buffers (same as number of swap chain images)
	VkPhysicalDevice					physicalDevice				= 0;				// Physical device (GPU) that Vulkan will use
	struct SVulkanSwapChainSemaphores 
	{
		VkSemaphore presentComplete;	// Swap chain image presentation
		VkSemaphore renderComplete;		// Command buffer submission and execution
	} semaphores;																// Synchronization semaphores
	vkTools::VulkanTextureLoader		*textureLoader				= nullptr;			// Simple texture loader


public: 
	std::string							getAssetPath();							// Returns the base asset path (for shaders, models, textures) depending on the os

	SScreenRect				ScreenRect; 
	bool					prepared			= false;
	VkClearColorValue		defaultClearColor	= { { 0.025f, 0.025f, 0.025f, 1.0f } };
	float					zoom				= 0;
	float					timer				= 0.0f;		// Defines a frame rate independent timer value clamped from -1.0...1.0 for use in animations, rotations, etc.
	float					timerSpeed			= 0.25f;	// Multiplier for speeding up (or slowing down) the global timer
	bool					paused				= false;
	float					rotationSpeed		= 1.0f;		// Use to adjust mouse rotation speed
	float					zoomSpeed			= 1.0f;		// Use to adjust mouse zoom speed
	glm::vec3				rotation			= glm::vec3();
	glm::vec2				mousePos;
	std::string				title				= "Vulkan Example";
	std::string				name				= "vulkanExample";
	VulkanDepthStencil		depthStencil;

	// OS specific 
#if defined(_WIN32)
	HWND					window				= 0;
	HINSTANCE				windowInstance		= 0;
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
	} mouseButtons;
	bool quit;
	xcb_connection_t *connection=0;
	xcb_screen_t *screen=0;
	xcb_window_t window;
	xcb_intern_atom_reply_t *atom_wm_delete_window=0;
#endif

	CVulkanFramework(bool enableValidation);
	CVulkanFramework() : CVulkanFramework(ENABLE_VALIDATION) {};
	~CVulkanFramework();

	void initVulkan(bool enableValidation);	// Setup the vulkan instance, enable required extensions and connect to the physical device (GPU)
	
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

	int32_t				render()
	{
		return m_pVulkanExample->render();
	};

	// Called when view change occurs. Can be overriden in derived class to e.g. update uniform buffers containing view dependant matrices
	virtual void		viewChanged();
	virtual void		keyPressed(uint32_t keyCode);	// Called if a key is pressed 

	// Get memory type for a given memory allocation (flags and bits)
	VkBool32			getMemoryType(uint32_t typeBits, VkFlags properties, uint32_t *typeIndex);
	void				createCommandPool();	// Creates a new (graphics) command pool object storing command buffers
	void				setupDepthStencil();	// Setup default depth and stencil views
	void				setupFrameBuffer();		// Create framebuffers for all requested swap chain images
	void				setupRenderPass();		// Setup a default render pass
	void				initSwapchain();		// Connect and prepare the swap chain
	void				setupSwapChain();		// Create swap chain images
	bool				checkCommandBuffers();	// Check if command buffers are valid (!= VK_NULL_HANDLE)
	void				createCommandBuffers();	// Create command buffers for drawing commands

	// Destroy all command buffers and set their handles to VK_NULL_HANDLE. May be necessary during runtime if options are toggled 
	void				destroyCommandBuffers();
	void				createSetupCommandBuffer();	// Create command buffer for setup commands
	void				flushSetupCommandBuffer();	// Finalize setup command bufferm submit it to the queue and remove it
	void				createPipelineCache();		// Create a cache pool for rendering pipelines
	void				renderLoop();				// Start the main render loop
	virtual int32_t		prepare();					// Prepare commonly used Vulkan functions


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

	VkPipelineShaderStageCreateInfo loadShader(std::string fileName, VkShaderStageFlagBits stage);	// Load a SPIR-V shader

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
};

#endif	// __VULKANEXAMPLEBASE_H__
