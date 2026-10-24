#include "X264Encoder.h"
#include "ColorConversion.h"
#include "UniqueFd.h"
#include <array>
#include <cerrno>
#include <cmath>
#include <fcntl.h>
#include <iostream>
#include <stdexcept>
#include <vector>

namespace {
void Require(bool test, const char* message) { if (!test) throw std::runtime_error(message); }
bool HasNal(const std::vector<uint8_t>& data, uint8_t type) {
    for (size_t i = 0; i + 3 < data.size(); ++i)
        if (data[i] == 0 && data[i + 1] == 0 && data[i + 2] == 1 && (data[i + 3] & 31) == type) return true;
    return false;
}
}
int main() {
    try {
        const std::array<std::array<int, 6>, 5> colors{{
            {0, 0, 0, 16, 128, 128}, {255, 255, 255, 235, 128, 128},
            {255, 0, 0, 63, 102, 240}, {0, 255, 0, 173, 42, 26}, {0, 0, 255, 32, 240, 118}}};
        for (const auto& sample : colors) {
            const auto yuv = color::RgbToYuv(sample[0], sample[1], sample[2]);
            Require(std::abs(yuv.y - sample[3]) <= 1 && std::abs(yuv.u - sample[4]) <= 1 &&
                    std::abs(yuv.v - sample[5]) <= 1, "BT.709 limited-range conversion failed");
        }
        for (int i = 0; i < 1000; ++i) {
            int descriptors[2]{};
            Require(pipe(descriptors) == 0, "pipe failed");
            {
                UniqueFd read(descriptors[0]), write(descriptors[1]);
                UniqueFd moved(std::move(read));
                Require(read.Get() == -1 && moved.Get() == descriptors[0], "FD move lost ownership");
                UniqueFd transferred(moved.Release());
                Require(moved.Get() == -1 && fcntl(transferred.Get(), F_GETFD) >= 0, "FD transfer closed prematurely");
            }
            Require(fcntl(descriptors[0], F_GETFD) == -1 && errno == EBADF, "Read FD leaked");
            Require(fcntl(descriptors[1], F_GETFD) == -1 && errno == EBADF, "Write FD leaked");
        }
        X264Encoder encoder;
        Require(!encoder.Init(1, 240, 60, 1000000), "Odd dimensions accepted");
        Require(encoder.Init(320, 240, 60, 1000000), "x264 Init failed");
        std::vector<uint8_t> rgba(320 * 240 * 4, 0);
        for (size_t i = 0; i < rgba.size(); i += 4) { rgba[i] = 255; rgba[i + 3] = 255; }
        const auto first = encoder.EncodeFrame(rgba.data());
        Require(HasNal(first, 7) && HasNal(first, 8) && HasNal(first, 5), "Missing SPS/PPS/IDR");
        Require(HasNal(encoder.EncodeFrame(rgba.data()), 1), "Missing P frame");
        encoder.RequestKeyFrame();
        Require(HasNal(encoder.EncodeFrame(rgba.data()), 5), "Forced keyframe missing");
        bool periodicIdr = false;
        for (int i = 0; i < 65; ++i) periodicIdr |= HasNal(encoder.EncodeFrame(rgba.data()), 5);
        Require(periodicIdr, "Periodic IDR missing");
        Require(!encoder.UsesGpuFrames(), "CPU encoder advertised GPU frames");
        bool rejected = false;
        try { (void)encoder.EncodeGpuFrame(); } catch (const std::logic_error&) { rejected = true; }
        Require(rejected, "CPU encoder accepted GPU input");
        Require(encoder.Init(640, 480, 60, 4000000), "Reinitialization failed");
        std::cout << "BT.709 conversion, FD ownership, and CPU H.264 tests passed\n";
        return 0;
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
