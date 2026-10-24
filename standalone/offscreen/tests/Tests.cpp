#include "CameraState.h"
#include "Scene.h"
#ifdef __APPLE__
#include "VideoToolboxEncoder.h"
#else
#include "X264Encoder.h"
#endif
#include "EncoderFactory.h"
#include "WebRTCStreamer.h"
#include <rtc/rtc.hpp>

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
    Require(std::abs(pose.yaw + 0.314159f) < 0.00001f &&
            std::abs(pose.pitch + 0.628319f) < 0.00001f, "Wrong orbit direction");
    RenderControls render;
    Require(!ApplyControlMessage(camera,R"({"type":"set_render_mode","mode":"hybrid"})",&render),
            "Unsupported RT request accepted");
    Require(ApplyControlMessage(camera,R"({"type":"set_render_mode","mode":"raster"})",&render),
            "Raster request rejected");
    render.Configure(true,RenderMode::Raster);
    Require(ApplyControlMessage(camera,R"({"type":"set_render_mode","mode":"hybrid"})",&render) &&
            render.Load().mode==RenderMode::Hybrid,"Hybrid request rejected");
    Require(ApplyControlMessage(camera,R"({"type":"set_render_mode","mode":"pathtrace"})",&render) &&
            render.Load().mode==RenderMode::PathTraced,"Path request rejected");
    for(const auto* message : {R"({"type":"set_render_mode"})",R"({"type":"set_render_mode","mode":1})",
            R"({"type":"set_render_mode","mode":"invalid"})",R"({"type":"set_render_mode","mode":null})"})
        Require(!ApplyControlMessage(camera,message,&render),"Malformed render mode accepted");
    Require(camera.Load().revision==pose.revision,"Mode change modified camera pose");
    const std::vector<std::string> invalid = {
        "garbage", "null", "[]", R"({"type":"rotate","dx":"1","dy":0})",
        R"({"type":"rotate","dx":true,"dy":0})", R"({"type":"rotate","dx":0})",
        R"({"type":"other","dx":0,"dy":0})", R"({"type":"rotate","dx":1e999,"dy":0})",
        R"({"type":"zoom"})", R"({"type":"zoom","delta":"1"})",
        R"({"type":"zoom","delta":true})", R"({"type":"zoom","delta":1e999})",
        R"({"type":42})",
        R"({"type":"pan","dx":0})", R"({"type":"pan","dx":true,"dy":0})",
        R"({"type":"pan","dx":0,"dy":1e999})",
        std::string(513, ' ')};
    for (const auto& message : invalid) Require(!ApplyControlMessage(camera, message), "Malformed controls accepted");
    Require(!camera.Rotate(std::numeric_limits<double>::infinity(), 0), "Infinite controls accepted");
    Require(camera.Load().yaw == pose.yaw && camera.Load().pitch == pose.pitch, "Invalid controls changed camera");
    Require(camera.Load().distance == 3, "Invalid controls changed camera distance");
    Require(!camera.Zoom(std::numeric_limits<double>::quiet_NaN()), "NaN zoom accepted");
    Require(!camera.Zoom(std::numeric_limits<double>::infinity()), "Infinite zoom accepted");
    Require(ApplyControlMessage(camera, R"({"type":"zoom","delta":-0.25})"), "Valid zoom rejected");
    Require(camera.Load().distance < 3 && camera.Load().yaw == pose.yaw && camera.Load().pitch == pose.pitch,
            "Zoom changed orbit or did not move the camera");
    for (int i = 0; i < 20; ++i) camera.Zoom(-1e100);
    Require(camera.Load().distance == CameraState::minDistance, "Near zoom limit failed");
    for (int i = 0; i < 20; ++i) camera.Zoom(1e100);
    Require(camera.Load().distance == CameraState::maxDistance, "Far zoom limit failed");
    const auto orbitBeforePan=camera.Load();
    Require(ApplyControlMessage(camera,R"({"type":"pan","dx":0.1,"dy":0.1})"),"Pan rejected");
    Require(glm::length(camera.Load().target)>0.1f && camera.Load().distance==orbitBeforePan.distance &&
        camera.Load().yaw==orbitBeforePan.yaw,"Pan changed orbit/distance instead of target");
    Require(!camera.Pan(std::numeric_limits<double>::infinity(),0),"Non-finite pan accepted");
    Require(ApplyControlMessage(camera, R"({"type":"reset"})"), "Reset rejected");
    Require(camera.Load().yaw == 0 && camera.Load().pitch == 0 && camera.Load().distance == 3,
            "Reset did not restore the default camera");
    Require(camera.Load().target==glm::vec3(0),"Reset did not restore camera target");
    camera.Rotate(1e100, 1e100);
    Require(std::abs(camera.Load().pitch) <= 1.4f && std::isfinite(camera.Load().yaw), "Camera limits failed");

    CameraState shared;
    std::vector<std::jthread> writers;
    for (int i = 0; i < 4; ++i) writers.emplace_back([&] {
        for (int n = 0; n < 1000; ++n) shared.Rotate(0.0001, 0);
    });
    writers.clear();
    Require(std::abs(shared.Load().yaw + 0.4f * std::numbers::pi_v<float>) < 0.001f, "Concurrent orbit updates lost");
    CameraState concurrent, sequential;
    {
        std::jthread orbit([&] { for (int n = 0; n < 100; ++n) concurrent.Rotate(0.001, -0.001); });
        std::jthread zoom([&] { for (int n = 0; n < 100; ++n) concurrent.Zoom(0.001); });
    }
    for (int n = 0; n < 100; ++n) { sequential.Rotate(0.001, -0.001); sequential.Zoom(0.001); }
    const auto actual = concurrent.Load(), expected = sequential.Load();
    Require(actual.yaw == expected.yaw && actual.pitch == expected.pitch && actual.distance == expected.distance,
            "Concurrent orbit and zoom overwrote each other");
    const auto matrix = scene::MVP(0, {}, 4.0f / 3.0f);
    Require(matrix[15] > 0 && matrix[14] / matrix[15] > 0 && matrix[14] / matrix[15] < 1,
            "Projection depth is outside Vulkan's range");
    Require(matrix != scene::MVP(1, {}, 4.0f / 3.0f), "Model does not rotate");
    Require(matrix != scene::MVP(0, {0.3f, 0.2f}, 4.0f / 3.0f), "Camera does not orbit");
    Require(matrix != scene::MVP(0, {0, 0, 2}, 4.0f / 3.0f), "Camera distance does not affect projection");
    CameraPose shifted; shifted.target={0.4f,0.2f,0};
    Require(matrix!=scene::MVP(0,shifted,4.0f/3.0f),"Pan does not affect view/projection");
    const auto view=CameraView(shifted,16.f/9.f);
    Require(std::abs(glm::length(view.eye-shifted.target)-shifted.distance)<0.0001f,"Arcball eye is not centered on target");
}

void TestEncoder() {
#ifdef __APPLE__
    VideoToolboxEncoder encoder;
#else
    X264Encoder encoder;
#endif
    Require(!encoder.Init(0, 480, 60, 4000000), "Zero width accepted");
    Require(!encoder.Init(641, 480, 60, 4000000), "Odd width accepted");
    Require(!encoder.Init(7682, 4320, 60, 4000000), "Above-8K resolution accepted");
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
    Require(encoder.Init(2560, 1440, 60, 16000000), "2K encoder Init failed");
    pixels.assign(2560 * 1440 * 4, 255);
    const auto highResolution = encoder.EncodeFrame(pixels.data());
    Require(HasNal(highResolution, 7) && HasNal(highResolution, 5), "2K encoder did not produce a keyframe");
}

void TestVideoConfig() {
    const auto low = ResolutionPreset("2k"), high = ResolutionPreset("8k");
    Require(low.width == 2560 && low.height == 1440 && high.width == 7680 && high.height == 4320,
            "Resolution presets are incorrect");
    Require(H264Level(640, 480, 60) == 31 && H264Level(2560, 1440, 60) == 51 &&
            H264Level(3840, 2160, 60) == 52 && H264Level(7680, 4320, 60) == 61,
            "Wrong H.264 levels for requested frame sizes/rates");
    Require(H264Level(640, 480, 60, 16000000) == 51 && H264Level(7680, 2, 60) == 51,
            "H.264 bitrate or dimension limits were ignored");
    Require(ValidVideoSettings(7680, 4320, 60, 100000000) &&
            !ValidVideoSettings(7682, 4320, 60, 100000000) &&
            !ValidVideoSettings(2560, 1440, 60, 100000001) &&
            !ValidVideoSettings(2560, 1440, 61, 16000000), "Video limits failed");
    bool rejected = false;
    try { (void)ResolutionPreset("1080p"); } catch (const std::invalid_argument&) { rejected = true; }
    Require(rejected, "Below-minimum resolution accepted");

    // A sequence-header OBU followed by a frame OBU in NVENC's low-overhead format.
    const rtc::binary obu{rtc::byte{0x0a}, rtc::byte{1}, rtc::byte{0},
        rtc::byte{0x32}, rtc::byte{3}, rtc::byte{0x11}, rtc::byte{0x22}, rtc::byte{0x33}};
    auto config = std::make_shared<rtc::RtpPacketizationConfig>(42, "test", 96, 90000);
    rtc::AV1RtpPacketizer packetizer(rtc::AV1RtpPacketizer::Packetization::TemporalUnit, config);
    rtc::message_vector messages{rtc::make_message(obu.begin(), obu.end())};
    packetizer.outgoing(messages, [](rtc::message_ptr) {});
    Require(!messages.empty() && messages.back()->size() > 12 &&
            (std::to_integer<unsigned>((*messages.back())[1]) & 0x80) != 0,
            "AV1 temporal unit was not packetized/marked");
}
} // namespace

int main() {
    try {
        TestControls();
        TestVideoConfig();
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
