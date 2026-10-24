#pragma once

#include <cstdint>

#ifdef __CUDACC__
#define COLOR_HD __host__ __device__
#else
#define COLOR_HD
#endif

namespace color {
struct Yuv { uint8_t y, u, v; };
COLOR_HD inline uint8_t ClampByte(int value) {
    return static_cast<uint8_t>(value < 0 ? 0 : (value > 255 ? 255 : value));
}
// BT.709 limited-range YUV, shared by CUDA and the CPU fallback to keep colors consistent.
COLOR_HD inline Yuv RgbToYuv(int r, int g, int b) {
    return {ClampByte(16 + ((748 * r + 2516 * g + 254 * b + 2048) >> 12)),
            ClampByte(128 + ((-412 * r - 1387 * g + 1799 * b + 2048) >> 12)),
            ClampByte(128 + ((1799 * r - 1634 * g - 165 * b + 2048) >> 12))};
}
} // namespace color
#undef COLOR_HD
