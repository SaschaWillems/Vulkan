#pragma once
#include "IVideoEncoder.h"
#include <memory>

class VulkanMemoryExporter;
enum class EncoderPreference { Automatic, Hardware, Software };
struct EncoderSettings { uint32_t width, height, fps, bitrate; VideoCodec codec = VideoCodec::H264; };

// The exporter, when provided, must outlive the returned encoder.
std::unique_ptr<IVideoEncoder> CreateVideoEncoder(const EncoderSettings& settings,
    EncoderPreference preference, VulkanMemoryExporter* exporter = nullptr);
