#pragma once
#include "IVideoEncoder.h"
#include <memory>

class VulkanMemoryExporter;
enum class EncoderPreference { Automatic, Hardware, Software };
struct EncoderSettings { uint32_t width, height, fps, bitrate; };

// The exporter, when provided, must outlive the returned encoder.
std::unique_ptr<IVideoEncoder> CreateVideoEncoder(const EncoderSettings& settings,
    EncoderPreference preference, VulkanMemoryExporter* exporter = nullptr);
