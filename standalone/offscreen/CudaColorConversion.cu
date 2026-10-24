#include "CudaColorConversion.h"
#include "ColorConversion.h"

template<bool Image> __global__ void Convert(const uchar4* rgba, cudaSurfaceObject_t surface,
    uint32_t width, uint32_t height, uint8_t* nv12, size_t pitch) {
    const uint32_t x = (blockIdx.x * blockDim.x + threadIdx.x) * 2;
    const uint32_t y = (blockIdx.y * blockDim.y + threadIdx.y) * 2;
    if (x >= width || y >= height) return;
    int r = 0, g = 0, b = 0;
    for (uint32_t dy = 0; dy < 2; ++dy) {
        for (uint32_t dx = 0; dx < 2; ++dx) {
            uchar4 pixel;
            if constexpr (Image) pixel = surf2Dread<uchar4>(surface, (x + dx) * sizeof(uchar4), y + dy);
            else pixel = rgba[static_cast<size_t>(y + dy) * width + x + dx];
            nv12[static_cast<size_t>(y + dy) * pitch + x + dx] = color::RgbToYuv(pixel.x, pixel.y, pixel.z).y;
            r += pixel.x; g += pixel.y; b += pixel.z;
        }
    }
    const auto uv = color::RgbToYuv((r + 2) / 4, (g + 2) / 4, (b + 2) / 4);
    const size_t offset = pitch * height + static_cast<size_t>(y / 2) * pitch + x;
    nv12[offset] = uv.u;
    nv12[offset + 1] = uv.v;
}

cudaError_t LaunchRgbaToNv12(const uint8_t* rgba, cudaSurfaceObject_t image,
    uint32_t width, uint32_t height, uint8_t* nv12, size_t pitch, cudaStream_t stream) {
    const dim3 threads(16, 16);
    const dim3 blocks((width + 31) / 32, (height + 31) / 32);
    if (image) Convert<true><<<blocks, threads, 0, stream>>>(nullptr, image, width, height, nv12, pitch);
    else Convert<false><<<blocks, threads, 0, stream>>>(reinterpret_cast<const uchar4*>(rgba), 0, width, height, nv12, pitch);
    return cudaGetLastError();
}
