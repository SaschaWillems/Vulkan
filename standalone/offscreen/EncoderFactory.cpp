#include "EncoderFactory.h"
#ifdef __APPLE__
#include "VideoToolboxEncoder.h"
#else
#include "X264Encoder.h"
#endif
#ifdef OFFSCREEN_HAS_NVENC
#include "NvencZeroCopyEncoder.h"
#endif
#include <iostream>
#include <stdexcept>

std::unique_ptr<IVideoEncoder> CreateVideoEncoder(const EncoderSettings& settings,
    EncoderPreference preference, VulkanMemoryExporter* exporter) {
    const auto initialize = [&](IVideoEncoder& encoder) {
        return encoder.Init(settings.width, settings.height, settings.fps, settings.bitrate);
    };
#ifdef __APPLE__
    (void)exporter;
    if (preference == EncoderPreference::Software)
        throw std::runtime_error("The macOS build uses VideoToolbox; CPU fallback is provided on Linux");
    auto encoder = std::make_unique<VideoToolboxEncoder>();
    if (!initialize(*encoder)) throw std::runtime_error("Cannot initialize VideoToolbox hardware H.264");
    return encoder;
#else
#ifdef OFFSCREEN_HAS_NVENC
    if (preference != EncoderPreference::Software && exporter) {
        auto encoder = std::make_unique<NvencZeroCopyEncoder>(*exporter);
        if (initialize(*encoder)) return encoder;
    }
#else
    (void)exporter;
#endif
    if (preference == EncoderPreference::Hardware)
        throw std::runtime_error("NVENC was required but GPU export, CUDA, or NVENC initialization is unavailable");
    if (preference == EncoderPreference::Automatic)
        std::cerr << "NVENC unavailable; using CPU x264 with Vulkan host readback\n";
    auto encoder = std::make_unique<X264Encoder>();
    if (!initialize(*encoder)) throw std::runtime_error("Cannot initialize x264 fallback");
    return encoder;
#endif
}
