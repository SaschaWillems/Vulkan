/*
 * Copyright (C) 2016-2026 by Sascha Willems - www.saschawillems.de
 *
 * This code is licensed under the MIT license (MIT) (http://opensource.org/licenses/MIT)
 */

#ifndef VULKANEXAMPLES_VULKANOHOS_H
#define VULKANEXAMPLES_VULKANOHOS_H

#if defined(__OHOS__)
#include <memory>
#include <mutex>
#include <hilog/log.h>
#include <rawfile/raw_file_manager.h>

#define APP_LOG_DOMAIN 0x0001
#define APP_LOG_TAG "XComponent_Native"
#define LOGI(...) ((void)OH_LOG_Print(LOG_APP, LOG_INFO, APP_LOG_DOMAIN, APP_LOG_TAG, __VA_ARGS__))
#define LOGD(...) ((void)OH_LOG_Print(LOG_APP, LOG_DEBUG, APP_LOG_DOMAIN, APP_LOG_TAG, __VA_ARGS__))
#define LOGW(...) ((void)OH_LOG_Print(LOG_APP, LOG_WARN, APP_LOG_DOMAIN, APP_LOG_TAG, __VA_ARGS__))
#define LOGE(...) ((void)OH_LOG_Print(LOG_APP, LOG_ERROR, APP_LOG_DOMAIN, APP_LOG_TAG, __VA_ARGS__))

namespace vks 
{
    namespace OHOS
    {
        const int32_t DOUBLE_TAP_TIMEOUT = 300 * 1000000;
        const int32_t TAP_TIMEOUT = 180 * 1000000;
		const int32_t DOUBLE_TAP_SLOP = 100;
		const int32_t TAP_SLOP = 8; 

		/** @brief Density of the device screen (in DPI) */
		extern int32_t screenDensity;

        void setDeviceConfig(int screenDesity);
    }
}

class ResourceManager {
public:
    /**
     * Get the singleton instance of ResourceManager
     * @return Reference to the ResourceManager instance
     */
    static ResourceManager& getInstance();

    /**
     * Initialize the ResourceManager with NativeResourceManager
     * Must be called once during application initialization
     *
     * @param nativeResourceManager Pointer to NativeResourceManager from HarmonyOS
     */
    void initialize(NativeResourceManager* nativeResourceManager);

    /**
     * Get the NativeResourceManager pointer
     * @return NativeResourceManager pointer, nullptr if not initialized
     */
    NativeResourceManager* getNativeResourceManager() const;

    /**
     * Check if ResourceManager has been initialized
     * @return true if initialized, false otherwise
     */
    bool isInitialized() const;

    /**
     * Reset the ResourceManager (mainly for testing purposes)
     */
    void reset();

private:
    // Private constructor for singleton pattern
    ResourceManager() = default;

    // Delete copy constructor and assignment operator
    ResourceManager(const ResourceManager&) = delete;
    ResourceManager& operator=(const ResourceManager&) = delete;

    // Delete move constructor and assignment operator
    ResourceManager(ResourceManager&&) = delete;
    ResourceManager& operator=(ResourceManager&&) = delete;

    // Static instance pointer
    static std::unique_ptr<ResourceManager> instance;

    // Mutex for thread-safe singleton creation
    static std::mutex instanceMutex;

    // NativeResourceManager pointer
    NativeResourceManager* nativeResourceMgr = nullptr;

    // Mutex for thread-safe access to nativeResourceMgr
    mutable std::mutex resourceMutex;
};

#endif

#endif //VULKANEXAMPLES_VULKANOHOS_H
