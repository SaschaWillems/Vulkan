/*
 * Copyright (C) 2016-2026 by Sascha Willems - www.saschawillems.de
 *
 * This code is licensed under the MIT license (MIT) (http://opensource.org/licenses/MIT)
 */

/*
 * Resource Manager Implementation
 */
#if defined(__OHOS__)
#include "VulkanOHOS.h"
#include <ace/xcomponent/native_interface_xcomponent.h>
#include "uv.h"

// Global NativeWindow pointer for OHOS platform
NativeWindow* nativeWindow = nullptr;

// Global loop pointer for OHOS main thread
uv_loop_t* loop = nullptr;
uv_async_t msgSignal{};

// Initialize static members
std::unique_ptr<ResourceManager> ResourceManager::instance = nullptr;
std::mutex ResourceManager::instanceMutex;

ResourceManager& ResourceManager::getInstance() {
    // Double-checked locking pattern for thread-safe singleton
    if (!instance) {
        std::lock_guard<std::mutex> lock(instanceMutex);
        if (!instance) {
            // Use new directly instead of make_unique to work with private constructor
            instance.reset(new ResourceManager());
            LOGI("ResourceManager singleton instance created");
        }
    }
    return *instance;
}

void ResourceManager::initialize(NativeResourceManager* nativeResourceManager) {
    std::lock_guard<std::mutex> lock(resourceMutex);

    if (nativeResourceManager == nullptr) {
        LOGW("ResourceManager initialized with nullptr");
    }

    nativeResourceMgr = nativeResourceManager;
    LOGI("ResourceManager initialized with NativeResourceManager");
}

NativeResourceManager* ResourceManager::getNativeResourceManager() const {
    std::lock_guard<std::mutex> lock(resourceMutex);
    return nativeResourceMgr;
}

bool ResourceManager::isInitialized() const {
    std::lock_guard<std::mutex> lock(resourceMutex);
    return nativeResourceMgr != nullptr;
}

void ResourceManager::reset() {
    std::lock_guard<std::mutex> lock(resourceMutex);
    nativeResourceMgr = nullptr;
    LOGI("ResourceManager reset");
}

int32_t vks::OHOS::screenDensity;

namespace vks {
    namespace OHOS {
        void setDeviceConfig(int screenDensity) {
            vks::OHOS::screenDensity = screenDensity;
        }
    }
}

#endif
