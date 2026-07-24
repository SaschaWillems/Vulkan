/*
 * Vulkan entry points
 *
 * Platform specific macros for the example main entry points
 * 
 * Copyright (C) 2024-2025 by Sascha Willems - www.saschawillems.de
 *
 * This code is licensed under the MIT license (MIT) (http://opensource.org/licenses/MIT)
 */

#if defined(_WIN32)
/*
 * Windows
 */
#define VULKAN_EXAMPLE_MAIN()																		\
VulkanExample *vulkanExample;																		\
LRESULT CALLBACK WndProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)						\
{																									\
	if (vulkanExample != NULL)																		\
	{																								\
		vulkanExample->handleMessages(hWnd, uMsg, wParam, lParam);									\
	}																								\
	return (DefWindowProc(hWnd, uMsg, wParam, lParam));												\
}																									\
int APIENTRY WinMain(_In_ HINSTANCE hInstance, _In_opt_  HINSTANCE hPrevInstance, _In_ LPSTR, _In_ int) \
{																									\
	for (int32_t i = 0; i < __argc; i++) { VulkanExample::args.push_back(__argv[i]); };  			\
	vulkanExample = new VulkanExample();															\
	vulkanExample->initVulkan();																	\
	vulkanExample->setupWindow(hInstance, WndProc);													\
	vulkanExample->prepare();																		\
	vulkanExample->renderLoop();																	\
	delete(vulkanExample);																			\
	return 0;																						\
}

#elif defined(VK_USE_PLATFORM_ANDROID_KHR)
/*
 * Android
 */
#define VULKAN_EXAMPLE_MAIN()																		\
VulkanExample *vulkanExample;																		\
void android_main(android_app* state)																\
{																									\
	androidApp = state;																				\
	vks::android::getDeviceConfig();																\
	vulkanExample = new VulkanExample();															\
	state->userData = vulkanExample;																\
	state->onAppCmd = VulkanExample::handleAppCommand;												\
	state->onInputEvent = VulkanExample::handleAppInput;											\
	vulkanExample->renderLoop();																	\
	delete(vulkanExample);																			\
}

#elif defined(VK_USE_PLATFORM_OHOS)
/**
 * OHOS
 */
#define VULKAN_EXAMPLE_MAIN()                                                                       \
VulkanExample vulkanExample;                                                                        \
                                                                                                    \
void OnSurfaceCreatedCB(OH_NativeXComponent *component, void *window) {                             \
    LOGD("OnSurfaceCreatedCB");                                                                     \
    uint64_t width, height;                                                                         \
    int32_t ret = OH_NativeXComponent_GetXComponentSize(component, window, &width, &height);        \
                                                                                                    \
    if (!vulkanExample.initVulkan()) {                                                              \
        LOGE("PluginRender::OnSurfaceCreated vulkanExample initVulkan FALSE");                      \
        return;                                                                                     \
    }                                                                                               \
    nativeWindow = (static_cast<OHNativeWindow *>(window));                                         \
    vulkanExample.prepare();                                                                        \
    uv_async_send(&msgSignal);                                                                      \
}                                                                                                   \
                                                                                                    \
void OnSurfaceChangedCB(OH_NativeXComponent *component, void *window) {                             \
    LOGD("OnSurfaceChangedCB");                                                                     \
}                                                                                                   \
                                                                                                    \
void OnSurfaceDestroyedCB(OH_NativeXComponent *component, void *window) {                           \
    LOGD("OnSurfaceDestroyedCB");                                                                   \
}                                                                                                   \
                                                                                                    \
void DispatchTouchEventCB(OH_NativeXComponent *component, void *window) {                           \
    VulkanExample::handleAppInput(component, window, &vulkanExample);                               \
}                                                                                                   \
                                                                                                    \
extern "C" napi_value Init(napi_env env, napi_value exports) {                                          \
    napi_status status;                                                                             \
    napi_value exportInstance = nullptr;                                                            \
    OH_NativeXComponent *nativeXComponent = nullptr;                                                \
    int32_t ret;                                                                                    \
    char idStr[OH_XCOMPONENT_ID_LEN_MAX + 1] = {};                                                  \
    uint64_t idSize = OH_XCOMPONENT_ID_LEN_MAX + 1;                                                 \
    status = napi_get_named_property(env, exports, OH_NATIVE_XCOMPONENT_OBJ, &exportInstance);      \
    if (status != napi_ok) {                                                                        \
        LOGE("Export: napi_get_named_property fail");                                               \
    }                                                                                               \
    status = napi_unwrap(env, exportInstance, reinterpret_cast<void **>(&nativeXComponent));        \
    if (status != napi_ok) {                                                                        \
        LOGE("Export: napi_unwrap failed status[%{public}d]", status);                              \
    }                                                                                               \
    napi_value global;                                                                              \
    status = napi_get_global(env, &global);                                                         \
    status = napi_get_named_property(env, global, "globalThis", &global);                           \
    napi_value densityDPI;                                                                          \
    status = napi_get_named_property(env, global, "densityDPI", &densityDPI);                       \
    double density = 0.0f;                                                                          \
    napi_get_value_double(env, densityDPI, &density);                                               \
    vks::OHOS::setDeviceConfig(density);                                                            \
    napi_value context;                                                                             \
    status = napi_get_named_property(env, global, "context", &context);                             \
    napi_value resourceManager = NULL;                                                              \
    status = napi_get_named_property(env, context, "resourceManager", &resourceManager);            \
    NativeResourceManager *nativeResourceManager = OH_ResourceManager_InitNativeResourceManager(env, resourceManager);  \
    ResourceManager::getInstance().initialize(nativeResourceManager);                               \
    status = napi_get_uv_event_loop(env, &loop);                                                    \
    msgSignal.data = &vulkanExample;                                                                \
                                                                                                    \
    int uv_status = uv_async_init(loop, &msgSignal, VulkanExampleBase::onMessageCallbackStatic);    \
    static OH_NativeXComponent_Callback callback;                                                   \
    callback.OnSurfaceCreated = OnSurfaceCreatedCB;                                                 \
    callback.OnSurfaceChanged = OnSurfaceChangedCB;                                                 \
    callback.OnSurfaceDestroyed = OnSurfaceDestroyedCB;                                             \
    callback.DispatchTouchEvent = DispatchTouchEventCB;                                             \
                                                                                                    \
    OH_NativeXComponent_RegisterCallback(nativeXComponent, &callback);                              \
    return exports;                                                                                 \
}                                                                                                   \
                                                                                                    \
static napi_module demoModule = {                                                                   \
    .nm_version = 1,                                                                                \
    .nm_flags = 0,                                                                                  \
    .nm_filename = nullptr,                                                                         \
    .nm_register_func = Init,                                                                       \
    .nm_modname = "nativerender_triangle",                                                          \
    .nm_priv = ((void *)0),                                                                         \
    .reserved = {0},                                                                                \
};                                                                                                  \
                                                                                                    \
extern "C" __attribute__((constructor)) void RegisterEntryModule(void) { napi_module_register(&demoModule); }

#elif defined(_DIRECT2DISPLAY)
/*
 * Direct-to-display
 */
#define VULKAN_EXAMPLE_MAIN()																		\
VulkanExample *vulkanExample;																		\
static void handleEvent()                                											\
{																									\
}																									\
int main(const int argc, const char *argv[])													    \
{																									\
	for (size_t i = 0; i < argc; i++) { VulkanExample::args.push_back(argv[i]); };  				\
	vulkanExample = new VulkanExample();															\
	vulkanExample->initVulkan();																	\
	vulkanExample->prepare();																		\
	vulkanExample->renderLoop();																	\
	delete(vulkanExample);																			\
	return 0;																						\
}

#elif defined(VK_USE_PLATFORM_DIRECTFB_EXT)
/*
 * Direct FB
 */
#define VULKAN_EXAMPLE_MAIN()																		\
VulkanExample *vulkanExample;																		\
static void handleEvent(const DFBWindowEvent *event)												\
{																									\
	if (vulkanExample != NULL)																		\
	{																								\
		vulkanExample->handleEvent(event);															\
	}																								\
}																									\
int main(const int argc, const char *argv[])													    \
{																									\
	for (size_t i = 0; i < argc; i++) { VulkanExample::args.push_back(argv[i]); };  				\
	vulkanExample = new VulkanExample();															\
	vulkanExample->initVulkan();																	\
	vulkanExample->setupWindow();					 												\
	vulkanExample->prepare();																		\
	vulkanExample->renderLoop();																	\
	delete(vulkanExample);																			\
	return 0;																						\
}

#elif (defined(VK_USE_PLATFORM_WAYLAND_KHR) || defined(VK_USE_PLATFORM_HEADLESS_EXT))
 /*
  * Wayland / headless
  */
#define VULKAN_EXAMPLE_MAIN()																		\
VulkanExample *vulkanExample;																		\
int main(const int argc, const char *argv[])													    \
{																									\
	for (size_t i = 0; i < argc; i++) { VulkanExample::args.push_back(argv[i]); };  				\
	vulkanExample = new VulkanExample();															\
	vulkanExample->initVulkan();																	\
	vulkanExample->setupWindow();					 												\
	vulkanExample->prepare();																		\
	vulkanExample->renderLoop();																	\
	delete(vulkanExample);																			\
	return 0;																						\
}

#elif defined(VK_USE_PLATFORM_XCB_KHR)
/*
 * X11 Xcb
 */
#define VULKAN_EXAMPLE_MAIN()																		\
VulkanExample *vulkanExample;																		\
static void handleEvent(const xcb_generic_event_t *event)											\
{																									\
	if (vulkanExample != NULL)																		\
	{																								\
		vulkanExample->handleEvent(event);															\
	}																								\
}																									\
int main(const int argc, const char *argv[])													    \
{																									\
	for (size_t i = 0; i < argc; i++) { VulkanExample::args.push_back(argv[i]); };  				\
	vulkanExample = new VulkanExample();															\
	vulkanExample->initVulkan();																	\
	vulkanExample->setupWindow();					 												\
	vulkanExample->prepare();																		\
	vulkanExample->renderLoop();																	\
	delete(vulkanExample);																			\
	return 0;																						\
}

#elif (defined(VK_USE_PLATFORM_IOS_MVK) || defined(VK_USE_PLATFORM_MACOS_MVK) || defined(VK_USE_PLATFORM_METAL_EXT))
/*
 * iOS and macOS (using MoltenVK)
 */
#if defined(VK_EXAMPLE_XCODE_GENERATED)
#define VULKAN_EXAMPLE_MAIN()																		\
VulkanExample *vulkanExample;																		\
int main(const int argc, const char *argv[])														\
{																									\
	@autoreleasepool																				\
	{																								\
		for (size_t i = 0; i < argc; i++) { VulkanExample::args.push_back(argv[i]); };				\
		vulkanExample = new VulkanExample();														\
		vulkanExample->initVulkan();																\
		vulkanExample->setupWindow(nullptr);														\
		vulkanExample->prepare();																	\
		vulkanExample->renderLoop();																\
		delete(vulkanExample);																		\
	}																								\
	return 0;																						\
}
#else
#define VULKAN_EXAMPLE_MAIN()
#endif

#elif defined(VK_USE_PLATFORM_SCREEN_QNX)
/*
 * QNX Screen
 */
#define VULKAN_EXAMPLE_MAIN()																		\
VulkanExample *vulkanExample;																		\
int main(const int argc, const char *argv[])														\
{																									\
	for (int i = 0; i < argc; i++) { VulkanExample::args.push_back(argv[i]); };						\
	vulkanExample = new VulkanExample();															\
	vulkanExample->initVulkan();																	\
	vulkanExample->setupWindow();																	\
	vulkanExample->prepare();																		\
	vulkanExample->renderLoop();																	\
	delete(vulkanExample);																			\
	return 0;																						\
}
#endif
