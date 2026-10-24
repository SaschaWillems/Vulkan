#include "CameraState.h"
#include "Scene.h"
#ifdef __APPLE__
#include "VideoToolboxEncoder.h"
#else
#include "X264Encoder.h"
#endif
#include "EncoderFactory.h"
#include "WebRTCStreamer.h"

#include <algorithm>
#include <array>
#include <cmath>
#include <iostream>
#include <limits>
#include <stdexcept>
#include <thread>
#include <vector>

namespace {
void Require(bool condition, const char* message) {
    if (!condition) throw std::runtime_error(message);
}

bool HasNal(const std::vector<uint8_t>& bytes, uint8_t type) {
    for (size_t i = 0; i + 3 < bytes.size(); ++i)
        if (bytes[i] == 0 && bytes[i + 1] == 0 && bytes[i + 2] == 1 &&
            (bytes[i + 3] & 31) == type) return true;
    return false;
}

void TestControls() {
    CameraState camera;
    Require(ApplyControlMessage(camera, R"({"type":"rotate","dx":0.1,"dy":-0.2})"), "Valid controls rejected");
    const auto pose = camera.Load();
    Require(std::abs(pose.yaw - 0.314159f) < 0.00001f && pose.pitch < -0.62f, "Wrong orbit delta");
    const std::vector<std::string> invalid = {
        "garbage", "null", "[]", R"({"type":"rotate","dx":"1","dy":0})",
        R"({"type":"rotate","dx":true,"dy":0})", R"({"type":"rotate","dx":0})",
        R"({"type":"other","dx":0,"dy":0})", R"({"type":"rotate","dx":1e999,"dy":0})",
        std::string(513, ' ')};
    for (const auto& message : invalid) Require(!ApplyControlMessage(camera, message), "Malformed controls accepted");
    Require(!camera.Rotate(std::numeric_limits<double>::infinity(), 0), "Infinite controls accepted");
    Require(camera.Load().yaw == pose.yaw && camera.Load().pitch == pose.pitch, "Invalid controls changed camera");
    camera.Rotate(1e100, 1e100);
    Require(std::abs(camera.Load().pitch) <= 1.4f && std::isfinite(camera.Load().yaw), "Camera limits failed");

    CameraState shared;
    std::vector<std::jthread> writers;
    for (int i = 0; i < 4; ++i) writers.emplace_back([&] {
        for (int n = 0; n < 1000; ++n) shared.Rotate(0.0001, 0);
    });
    writers.clear();
    Require(std::abs(shared.Load().yaw - 0.4f * std::numbers::pi_v<float>) < 0.001f, "Concurrent orbit updates lost");
    const auto matrix = scene::MVP(0, {}, 4.0f / 3.0f);
    Require(matrix[15] > 0 && matrix[14] / matrix[15] > 0 && matrix[14] / matrix[15] < 1,
            "Projection depth is outside Vulkan's range");
    Require(matrix != scene::MVP(1, {}, 4.0f / 3.0f), "Model does not rotate");
    Require(matrix != scene::MVP(0, {0.3f, 0.2f}, 4.0f / 3.0f), "Camera does not orbit");
}

void TestEncoder() {
#ifdef __APPLE__
    VideoToolboxEncoder encoder;
#else
    X264Encoder encoder;
#endif
    Require(!encoder.Init(0, 480, 60, 4000000), "Zero width accepted");
    Require(!encoder.Init(641, 480, 60, 4000000), "Odd width accepted");
    Require(!encoder.Init(1920, 1080, 60, 4000000), "Out-of-level resolution accepted");
    bool rejected = false;
    try { (void)encoder.EncodeFrame(nullptr); } catch (const std::invalid_argument&) { rejected = true; }
    Require(rejected, "Uninitialized/null encoding was not rejected");
    Require(encoder.Init(640, 480, 60, 4000000), "Encoder Init failed");
#ifdef __APPLE__
    Require(encoder.IsHardwareAccelerated(), "Encoder is not hardware accelerated");
#endif
    std::vector<uint8_t> pixels(640 * 480 * 4, 255);
    for (size_t i = 0; i < pixels.size(); i += 4) { pixels[i + 1] = 0; pixels[i + 2] = 0; }
    const auto first = encoder.EncodeFrame(pixels.data());
    Require(HasNal(first, 7) && HasNal(first, 8) && HasNal(first, 5), "First frame lacks SPS/PPS/IDR");
    bool predicted = false;
    for (int i = 0; i < 5; ++i) predicted |= HasNal(encoder.EncodeFrame(pixels.data()), 1);
    Require(predicted, "Encoder did not emit P frames");
    encoder.RequestKeyFrame();
    const auto forced = encoder.EncodeFrame(pixels.data());
    Require(HasNal(forced, 7) && HasNal(forced, 8) && HasNal(forced, 5), "Forced IDR lacks parameter sets");
    Require(encoder.Init(320, 240, 60, 1000000), "Encoder reinitialization failed");
    Require(HasNal(encoder.EncodeFrame(pixels.data()), 5), "Reinitialized encoder failed");
}
} // namespace

int main() {
    try {
        TestControls();
        TestEncoder();
#ifdef __linux__
        const EncoderSettings settings{640, 480, 60, 4000000};
        Require(!CreateVideoEncoder(settings, EncoderPreference::Automatic)->UsesGpuFrames(),
                "Factory did not fall back without an exported frame source");
        bool rejected = false;
        try { (void)CreateVideoEncoder(settings, EncoderPreference::Hardware); }
        catch (const std::runtime_error&) { rejected = true; }
        Require(rejected, "Explicit hardware selection silently fell back");
#endif
        std::cout << "Controls, concurrent camera updates, projection, and H.264 encoder tests passed\n";
        return 0;
    } catch (const std::exception& error) {
        std::cerr << "Test failed: " << error.what() << '\n';
        return 1;
    }
}
