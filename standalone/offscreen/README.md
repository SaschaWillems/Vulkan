# Vulkan → NVENC / VideoToolbox / x264 → WebRTC

A standalone C++20 server that renders a rotating 3D triangle into a 640×480 RGBA Vulkan image and streams H.264 to a browser at a target of 60 FPS / 4 Mbps. Linux uses NVIDIA NVENC with GPU-only framebuffer capture when available, falling back to x264 with host readback. macOS uses VideoToolbox hardware encoding with host readback. Dragging the video orbits the shared camera. No windowing library, surface, or swapchain is used.

## Build on macOS

Install a Vulkan SDK with MoltenVK, recent Xcode command-line tools with C++20 support, CMake 3.21+, Git, and OpenSSL. If the SDK is not installed globally, source its `setup-env.sh` first. For Homebrew installations:

```sh
brew install cmake openssl@3
cmake -S standalone/offscreen -B build/webrtc -DCMAKE_BUILD_TYPE=Release \
  -DOPENSSL_ROOT_DIR="$(brew --prefix openssl@3)"
cmake --build build/webrtc --parallel
./build/webrtc/offscreen_renderer
```

CMake FetchContent downloads and builds **paullouisageneau/libdatachannel v0.24.5**, pinned to commit `443f6934d9007eb7076ab7825ba330f355fcbead`, including its submodules. The `paullouisdefaye/libdatachannel` URL in the original request does not exist. The JSON dependency comes from libdatachannel's pinned submodule. A network connection is needed for the first configure; there are no runtime shader files or shader compiler dependencies.

Open `standalone/offscreen/index.html` (also copied into the build directory), leave `ws://127.0.0.1:8080` as the signaling address, and click **Connect**. If preferred, serve the client locally:

```sh
python3 -m http.server 8000 --bind 127.0.0.1 --directory standalone/offscreen
# Open http://127.0.0.1:8000
```

The browser displays the received FPS/bitrate and confirms camera updates. Ctrl+C or SIGTERM stops the server and releases its resources.

Debug builds require `VK_LAYER_KHRONOS_validation` and `VK_EXT_debug_utils`. Vulkan 1.1 satisfies the portability-subset properties2 dependency; portability enumeration and the device portability subset are enabled when advertised.

## Build on Linux

Install a Vulkan-capable driver, CMake 3.21+, a C++20 compiler, OpenSSL, and x264 development packages. For Ubuntu 22.04:

```sh
sudo apt-get update
sudo apt-get install build-essential cmake ninja-build git pkg-config \
  libvulkan-dev vulkan-tools mesa-vulkan-drivers libssl-dev libx264-dev
cmake -S standalone/offscreen -B build/linux -G Ninja -DCMAKE_BUILD_TYPE=Release
cmake --build build/linux --parallel
./build/linux/offscreen_renderer
```

`ENABLE_NVENC=AUTO` (default) builds NVENC support when CUDA Toolkit 12.4+ and nvcc are installed. Runtime selection prefers an NVIDIA Vulkan device, matches its UUID to CUDA, and attempts NVENC initialization. Otherwise it logs the reason and uses x264. x264 is always built into Linux streaming builds so driver/encoder initialization failures can fall back. `ENABLE_NVENC=OFF` builds without CUDA; `ENABLE_NVENC=ON` requires the CUDA SDK at configure time. GPU hardware is not required during compilation.

The application compiles as C++20. The standalone CUDA conversion kernel compiles as C++17 so Ubuntu 22.04's packaged CMake can configure nvcc without requiring a newer CMake installation.

```sh
cmake -S standalone/offscreen -B build/linux-nvenc -G Ninja \
  -DCMAKE_BUILD_TYPE=Release -DENABLE_NVENC=ON \
  -DCUDAToolkit_ROOT=/usr/local/cuda \
  '-DCMAKE_CUDA_ARCHITECTURES=52;75;86;89'
cmake --build build/linux-nvenc --parallel
./build/linux-nvenc/offscreen_renderer --encoder hardware
```

The build uses installed `nvEncodeAPI.h` headers if found (`NVENC_SDK_ROOT` or `NVENC_INCLUDE_DIR` can select them); otherwise FetchContent downloads [nv-codec-headers n12.2.72.0](https://github.com/FFmpeg/nv-codec-headers/tree/n12.2.72.0), pinned to `c69278340ab1d5559c7d7bf0edf615dc33ddbba7`. These headers require NVIDIA Linux driver 550.54.14 or newer and an NVENC-capable GPU. Newer manually supplied SDK headers may require newer drivers. `libcuda.so.1` and `libnvidia-encode.so.1` are loaded at runtime, allowing the executable to start and use x264 when the driver is absent. CUDA runtime code is statically linked. CUDA 13 builds default to architectures 75/86/89; override architectures to match your deployment GPU and toolkit.

## Runtime options

```sh
./build/webrtc/offscreen_renderer --port 8080 --bind 127.0.0.1
./build/webrtc/offscreen_renderer --frames 120 --port 0  # finite render/encode smoke run
./build/webrtc/offscreen_renderer --snapshot            # write output.ppm in the current directory
./build/webrtc/offscreen_renderer --render-test         # animation, orbit, and resource reuse
# Linux streaming:
./build/linux/offscreen_renderer --encoder software     # explicitly exercise x264
./build/linux-nvenc/offscreen_renderer --encoder hardware --nvenc-input image
./build/linux-nvenc/offscreen_renderer --encoder hardware --nvenc-input buffer
```

`--encoder hardware` fails explicitly if hardware initialization is unavailable; it never silently uses x264. Automatic fallback happens during initialization. A CUDA/NVENC failure after streaming starts stops the process with an error instead of reusing potentially unsynchronized GPU memory.

The server supports up to four viewers, all viewing and controlling the same scene. It binds signaling to loopback by default. For a trusted LAN, use `--bind 0.0.0.0` and enter the server's LAN address in the client. Signaling is plain WebSocket and has no authentication; external deployments need authenticated WSS termination. WebRTC media itself uses DTLS/SRTP.

For separate networks, configure reachable STUN/TURN servers on both endpoints. Repeat `--ice-server <URL>` for the C++ endpoint and use the client's **ICE configuration** JSON array for the browser. STUN alone cannot traverse every NAT; TURN credentials and relay infrastructure are deployment inputs. This example does not provide a public relay or automatically expose any ports.

## Implementation

- `main.cpp`: Vulkan instance/device, MoltenVK portability, embedded shaders, MVP uniform buffer, reusable command buffer, render pass, framebuffer, staging readback, and frame pacing. `Scene.h` implements column-major perspective/model/view matrices with Vulkan's depth range.
- `EncoderFactory.h/.cpp`: platform/backend selection, explicit hardware mode, and Linux CPU fallback. The original `IVideoEncoder::Init`/`EncodeFrame` contract is preserved; GPU backends additionally implement `UsesGpuFrames`/`EncodeGpuFrame` with a bound Vulkan frame source.
- `IVideoEncoder.h`, `VideoToolboxEncoder.h/.mm`: RAII hardware compression session and pixel-buffer pool. RGBA is swizzled to BGRA while respecting CoreVideo row stride. B frames are disabled. The synchronous interface drains each submitted frame before returning its Annex-B access unit. SPS/PPS accompany each IDR; keyframes occur every second or on request. Unsupported hardware encoding fails explicitly.
- `VulkanMemoryExporter.h/.cpp`, `UniqueFd.h`: dedicated device-local memory with opaque-FD export, exportable binary semaphores, device UUID, and FD ownership.
- `CudaVulkanInterop.h/.cpp`, `CudaColorConversion.cu`: CUDA imports, GPU synchronization, and BT.709 limited-range RGBA → NV12 conversion into a pitched CUDA allocation. `ColorConversion.h` shares the conversion coefficients with x264.
- `NvencZeroCopyEncoder.h/.cpp`: runtime-loaded NVENC session, registered CUDA device pointer, H.264 baseline level 3.1, P1 ultra-low-latency tuning, CBR, one-frame VBV, no B frames/lookahead, infinite GOP with an IDR every second or on request. SPS/PPS and Annex-B access units feed the existing WebRTC packetizer.
- `X264Encoder.h/.cpp`: Linux CPU fallback using ultrafast/zerolatency, NV12, CBR/VBV, no B frames, and matching H.264 profile/keyframe behavior.
- `WebRTCStreamer.h/.cpp`: WebSocket offer/answer and trickle ICE, bounded signaling queues, per-peer H.264 RTP packetization, RTCP sender reports, NACK retransmission, and PLI/FIR keyframe requests. Peer creation and teardown are serviced by `Poll()` on the render thread. Callbacks capture weak session references.
- `CameraState.h`: yaw/pitch packed into one lock-free 64-bit atomic; compare/exchange preserves concurrent drag updates and consistent angle snapshots. Deltas are fractions of the displayed video width/height; finite inputs are bounded and pitch is clamped away from the poles. Invalid JSON does not change the camera.
- `index.html`: no external scripts or build tools. It handles asynchronous SDP/ICE ordering, video playback, drag input, statistics, disconnection, and manual reconnection.

**GPU path:** the default exports the actual optimal-tiled color image and imports it as a CUDA mipmapped array/surface. An optimal-tiled image cannot be treated as a linear CUDA buffer. `--nvenc-input buffer` instead copies the image into an exported device-local VkBuffer entirely on the GPU and imports that allocation with `cudaExternalMemoryGetMappedBuffer`. Vulkan image-export failure also tries the buffer path. Both paths convert to NV12 in VRAM and register that CUDA pointer with NVENC; neither allocates or reads a CPU framebuffer. The compressed H.264 bitstream is copied to CPU memory for libdatachannel. GPU conversion and the optional GPU buffer copy still occur: “zero-copy” here means zero raw-frame **host** copies.

Vulkan releases the shared image (in GENERAL layout) or buffer to `VK_QUEUE_FAMILY_EXTERNAL`, then signals a ready semaphore. CUDA waits, converts, and signals a done semaphore. Vulkan waits and reacquires ownership before the next render/copy. The first submission has no done wait. The allocation is dedicated, and CUDA/Vulkan devices must have identical UUIDs. Successful CUDA FD imports transfer ownership to CUDA; failed imports leave the FD in a closing RAII wrapper. NVENC unregisters its CUDA allocation before CUDA destroys mappings/imports; the Vulkan exporter remains alive until afterward.

A single GPU submission is in flight at a time. The fence completes before updating the uniform buffer or reading/reusing mapped memory. Color-attachment writes transition to transfer reads, then transfer writes become host-visible. Non-coherent allocations are explicitly flushed/invalidated over the whole mapped allocation. All Vulkan resources are released in reverse dependency order, including partial-initialization failures.

The pacing clock uses `std::chrono::high_resolution_clock` when it is steady, with `steady_clock` as the monotonic fallback. Overruns skip schedule slots instead of producing a burst of stale frames. RTP timestamps follow capture time on the 90 kHz clock. The target is 60 FPS; measured performance depends on GPU, encoder, network, and browser load. The server logs actual render FPS and missed slots. It renders continuously. NVENC encodes each frame so every semaphore exchange completes; CPU/VideoToolbox encoding runs when viewers are connected or for a finite `--frames` run.

## Tests

```sh
cmake -S standalone/offscreen -B build/webrtc-debug -DCMAKE_BUILD_TYPE=Debug \
  -DOPENSSL_ROOT_DIR="$(brew --prefix openssl@3)"
cmake --build build/webrtc-debug --parallel
ctest --test-dir build/webrtc-debug --output-on-failure

# Node 22+ and an installed Google Chrome; no npm packages required.
node standalone/offscreen/tests/browser_test.mjs \
  build/webrtc-debug/offscreen_renderer build/webrtc-debug
```

CTest verifies PPM dimensions/pixels, file-open errors, animation and orbit changes, uniform/framebuffer reuse, malformed controls, concurrent camera updates, hardware encoding, SPS/PPS/IDR/P-frame output, forced keyframes, encoder reinitialization, and a paced 120-frame render/encode run. The browser test uses an isolated temporary Chrome profile and local ports. It checks actual H.264 decoding, frame rate, decoded pixels, mouse drag acknowledgements, reconnection, validation output, and graceful shutdown. It writes `browser-test.png` and `browser-test-server.log` into the chosen artifact directory. Set `CHROME_BIN` to override the browser executable.

For additional synchronization validation with recent SDKs:

```sh
mkdir -p build/vulkan-validation
printf 'khronos_validation.validate_sync = true\nkhronos_validation.syncval_full_validation = true\n' \
  > build/vulkan-validation/vk_layer_settings.txt
VK_LAYER_SETTINGS_PATH="$PWD/build/vulkan-validation" \
  ctest --test-dir build/webrtc-debug --output-on-failure
```

On macOS, hardware tests require Metal and VideoToolbox access. Linux adds x264/color-conversion/FD ownership and factory fallback tests. On an NVIDIA Linux host, install Vulkan validation layers and explicitly enable both interop tests:

```sh
sudo apt-get install vulkan-validationlayers
cmake -S standalone/offscreen -B build/linux-debug -DCMAKE_BUILD_TYPE=Debug \
  -DENABLE_NVENC=ON -DOFFSCREEN_GPU_TESTS=ON
cmake --build build/linux-debug --parallel
VK_LAYER_SETTINGS_PATH="$PWD/build/vulkan-validation" \
  ctest --test-dir build/linux-debug --output-on-failure
OFFSCREEN_TEST_ENCODER=hardware OFFSCREEN_TEST_NVENC_INPUT=image \
  node standalone/offscreen/tests/browser_test.mjs \
  build/linux-debug/offscreen_renderer build/linux-debug/browser-image
OFFSCREEN_TEST_ENCODER=hardware OFFSCREEN_TEST_NVENC_INPUT=buffer \
  node standalone/offscreen/tests/browser_test.mjs \
  build/linux-debug/offscreen_renderer build/linux-debug/browser-buffer
```

The GPU tests require NVENC and fail if unavailable. Browser tests with `OFFSCREEN_TEST_NVENC_INPUT` also assert the NVENC backend, then check actual decoded video and interaction. Set `OFFSCREEN_TEST_ENCODER=software` to exercise x264 end to end. Chrome must run as a normal user; set `CHROME_BIN` if it is not on PATH.

Development verification on this Apple Silicon host: macOS Vulkan/VideoToolbox regression tests, synchronization validation, and browser tests; x264/FD tests executed locally. The ARM64 Linux Docker image builds with Ubuntu 22.04's packaged CMake and native nvcc 12.4, then renders and encodes 120 frames using llvmpipe/x264 automatic fallback. Explicit hardware mode fails clearly without NVIDIA hardware. NVIDIA GPU interop/encoding and x86-64 container execution still require verification on the target Linux host; no Linux zero-validation-error or 60 FPS hardware result is claimed here.

## Docker on an NVIDIA Linux host

Install a compatible NVIDIA driver and configure Docker using the [NVIDIA Container Toolkit installation guide](https://docs.nvidia.com/datacenter/cloud-native/container-toolkit/latest/install-guide.html). The toolkit provides GPU devices, driver libraries, and Vulkan ICD configuration; do not install a kernel driver inside the image.

From the repository root:

```sh
docker build -t vulkan-webrtc standalone/offscreen
docker run --rm --gpus all --network host vulkan-webrtc \
  --bind 0.0.0.0 --encoder hardware
```

Docker Desktop on macOS can build this Linux image, but it cannot pass an NVIDIA GPU through to run NVENC ([Docker GPU support](https://docs.docker.com/desktop/features/gpu/)). For local browser testing on a Mac, use the native macOS build above, which encodes with VideoToolbox. An Apple Silicon Docker build produces an ARM64 Linux image by default; use `docker build --platform linux/amd64 -t vulkan-webrtc standalone/offscreen` when building for an x86-64 Linux server.

Serve/open `standalone/offscreen/index.html` on the client and connect to `ws://<linux-host>:8080` (use `127.0.0.1` for a local browser). The container runs the signaling server, not an HTTP server. Host networking is intentional for Linux: exposing TCP 8080 alone does not expose the ICE-selected UDP media ports or fix advertised ICE addresses. For other network topologies, configure reachable ICE/TURN infrastructure on both endpoints.

The multi-stage Dockerfile uses CUDA 12.4.1/Ubuntu 22.04, builds Release without a GPU, and runs as UID 10001 under `tini` for signal forwarding. Driver capabilities are `compute,utility,graphics,video`; `NVIDIA_VISIBLE_DEVICES=all` is set. Select individual GPUs with Docker's `--gpus` option. The runtime includes the Vulkan loader, Mesa fallback, OpenSSL, and x264. Use `--encoder software` to verify fallback explicitly; use `--nvenc-input buffer` to test the mapped-buffer path. The image contains no authentication/TLS termination: restrict network access or place signaling behind authenticated WSS before public deployment.

## Portable snapshot build

The portable Vulkan renderer remains available without the Apple encoder:

```sh
cmake -S standalone/offscreen -B build/offscreen -DOFFSCREEN_STREAMING=OFF -DCMAKE_BUILD_TYPE=Release
cmake --build build/offscreen --parallel
./build/offscreen/offscreen_renderer --snapshot
ctest --test-dir build/offscreen --output-on-failure
```

This build has no encoder, CUDA, or WebRTC dependency; it requires only Vulkan and the C++ toolchain.
