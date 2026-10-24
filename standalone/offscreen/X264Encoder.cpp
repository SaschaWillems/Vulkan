#include "X264Encoder.h"
#include "ColorConversion.h"

#include <x264.h>
#include <atomic>
#include <iostream>
#include <stdexcept>

struct X264Encoder::Impl {
    x264_t* encoder = nullptr;
    x264_picture_t picture{};
    std::vector<uint8_t> nv12;
    uint32_t width = 0, height = 0;
    int64_t frame = 0;
    std::atomic<bool> forceKeyframe{true};
    ~Impl() { if (encoder) x264_encoder_close(encoder); }
};

X264Encoder::X264Encoder() : impl_(std::make_unique<Impl>()) {}
X264Encoder::~X264Encoder() = default;

bool X264Encoder::Init(uint32_t width, uint32_t height, uint32_t fps, uint32_t bitrate) {
    impl_ = std::make_unique<Impl>();
    const uint64_t blocks = ((uint64_t{width} + 15) / 16) * ((uint64_t{height} + 15) / 16);
    if (!width || !height || width % 2 || height % 2 || !fps || fps > 120 ||
        blocks > 3600 || blocks * fps > 108000 || bitrate < 1000 || bitrate > 14000000) return false;
    auto& state = *impl_;
    x264_param_t parameters{};
    if (x264_param_default_preset(&parameters, "ultrafast", "zerolatency") < 0) return false;
    parameters.i_width = static_cast<int>(width);
    parameters.i_height = static_cast<int>(height);
    parameters.i_csp = X264_CSP_NV12;
    parameters.i_fps_num = fps;
    parameters.i_fps_den = 1;
    parameters.i_timebase_num = 1;
    parameters.i_timebase_den = fps;
    parameters.b_vfr_input = 0;
    parameters.i_threads = 1;
    parameters.i_bframe = 0;
    parameters.i_keyint_max = static_cast<int>(fps);
    parameters.i_keyint_min = 1;
    parameters.i_scenecut_threshold = 0;
    parameters.i_level_idc = 31;
    parameters.b_annexb = 1;
    parameters.b_repeat_headers = 1;
    parameters.rc.i_rc_method = X264_RC_ABR;
    parameters.rc.i_bitrate = static_cast<int>(bitrate / 1000);
    parameters.rc.i_vbv_max_bitrate = parameters.rc.i_bitrate;
    parameters.rc.i_vbv_buffer_size = (parameters.rc.i_bitrate + static_cast<int>(fps) - 1) / static_cast<int>(fps);
    parameters.rc.b_filler = 1;
    parameters.i_nal_hrd = X264_NAL_HRD_CBR;
    parameters.vui.b_fullrange = 0;
    parameters.vui.i_colorprim = 1;
    parameters.vui.i_transfer = 1;
    parameters.vui.i_colmatrix = 1;
    parameters.i_log_level = X264_LOG_WARNING;
    if (x264_param_apply_profile(&parameters, "baseline") < 0) return false;
    state.encoder = x264_encoder_open(&parameters);
    if (!state.encoder) return false;
    state.width = width;
    state.height = height;
    state.nv12.resize(static_cast<size_t>(width) * height * 3 / 2);
    x264_picture_init(&state.picture);
    state.picture.img.i_csp = X264_CSP_NV12;
    state.picture.img.i_plane = 2;
    state.picture.img.i_stride[0] = static_cast<int>(width);
    state.picture.img.i_stride[1] = static_cast<int>(width);
    state.picture.img.plane[0] = state.nv12.data();
    state.picture.img.plane[1] = state.nv12.data() + static_cast<size_t>(width) * height;
    return true;
}

std::vector<uint8_t> X264Encoder::EncodeFrame(const uint8_t* rgba) {
    auto& state = *impl_;
    if (!state.encoder || !rgba) throw std::invalid_argument("x264 requires Init and RGBA pixels");
    auto* yPlane = state.picture.img.plane[0];
    auto* uvPlane = state.picture.img.plane[1];
    for (uint32_t y = 0; y < state.height; y += 2) {
        for (uint32_t x = 0; x < state.width; x += 2) {
            int r = 0, g = 0, b = 0;
            for (uint32_t dy = 0; dy < 2; ++dy) {
                for (uint32_t dx = 0; dx < 2; ++dx) {
                    const size_t pixel = static_cast<size_t>(y + dy) * state.width + x + dx;
                    const auto* rgb = rgba + pixel * 4;
                    yPlane[pixel] = color::RgbToYuv(rgb[0], rgb[1], rgb[2]).y;
                    r += rgb[0]; g += rgb[1]; b += rgb[2];
                }
            }
            const auto chroma = color::RgbToYuv((r + 2) / 4, (g + 2) / 4, (b + 2) / 4);
            const size_t offset = static_cast<size_t>(y / 2) * state.width + x;
            uvPlane[offset] = chroma.u;
            uvPlane[offset + 1] = chroma.v;
        }
    }
    state.picture.i_pts = state.frame++;
    state.picture.i_type = state.forceKeyframe.exchange(false) ? X264_TYPE_IDR : X264_TYPE_AUTO;
    x264_picture_t output{};
    x264_nal_t* nals = nullptr;
    int count = 0;
    const int bytes = x264_encoder_encode(state.encoder, &nals, &count, &state.picture, &output);
    if (bytes < 0) throw std::runtime_error("x264_encoder_encode failed");
    std::vector<uint8_t> encoded;
    encoded.reserve(static_cast<size_t>(bytes));
    for (int i = 0; i < count; ++i) {
        if (nals[i].i_payload < 0) throw std::runtime_error("Invalid x264 NAL size");
        encoded.insert(encoded.end(), nals[i].p_payload, nals[i].p_payload + nals[i].i_payload);
    }
    return encoded;
}
void X264Encoder::RequestKeyFrame() { impl_->forceKeyframe.store(true, std::memory_order_relaxed); }
