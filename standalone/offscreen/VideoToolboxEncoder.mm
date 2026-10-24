#include "VideoToolboxEncoder.h"

#include <CoreMedia/CoreMedia.h>
#include <CoreVideo/CoreVideo.h>
#include <VideoToolbox/VideoToolbox.h>

#include <atomic>
#include <exception>
#include <iostream>
#include <limits>
#include <mutex>
#include <stdexcept>
#include <string>

namespace {
void Check(OSStatus status, const char* operation) {
    if (status != noErr)
        throw std::runtime_error(std::string(operation) + " failed (OSStatus " + std::to_string(status) + ")");
}

template <typename T> class CFHandle {
public:
    explicit CFHandle(T value = nullptr) : value_(value) {}
    ~CFHandle() { if (value_) CFRelease(value_); }
    CFHandle(const CFHandle&) = delete;
    CFHandle& operator=(const CFHandle&) = delete;
    T Get() const { return value_; }
    T* Put() { return &value_; }
private:
    T value_;
};

CFNumberRef Number(int32_t value) {
    auto number = CFNumberCreate(kCFAllocatorDefault, kCFNumberSInt32Type, &value);
    if (!number) throw std::bad_alloc();
    return number;
}

void SetInteger(VTCompressionSessionRef session, CFStringRef key, int32_t value) {
    CFHandle<CFNumberRef> number(Number(value));
    char name[128]{};
    if (!CFStringGetCString(key, name, sizeof(name), kCFStringEncodingUTF8))
        throw std::runtime_error("Invalid VideoToolbox property name");
    Check(VTSessionSetProperty(session, key, number.Get()), name);
}

void AppendNal(std::vector<uint8_t>& output, const uint8_t* bytes, size_t length) {
    if (!bytes || length == 0) throw std::runtime_error("Empty H.264 NAL unit");
    output.insert(output.end(), {0, 0, 0, 1});
    output.insert(output.end(), bytes, bytes + length);
}

std::vector<uint8_t> AnnexB(CMSampleBufferRef sample) {
    if (!sample || !CMSampleBufferDataIsReady(sample))
        throw std::runtime_error("VideoToolbox returned an unready sample");
    const auto format = CMSampleBufferGetFormatDescription(sample);
    if (!format) throw std::runtime_error("Missing H.264 format description");
    size_t parameterCount = 0;
    int lengthBytes = 0;
    Check(CMVideoFormatDescriptionGetH264ParameterSetAtIndex(
        format, 0, nullptr, nullptr, &parameterCount, &lengthBytes), "Get H.264 format");
    if (lengthBytes < 1 || lengthBytes > 4) throw std::runtime_error("Invalid H.264 NAL length size");

    const auto attachments = CMSampleBufferGetSampleAttachmentsArray(sample, false);
    bool keyframe = true;
    if (attachments && CFArrayGetCount(attachments) > 0) {
        const auto first = static_cast<CFDictionaryRef>(CFArrayGetValueAtIndex(attachments, 0));
        keyframe = CFDictionaryGetValue(first, kCMSampleAttachmentKey_NotSync) != kCFBooleanTrue;
    }
    std::vector<uint8_t> output;
    // Repeat SPS/PPS on every IDR so new viewers and PLI recovery do not need old frames.
    if (keyframe) {
        for (size_t index = 0; index < parameterCount; ++index) {
            const uint8_t* bytes = nullptr;
            size_t size = 0;
            Check(CMVideoFormatDescriptionGetH264ParameterSetAtIndex(
                format, index, &bytes, &size, nullptr, nullptr), "Get H.264 parameter set");
            AppendNal(output, bytes, size);
        }
    }
    const auto block = CMSampleBufferGetDataBuffer(sample);
    if (!block) throw std::runtime_error("Missing H.264 sample data");
    // CMBlockBuffer need not be contiguous. CopyDataBytes handles chained blocks too.
    std::vector<uint8_t> avcc(CMBlockBufferGetDataLength(block));
    Check(CMBlockBufferCopyDataBytes(block, 0, avcc.size(), avcc.data()), "Read H.264 sample");
    size_t offset = 0;
    while (offset < avcc.size()) {
        if (avcc.size() - offset < static_cast<size_t>(lengthBytes))
            throw std::runtime_error("Truncated AVCC NAL length");
        uint32_t size = 0;
        for (int i = 0; i < lengthBytes; ++i) size = (size << 8) | avcc[offset++];
        if (size == 0 || size > avcc.size() - offset)
            throw std::runtime_error("Invalid AVCC NAL payload size");
        AppendNal(output, avcc.data() + offset, size);
        offset += size;
    }
    return output;
}
} // namespace

struct VideoToolboxEncoder::Impl {
    VTCompressionSessionRef session = nullptr;
    uint32_t width = 0, height = 0, fps = 0;
    int64_t frameIndex = 0;
    bool hardware = false;
    std::atomic<bool> forceKeyframe{true};
    std::mutex outputMutex;
    std::vector<uint8_t> output;
    std::exception_ptr failure;
    bool callbackReceived = false;

    ~Impl() { Reset(); }
    void Reset() {
        if (session) {
            // Drain before destroying callback state, including error paths.
            (void)VTCompressionSessionCompleteFrames(session, kCMTimeInvalid);
            VTCompressionSessionInvalidate(session);
            CFRelease(session);
            session = nullptr;
        }
        hardware = false;
    }

    static void OnEncoded(void* context, void*, OSStatus status,
                          VTEncodeInfoFlags flags, CMSampleBufferRef sample) noexcept {
        auto& self = *static_cast<Impl*>(context);
        std::lock_guard lock(self.outputMutex);
        self.callbackReceived = true;
        try {
            Check(status, "VideoToolbox encoding callback");
            if (!(flags & kVTEncodeInfo_FrameDropped)) self.output = AnnexB(sample);
        } catch (...) {
            self.failure = std::current_exception();
        }
    }
};

VideoToolboxEncoder::VideoToolboxEncoder() : impl_(std::make_unique<Impl>()) {}
VideoToolboxEncoder::~VideoToolboxEncoder() = default;

bool VideoToolboxEncoder::Init(uint32_t width, uint32_t height, uint32_t fps, uint32_t bitrate) {
    auto& state = *impl_;
    state.Reset();
    // Match the SDP's H.264 Baseline level 3.1 limits, including macroblocks per second.
    const uint64_t macroblocks = ((uint64_t{width} + 15) / 16) * ((uint64_t{height} + 15) / 16);
    if (!width || !height || width % 2 || height % 2 || !fps || fps > 120 ||
        macroblocks > 3600 || macroblocks * fps > 108000 || !bitrate || bitrate > 14000000)
        return false;
    try {
        CFHandle<CFNumberRef> w(Number(static_cast<int32_t>(width)));
        CFHandle<CFNumberRef> h(Number(static_cast<int32_t>(height)));
        CFHandle<CFNumberRef> pixelFormat(Number(kCVPixelFormatType_32BGRA));
        CFHandle<CFDictionaryRef> surface(CFDictionaryCreate(kCFAllocatorDefault, nullptr, nullptr, 0,
            &kCFTypeDictionaryKeyCallBacks, &kCFTypeDictionaryValueCallBacks));
        if (!surface.Get()) throw std::bad_alloc();
        const void* keys[] = {kCVPixelBufferWidthKey, kCVPixelBufferHeightKey,
                             kCVPixelBufferPixelFormatTypeKey, kCVPixelBufferIOSurfacePropertiesKey};
        const void* values[] = {w.Get(), h.Get(), pixelFormat.Get(), surface.Get()};
        CFHandle<CFDictionaryRef> attributes(CFDictionaryCreate(kCFAllocatorDefault, keys, values, 4,
            &kCFTypeDictionaryKeyCallBacks, &kCFTypeDictionaryValueCallBacks));
        const void* hardwareKey = kVTVideoEncoderSpecification_RequireHardwareAcceleratedVideoEncoder;
        const void* hardwareValue = kCFBooleanTrue;
        CFHandle<CFDictionaryRef> specification(CFDictionaryCreate(kCFAllocatorDefault,
            &hardwareKey, &hardwareValue, 1, &kCFTypeDictionaryKeyCallBacks, &kCFTypeDictionaryValueCallBacks));
        if (!attributes.Get() || !specification.Get()) throw std::bad_alloc();
        Check(VTCompressionSessionCreate(kCFAllocatorDefault, static_cast<int32_t>(width),
            static_cast<int32_t>(height), kCMVideoCodecType_H264, specification.Get(), attributes.Get(),
            nullptr, Impl::OnEncoded, &state, &state.session), "VTCompressionSessionCreate(hardware H.264)");
        Check(VTSessionSetProperty(state.session, kVTCompressionPropertyKey_RealTime, kCFBooleanTrue), "Set realtime");
        Check(VTSessionSetProperty(state.session, kVTCompressionPropertyKey_AllowFrameReordering, kCFBooleanFalse), "Disable B frames");
        Check(VTSessionSetProperty(state.session, kVTCompressionPropertyKey_ProfileLevel, kVTProfileLevel_H264_Baseline_3_1), "Set H.264 profile");
        SetInteger(state.session, kVTCompressionPropertyKey_ExpectedFrameRate, static_cast<int32_t>(fps));
        SetInteger(state.session, kVTCompressionPropertyKey_AverageBitRate, static_cast<int32_t>(bitrate));
        SetInteger(state.session, kVTCompressionPropertyKey_MaxKeyFrameInterval, static_cast<int32_t>(fps));
        Check(VTCompressionSessionPrepareToEncodeFrames(state.session), "Prepare VideoToolbox");
        CFHandle<CFTypeRef> hardware;
        Check(VTSessionCopyProperty(state.session, kVTCompressionPropertyKey_UsingHardwareAcceleratedVideoEncoder,
            kCFAllocatorDefault, hardware.Put()), "Query hardware encoder");
        state.hardware = hardware.Get() == kCFBooleanTrue;
        if (!state.hardware) throw std::runtime_error("VideoToolbox did not select a hardware encoder");
        state.width = width;
        state.height = height;
        state.fps = fps;
        state.frameIndex = 0;
        state.forceKeyframe.store(true);
        return true;
    } catch (const std::exception& error) {
        std::cerr << "Encoder initialization: " << error.what() << '\n';
        state.Reset();
        return false;
    }
}

std::vector<uint8_t> VideoToolboxEncoder::EncodeFrame(const uint8_t* rgbaPixels) {
    auto& state = *impl_;
    if (!state.session || !rgbaPixels) throw std::invalid_argument("EncodeFrame requires Init and RGBA pixels");
    const auto pool = VTCompressionSessionGetPixelBufferPool(state.session);
    if (!pool) throw std::runtime_error("VideoToolbox has no pixel buffer pool");
    CFHandle<CVPixelBufferRef> pixel;
    Check(CVPixelBufferPoolCreatePixelBuffer(kCFAllocatorDefault, pool, pixel.Put()), "Allocate encoder pixel buffer");
    Check(CVPixelBufferLockBaseAddress(pixel.Get(), 0), "Lock encoder pixel buffer");
    auto* destination = static_cast<uint8_t*>(CVPixelBufferGetBaseAddress(pixel.Get()));
    const size_t stride = CVPixelBufferGetBytesPerRow(pixel.Get());
    if (!destination || stride < static_cast<size_t>(state.width) * 4) {
        (void)CVPixelBufferUnlockBaseAddress(pixel.Get(), 0);
        throw std::runtime_error("Invalid BGRA pixel buffer layout");
    }
    // Respect CoreVideo's padded row stride and swizzle RGBA to the required BGRA layout.
    for (uint32_t y = 0; y < state.height; ++y) {
        const auto* source = rgbaPixels + static_cast<size_t>(y) * state.width * 4;
        auto* row = destination + static_cast<size_t>(y) * stride;
        for (uint32_t x = 0; x < state.width; ++x) {
            row[4 * x] = source[4 * x + 2];
            row[4 * x + 1] = source[4 * x + 1];
            row[4 * x + 2] = source[4 * x];
            row[4 * x + 3] = source[4 * x + 3];
        }
    }
    Check(CVPixelBufferUnlockBaseAddress(pixel.Get(), 0), "Unlock encoder pixel buffer");
    {
        std::lock_guard lock(state.outputMutex);
        state.output.clear();
        state.failure = nullptr;
        state.callbackReceived = false;
    }
    const void* forceKey = kVTEncodeFrameOptionKey_ForceKeyFrame;
    const void* forceValue = kCFBooleanTrue;
    const bool forceKeyframe = state.forceKeyframe.exchange(false);
    CFHandle<CFDictionaryRef> frameOptions(forceKeyframe
        ? CFDictionaryCreate(kCFAllocatorDefault, &forceKey, &forceValue, 1,
            &kCFTypeDictionaryKeyCallBacks, &kCFTypeDictionaryValueCallBacks) : nullptr);
    if (forceKeyframe && !frameOptions.Get()) throw std::bad_alloc();
    const CMTime pts = CMTimeMake(state.frameIndex++, static_cast<int32_t>(state.fps));
    VTEncodeInfoFlags flags = 0;
    Check(VTCompressionSessionEncodeFrame(state.session, pixel.Get(), pts,
        CMTimeMake(1, static_cast<int32_t>(state.fps)), frameOptions.Get(), nullptr, &flags), "Encode H.264 frame");
    // The interface returns one access unit synchronously. Draining each frame bounds latency
    // and gives the callback time to finish before this frame's result is consumed.
    Check(VTCompressionSessionCompleteFrames(state.session, pts), "Complete H.264 frame");
    std::lock_guard lock(state.outputMutex);
    if (state.failure) std::rethrow_exception(state.failure);
    if (!state.callbackReceived && !(flags & kVTEncodeInfo_FrameDropped))
        throw std::runtime_error("VideoToolbox completed without an output callback");
    return std::move(state.output);
}

void VideoToolboxEncoder::RequestKeyFrame() { impl_->forceKeyframe.store(true, std::memory_order_relaxed); }
bool VideoToolboxEncoder::IsHardwareAccelerated() const { return impl_->hardware; }
