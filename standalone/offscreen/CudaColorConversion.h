#pragma once
#include <cuda_runtime.h>
#include <cstdint>

cudaError_t LaunchRgbaToNv12(const uint8_t* rgba, cudaSurfaceObject_t image,
    uint32_t width, uint32_t height, uint8_t* nv12, size_t pitch, cudaStream_t stream);
