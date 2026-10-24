#pragma once

#include <cstdint>
#include <stdexcept>
#include <string_view>

enum class VideoCodec { H264, AV1 };
inline const char* CodecName(VideoCodec codec) { return codec == VideoCodec::AV1 ? "AV1" : "H.264"; }

struct VideoResolution { uint32_t width, height, bitrate; };
inline VideoResolution ResolutionPreset(std::string_view name) {
    if (name == "2k") return {2560, 1440, 16000000};
    if (name == "4k") return {3840, 2160, 40000000};
    if (name == "8k") return {7680, 4320, 100000000};
    throw std::invalid_argument("--resolution expects 2k, 4k, or 8k");
}

inline bool ValidVideoSettings(uint32_t width, uint32_t height, uint32_t fps, uint32_t bitrate) {
    return width && height && width <= 7680 && height <= 4320 && width % 2 == 0 && height % 2 == 0 &&
        fps && fps <= 60 && bitrate >= 1000 && bitrate <= 100000000;
}

// Baseline H.264 levels must cover both frame size and macroblocks per second.
inline uint32_t H264Level(uint32_t width, uint32_t height, uint32_t fps, uint32_t bitrate = 0) {
    const uint64_t columns = (uint64_t{width} + 15) / 16, rows = (uint64_t{height} + 15) / 16;
    const uint64_t blocks = ((uint64_t{width} + 15) / 16) * ((uint64_t{height} + 15) / 16);
    if (columns <= 169 && rows <= 169 && blocks <= 3600 && blocks * fps <= 108000 && bitrate <= 14000000) return 31;
    if (blocks <= 36864 && blocks * fps <= 983040) return 51;
    if (blocks <= 36864 && blocks * fps <= 2073600) return 52;
    return 61;
}
