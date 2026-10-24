#include "NvencZeroCopyEncoder.h"
#include "CudaVulkanInterop.h"
#include "VulkanMemoryExporter.h"
#include <nvEncodeAPI.h>
#include <atomic>
#include <dlfcn.h>
#include <iostream>
#include <stdexcept>
#include <string>

namespace {
void Check(NVENCSTATUS status, const char* expression) {
    if (status != NV_ENC_SUCCESS)
        throw std::runtime_error(std::string(expression) + " failed (NVENCSTATUS " + std::to_string(status) + ")");
}
#define NVENC_CHECK(expression) Check((expression), #expression)
template<typename T> T Load(void* library, const char* name) {
    const auto function = reinterpret_cast<T>(dlsym(library, name));
    if (!function) throw std::runtime_error(std::string("Missing NVENC entry point: ") + name);
    return function;
}
}

struct NvencZeroCopyEncoder::Impl {
    void* library = nullptr;
    NV_ENCODE_API_FUNCTION_LIST api{};
    void* encoder = nullptr;
    bool initialized = false;
    std::unique_ptr<CudaVulkanInterop> interop;
    NV_ENC_REGISTERED_PTR registered = nullptr;
    NV_ENC_INPUT_PTR mapped = nullptr;
    NV_ENC_OUTPUT_PTR output = nullptr;
    bool bitstreamLocked = false, pending = false;
    uint32_t width = 0, height = 0, fps = 0;
    uint64_t frame = 0;
    std::atomic<bool> keyframe{true};

    ~Impl() {
        if (encoder) {
            if (initialized) {
                NV_ENC_PIC_PARAMS eos{};
                eos.version = NV_ENC_PIC_PARAMS_VER;
                eos.encodePicFlags = NV_ENC_PIC_FLAG_EOS;
                (void)api.nvEncEncodePicture(encoder, &eos);
            }
            // Cover partial encode/bitstream-copy failures before unregistering GPU memory.
            if (pending && output && !bitstreamLocked) {
                NV_ENC_LOCK_BITSTREAM lock{};
                lock.version = NV_ENC_LOCK_BITSTREAM_VER;
                lock.outputBitstream = output;
                bitstreamLocked = api.nvEncLockBitstream(encoder, &lock) == NV_ENC_SUCCESS;
            }
            if (bitstreamLocked) (void)api.nvEncUnlockBitstream(encoder, output);
            if (mapped) (void)api.nvEncUnmapInputResource(encoder, mapped);
            if (output) (void)api.nvEncDestroyBitstreamBuffer(encoder, output);
            if (registered) (void)api.nvEncUnregisterResource(encoder, registered);
            (void)api.nvEncDestroyEncoder(encoder);
        }
        interop.reset();
        if (library) dlclose(library);
    }
};

NvencZeroCopyEncoder::NvencZeroCopyEncoder(VulkanMemoryExporter& source)
    : source_(source), impl_(std::make_unique<Impl>()) {}
NvencZeroCopyEncoder::~NvencZeroCopyEncoder() = default;

bool NvencZeroCopyEncoder::Init(uint32_t width, uint32_t height, uint32_t fps, uint32_t bitrate) {
    impl_ = std::make_unique<Impl>();
    const uint64_t blocks = ((uint64_t{width} + 15) / 16) * ((uint64_t{height} + 15) / 16);
    if (!width || !height || width % 2 || height % 2 || !fps || fps > 120 ||
        width != source_.Width() || height != source_.Height() || blocks > 3600 ||
        blocks * fps > 108000 || bitrate < 1000 || bitrate > 14000000) return false;
    try {
        auto& state = *impl_;
        // Runtime loading allows x264 fallback even when NVIDIA driver libraries are absent.
        state.library = dlopen("libnvidia-encode.so.1", RTLD_NOW | RTLD_LOCAL);
        if (!state.library) throw std::runtime_error("Cannot load libnvidia-encode.so.1 (NVIDIA video capability)");
        using GetVersion = NVENCSTATUS (NVENCAPI*)(uint32_t*);
        using CreateApi = NVENCSTATUS (NVENCAPI*)(NV_ENCODE_API_FUNCTION_LIST*);
        uint32_t version = 0;
        NVENC_CHECK(Load<GetVersion>(state.library, "NvEncodeAPIGetMaxSupportedVersion")(&version));
        if (version < ((NVENCAPI_MAJOR_VERSION << 4) | NVENCAPI_MINOR_VERSION))
            throw std::runtime_error("NVIDIA driver is too old for the compiled NVENC API");
        state.api.version = NV_ENCODE_API_FUNCTION_LIST_VER;
        NVENC_CHECK(Load<CreateApi>(state.library, "NvEncodeAPICreateInstance")(&state.api));
        state.interop = std::make_unique<CudaVulkanInterop>();
        state.interop->Initialize(source_);
        NV_ENC_OPEN_ENCODE_SESSION_EX_PARAMS open{};
        open.version = NV_ENC_OPEN_ENCODE_SESSION_EX_PARAMS_VER;
        open.apiVersion = NVENCAPI_VERSION;
        open.deviceType = NV_ENC_DEVICE_TYPE_CUDA;
        open.device = state.interop->Context();
        NVENC_CHECK(state.api.nvEncOpenEncodeSessionEx(&open, &state.encoder));

        NV_ENC_PRESET_CONFIG preset{};
        preset.version = NV_ENC_PRESET_CONFIG_VER;
        preset.presetCfg.version = NV_ENC_CONFIG_VER;
        NVENC_CHECK(state.api.nvEncGetEncodePresetConfigEx(state.encoder, NV_ENC_CODEC_H264_GUID,
            NV_ENC_PRESET_P1_GUID, NV_ENC_TUNING_INFO_ULTRA_LOW_LATENCY, &preset));
        auto config = preset.presetCfg;
        config.version = NV_ENC_CONFIG_VER;
        config.profileGUID = NV_ENC_H264_PROFILE_BASELINE_GUID;
        config.gopLength = NVENC_INFINITE_GOPLENGTH;
        config.frameIntervalP = 1; // No B frames, reorder delay, or lookahead.
        config.rcParams.rateControlMode = NV_ENC_PARAMS_RC_CBR;
        config.rcParams.averageBitRate = config.rcParams.maxBitRate = bitrate;
        config.rcParams.vbvBufferSize = config.rcParams.vbvInitialDelay = (bitrate + fps - 1) / fps;
        config.rcParams.enableLookahead = 0;
        config.rcParams.lookaheadDepth = 0;
        config.rcParams.zeroReorderDelay = 1;
        config.rcParams.multiPass = NV_ENC_MULTI_PASS_DISABLED;
        auto& h264 = config.encodeCodecConfig.h264Config;
        h264.level = NV_ENC_LEVEL_H264_31;
        h264.idrPeriod = NVENC_INFINITE_GOPLENGTH;
        h264.repeatSPSPPS = 1;
        h264.chromaFormatIDC = 1;
        h264.entropyCodingMode = NV_ENC_H264_ENTROPY_CODING_MODE_CAVLC;
        h264.h264VUIParameters.videoSignalTypePresentFlag = 1;
        h264.h264VUIParameters.videoFormat = NV_ENC_VUI_VIDEO_FORMAT_UNSPECIFIED;
        h264.h264VUIParameters.videoFullRangeFlag = 0;
        h264.h264VUIParameters.colourDescriptionPresentFlag = 1;
        h264.h264VUIParameters.colourPrimaries = NV_ENC_VUI_COLOR_PRIMARIES_BT709;
        h264.h264VUIParameters.transferCharacteristics = NV_ENC_VUI_TRANSFER_CHARACTERISTIC_BT709;
        h264.h264VUIParameters.colourMatrix = NV_ENC_VUI_MATRIX_COEFFS_BT709;
        NV_ENC_INITIALIZE_PARAMS initialize{};
        initialize.version = NV_ENC_INITIALIZE_PARAMS_VER;
        initialize.encodeGUID = NV_ENC_CODEC_H264_GUID;
        initialize.presetGUID = NV_ENC_PRESET_P1_GUID;
        initialize.tuningInfo = NV_ENC_TUNING_INFO_ULTRA_LOW_LATENCY;
        initialize.encodeWidth = initialize.darWidth = initialize.maxEncodeWidth = width;
        initialize.encodeHeight = initialize.darHeight = initialize.maxEncodeHeight = height;
        initialize.frameRateNum = fps;
        initialize.frameRateDen = 1;
        initialize.enableEncodeAsync = 0; // Linux NVENC uses synchronous bitstream retrieval.
        initialize.enablePTD = 1;
        initialize.encodeConfig = &config;
        NVENC_CHECK(state.api.nvEncInitializeEncoder(state.encoder, &initialize));
        state.initialized = true;
        NV_ENC_REGISTER_RESOURCE resource{};
        resource.version = NV_ENC_REGISTER_RESOURCE_VER;
        resource.resourceType = NV_ENC_INPUT_RESOURCE_TYPE_CUDADEVICEPTR;
        resource.resourceToRegister = state.interop->Nv12Pointer();
        resource.width = width;
        resource.height = height;
        resource.pitch = state.interop->Pitch();
        resource.bufferFormat = NV_ENC_BUFFER_FORMAT_NV12;
        resource.bufferUsage = NV_ENC_INPUT_IMAGE;
        NVENC_CHECK(state.api.nvEncRegisterResource(state.encoder, &resource));
        state.registered = resource.registeredResource;
        NV_ENC_CREATE_BITSTREAM_BUFFER bitstream{};
        bitstream.version = NV_ENC_CREATE_BITSTREAM_BUFFER_VER;
        NVENC_CHECK(state.api.nvEncCreateBitstreamBuffer(state.encoder, &bitstream));
        state.output = bitstream.bitstreamBuffer;
        state.width = width; state.height = height; state.fps = fps;
        return true;
    } catch (const std::exception& error) {
        std::cerr << "NVENC initialization: " << error.what() << '\n';
        impl_ = std::make_unique<Impl>();
        return false;
    }
}

std::vector<uint8_t> NvencZeroCopyEncoder::EncodeFrame(const uint8_t*) {
    throw std::logic_error("NVENC accepts the shared Vulkan frame through EncodeGpuFrame, not CPU pixels");
}
std::vector<uint8_t> NvencZeroCopyEncoder::EncodeGpuFrame() {
    auto& state = *impl_;
    if (!state.registered || !state.output) throw std::logic_error("NVENC is not initialized");
    state.interop->ConvertFrame();
    NV_ENC_MAP_INPUT_RESOURCE mapping{};
    mapping.version = NV_ENC_MAP_INPUT_RESOURCE_VER;
    mapping.registeredResource = state.registered;
    NVENC_CHECK(state.api.nvEncMapInputResource(state.encoder, &mapping));
    state.mapped = mapping.mappedResource;
    NV_ENC_PIC_PARAMS picture{};
    picture.version = NV_ENC_PIC_PARAMS_VER;
    picture.inputBuffer = state.mapped;
    picture.bufferFmt = mapping.mappedBufferFmt;
    picture.inputWidth = state.width;
    picture.inputHeight = state.height;
    picture.inputPitch = state.interop->Pitch();
    picture.outputBitstream = state.output;
    picture.pictureStruct = NV_ENC_PIC_STRUCT_FRAME;
    picture.frameIdx = static_cast<uint32_t>(state.frame);
    picture.inputTimeStamp = state.frame;
    picture.inputDuration = 1;
    if (state.keyframe.exchange(false) || state.frame % state.fps == 0)
        picture.encodePicFlags = NV_ENC_PIC_FLAG_FORCEIDR | NV_ENC_PIC_FLAG_OUTPUT_SPSPPS;
    const auto status = state.api.nvEncEncodePicture(state.encoder, &picture);
    state.pending = status == NV_ENC_SUCCESS || status == NV_ENC_ERR_NEED_MORE_INPUT;
    NVENC_CHECK(status); // NEED_MORE_INPUT is incompatible with this zero-delay single-slot pipeline.
    NV_ENC_LOCK_BITSTREAM lock{};
    lock.version = NV_ENC_LOCK_BITSTREAM_VER;
    lock.outputBitstream = state.output;
    lock.doNotWait = 0;
    NVENC_CHECK(state.api.nvEncLockBitstream(state.encoder, &lock));
    state.bitstreamLocked = true;
    state.pending = false;
    const auto* bytes = static_cast<const uint8_t*>(lock.bitstreamBufferPtr);
    if (!bytes || !lock.bitstreamSizeInBytes) throw std::runtime_error("NVENC returned an empty bitstream");
    std::vector<uint8_t> result(bytes, bytes + lock.bitstreamSizeInBytes);
    NVENC_CHECK(state.api.nvEncUnlockBitstream(state.encoder, state.output));
    state.bitstreamLocked = false;
    NVENC_CHECK(state.api.nvEncUnmapInputResource(state.encoder, state.mapped));
    state.mapped = nullptr;
    ++state.frame;
    return result;
}
void NvencZeroCopyEncoder::RequestKeyFrame() { impl_->keyframe.store(true, std::memory_order_relaxed); }
