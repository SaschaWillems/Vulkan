# glTF PBR → Vulkan → NVENC / VideoToolbox / x264 → WebRTC

A standalone C++20 server that renders glTF 2.0/GLB models with a metallic-roughness PBR pipeline into a configurable 2K–8K RGBA Vulkan image and streams it to a browser at a target of 60 FPS. The default is 2560×1440 at 16 Mbps; 2K/4K use H.264 and 8K automatically selects AV1. Linux uses NVIDIA NVENC with GPU-only framebuffer capture when available, falling back to x264 with host readback for H.264. AV1 requires AV1-capable NVENC hardware. macOS uses VideoToolbox hardware encoding with host readback. Dragging the video orbits the shared camera; Shift/right-drag pans it. Without `--model`, a generated colored box is used. No windowing library, surface, or swapchain is used.

## Build on macOS

Install a Vulkan SDK with MoltenVK and `glslangValidator`, recent Xcode command-line tools with C++20 support, CMake 3.21+, Git, and OpenSSL. Python 3 is needed when tests are enabled (the default). If the SDK is not installed globally, source its `setup-env.sh` first. For Homebrew installations:

```sh
brew install cmake openssl@3
cmake -S standalone/offscreen -B build/webrtc -DCMAKE_BUILD_TYPE=Release \
  -DOPENSSL_ROOT_DIR="$(brew --prefix openssl@3)"
cmake --build build/webrtc --parallel
./build/webrtc/offscreen_renderer
```

CMake FetchContent downloads and builds **paullouisageneau/libdatachannel v0.24.5**, pinned to commit `443f6934d9007eb7076ab7825ba330f355fcbead`, including its submodules. The `paullouisdefaye/libdatachannel` URL in the original request does not exist. The JSON dependency comes from libdatachannel's pinned submodule. FetchContent also pins fastgltf v0.8.0, VMA v3.2.1, GLM 1.0.1, and stb for PNG/JPEG decoding. A network connection is needed for the first configure. `glslangValidator` compiles `shaders/pbr.vert` and `shaders/pbr.frag` during the build; CMake embeds the resulting SPIR-V in the executable, so there are no runtime shader-file or shader-compiler dependencies.

Open `standalone/offscreen/index.html` (also copied into the build directory), leave `ws://127.0.0.1:8080` as the signaling address, and click **Connect**. If preferred, serve the client locally:

```sh
python3 -m http.server 8000 --bind 127.0.0.1 --directory standalone/offscreen
# Open http://127.0.0.1:8000
```

The browser displays the received FPS/bitrate and confirms camera updates. Drag to orbit, Shift-drag or right-drag to pan, scroll over the video to zoom, or click **Reset View** to restore the default camera. Zoom distance is bounded to 1.5–12 scene units; reset restores yaw/pitch/target to zero and distance to 3. Ctrl+C or SIGTERM stops the server and releases its resources.

For local viewer development, keep the native server running and serve `standalone/offscreen` with the HTTP command above. Refresh the browser after HTML/CSS/JavaScript edits. For C++ changes, run `cmake --build build/webrtc --parallel 4` and restart the server. New camera commands require the updated server binary, including when connecting to RunPod.

Debug builds require `VK_LAYER_KHRONOS_validation` and `VK_EXT_debug_utils`. Vulkan 1.1 satisfies the portability-subset properties2 dependency; portability enumeration and the device portability subset are enabled when advertised.

## Build on Linux

Install a Vulkan-capable driver, CMake 3.21+, a C++20 compiler, OpenSSL, and x264 development packages. For Ubuntu 22.04:

```sh
sudo apt-get update
sudo apt-get install build-essential cmake ninja-build git pkg-config \
  libvulkan-dev vulkan-tools mesa-vulkan-drivers glslang-tools libssl-dev libx264-dev python3
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

## Load a model

```sh
# Run from the repository root; relative model paths resolve from the current directory.
./build/webrtc/offscreen_renderer --model path/to/model.glb --resolution 2k --fps 30
# On the tested M4 Pro, a textured 4K model decodes at about 18 FPS:
./build/webrtc/offscreen_renderer --model path/to/model.glb --resolution 4k --fps 20
./build/linux-nvenc/offscreen_renderer --model path/to/model.gltf \
  --bind 0.0.0.0 --port 8080 --encoder hardware
# Offline fallback box, or opt-in rotation:
./build/webrtc/offscreen_renderer
./build/webrtc/offscreen_renderer --model path/to/model.glb --spin
```

Models are static by default so camera gestures remain predictable. `--spin` enables continuous model rotation. The selected glTF scene (default scene, otherwise first scene) is traversed with composed parent/child TRS or matrix transforms. Geometry is centered and uniformly fitted to a unit bounding sphere, preserving shape and relative node placement. Camera coordinates operate in this normalized scene space. Reset View restores the centered view.

The loader handles external buffers/images, GLB bufferView images, embedded data URIs, interleaved and sparse accessors, normalized vertex colors, optional normals/UVs/colors, and 8/16/32-bit unsigned indices (uploaded as 32-bit). Triangle lists, strips, and fans are supported. Missing normals are generated from area-weighted faces; missing colors default to white. Missing/unreadable PNG/JPEG images log a warning and use white textures, preserving material factors and vertex colors. Malformed geometry, cyclic hierarchies, missing model/buffer files, and unsupported required extensions fail explicitly.

Base-color textures use sRGB sampling, while metallic-roughness textures use linear sampling with G=roughness and B=metallic. Independent UV sets, `KHR_texture_transform`, sampler filters/wrap modes, and GPU-generated mipmaps are supported. PBR shading uses GGX distribution, Smith-Schlick geometry, Schlick Fresnel, directional illumination, ambient fill, Reinhard tone mapping, and gamma encoding. Opaque/masked materials write depth; blended primitives are sorted back-to-front by their transformed bounds centers and do not write depth.

This is a static metallic-roughness viewer: animation, skinning deformation, morph weights, Draco/meshopt geometry decoding, KTX2/Basis textures, normal-map shading, occlusion maps, emissive maps, and image-based lighting are not implemented. Skinned meshes use base vertex positions and log a warning. Unsupported required extensions are rejected. Normal/occlusion/emissive textures present in an otherwise supported asset do not contribute to this basic material shader. Textures are limited to PNG/JPEG, at most 16384 pixels per axis and 64 megapixels per image; unsupported images use the material fallback.

The fallback asset and generated test fixtures require no downloads at runtime. A real Damaged Helmet GLB from [Khronos glTF Sample Assets](https://github.com/KhronosGroup/glTF-Sample-Assets/tree/main/Models/DamagedHelmet) was also used for development verification; it is not bundled with the application.

## Resolution and codec

The server selects the source resolution at startup. All viewers receive that resolution; the browser scales the displayed video to the available space and reports the received pixel dimensions in its statistics. Here “2K” means QHD (2560×1440).

| Preset | Source pixels | Default bitrate | Automatic codec |
| --- | --- | --- | --- |
| `2k` (default/minimum) | 2560×1440 | 16 Mbps | H.264 |
| `4k` | 3840×2160 | 40 Mbps | H.264 |
| `8k` (maximum) | 7680×4320 | 100 Mbps | AV1 |

```sh
./build/webrtc/offscreen_renderer --resolution 2k
./build/webrtc/offscreen_renderer --resolution 4k --fps 30
# Linux on an AV1-capable NVIDIA GPU, such as the RunPod L4:
./build/linux-nvenc/offscreen_renderer --resolution 8k --encoder hardware \
  --ice-server stun:stun.l.google.com:19302
```

`--fps` accepts 1–60 (default 60). `--bitrate` overrides the preset in bits/second, from 1000 to 100000000. `--codec auto|h264|av1` overrides codec selection. The default resolution cannot be reduced below 2K through the CLI; encoder unit tests retain small frame sizes for fast regression checks. Snapshot and render-test modes support all three resolutions without requiring a video encoder.

NVIDIA's Ada/L4 H.264 encoder is limited to 4096×4096, while its AV1 encoder supports up to 8192×8192 ([NVIDIA format limits](https://developer.nvidia.com/video-codec-sdk)). Therefore 8K selects AV1, imports the same Vulkan memory into CUDA, and sends low-overhead AV1 OBUs through libdatachannel's AV1 RTP packetizer. NVENC's width/height limits are queried before initialization. Unsupported codecs/resolutions fail explicitly; resolution is never silently reduced. Automatic Linux x264 fallback applies to H.264; AV1 has no CPU fallback. Explicit `--codec h264` can use x264 at 8K, but browser H.264 decoding limits may prevent playback.

macOS uses VideoToolbox H.264 at resolutions supported by its hardware (2K/4K on the tested M4 Pro); AV1 encoding is not implemented on macOS. Automatic 8K streaming therefore requires Linux/AV1 NVENC and an AV1-capable browser. Higher resolutions do not guarantee 60 FPS: capture, encoding, browser decoding, and network throughput must all keep up. Use `--fps 30` when needed. The Mac host readback path is distinct from the Linux NVENC path that keeps raw frames on the GPU.

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

- `main.cpp`: Vulkan instance/device, MoltenVK portability, color/depth render pass, framebuffer, capture/readback, frame pacing, and `--model`/`--spin` CLI integration. Command buffers are reset/re-recorded after the preceding frame fence so each draw receives its current push constants.
- `GltfLoader.h/.cpp`: fastgltf ingestion, scene validation, node transforms, materials/images, generated normals, fallback box, and VMA vertex/index buffers uploaded through staging buffers.
- `PbrRenderer.h/.cpp`: VMA allocator, geometry/texture uploads, depth image, mip generation, material/lighting descriptors, opaque/blended pipelines, and 128-byte per-draw model/view-projection push constants. Model resources are initialized once; only camera/lighting data and draw commands change per frame.
- `shaders/pbr.vert`, `shaders/pbr.frag`: build-time GLSL sources for world-space geometry/normal transforms and Cook-Torrance metallic-roughness shading. `shaders/embed.cmake` embeds SPIR-V; `Vma.cpp` and `StbImage.cpp` isolate third-party implementations.
- `EncoderFactory.h/.cpp`: platform/backend selection, explicit hardware mode, and Linux CPU fallback. The original `IVideoEncoder::Init`/`EncodeFrame` contract is preserved; GPU backends additionally implement `UsesGpuFrames`/`EncodeGpuFrame` with a bound Vulkan frame source.
- `IVideoEncoder.h`, `VideoToolboxEncoder.h/.mm`: RAII hardware compression session and pixel-buffer pool. RGBA is swizzled to BGRA while respecting CoreVideo row stride. B frames are disabled. The synchronous interface drains each submitted frame before returning its Annex-B access unit. SPS/PPS accompany each IDR; keyframes occur every second or on request. Unsupported hardware encoding fails explicitly.
- `VulkanMemoryExporter.h/.cpp`, `UniqueFd.h`: dedicated device-local memory with opaque-FD export, exportable binary semaphores, device UUID, and FD ownership.
- `CudaVulkanInterop.h/.cpp`, `CudaColorConversion.cu`: CUDA imports, GPU synchronization, and BT.709 limited-range RGBA → NV12 conversion into a pitched CUDA allocation. `ColorConversion.h` shares the conversion coefficients with x264.
- `NvencZeroCopyEncoder.h/.cpp`: runtime-loaded NVENC session, registered CUDA device pointer, H.264 baseline with resolution/frame-rate-dependent levels or AV1 Main, P1 ultra-low-latency tuning, CBR, one-frame VBV, no B frames/lookahead, infinite GOP with an IDR every second or on request. H.264 SPS/PPS and Annex-B access units, or AV1 sequence headers and low-overhead OBUs, feed the matching WebRTC packetizer.
- `X264Encoder.h/.cpp`: Linux CPU fallback using ultrafast/zerolatency, NV12, CBR/VBV, no B frames, and matching H.264 profile/keyframe behavior.
- `WebRTCStreamer.h/.cpp`: WebSocket offer/answer and trickle ICE, bounded signaling queues, per-peer H.264/AV1 RTP packetization, RTCP sender reports, NACK retransmission, and PLI/FIR keyframe requests. Peer creation and teardown are serviced by `Poll()` on the render thread. Callbacks capture weak session references.
- `Camera.h` (`CameraState.h` forwards to it): GLM look-at/perspective orbit camera with target, yaw/pitch, and distance. Immutable camera snapshots are published using shared-pointer atomic operations, including the portable API supported by Apple libc++; render reads observe one coherent pose while callbacks update it. Pan/rotate/zoom updates use compare/exchange and reset publishes the entire default pose. Pitch, distance, and target are bounded; non-finite inputs are rejected. `Scene.h` retains a small GLM MVP helper for tests.
- `index.html`: no external scripts or build tools. It handles asynchronous SDP/ICE ordering, video playback, batched orbit/pan/wheel input, reset, statistics, disconnection, and manual reconnection. Controls enable when the DataChannel opens. The server sends camera state on connection and after accepted commands.

The `controls` DataChannel accepts `{"type":"rotate","dx":0.1,"dy":-0.1}`, `{"type":"pan","dx":0.1,"dy":-0.1}`, `{"type":"zoom","delta":-0.2}`, and `{"type":"reset"}`. Positive zoom deltas increase distance; negative deltas zoom in, with distance multiplied by `exp(delta)` after bounding the delta to [-1,1]. Orbit deltas are fractions of video width/height. Pan deltas are fractions of video height on both axes and translate the target along camera right/up vectors. Telemetry has the form `{"type":"camera","yaw":0,"pitch":0,"distance":3,"target":[0,0,0]}`. Reset also cancels unsent browser input so a pending wheel event cannot immediately undo it.

**GPU path:** the default exports the actual optimal-tiled color image and imports it as a CUDA mipmapped array/surface. An optimal-tiled image cannot be treated as a linear CUDA buffer. `--nvenc-input buffer` instead copies the image into an exported device-local VkBuffer entirely on the GPU and imports that allocation with `cudaExternalMemoryGetMappedBuffer`. Vulkan image-export failure also tries the buffer path. Both paths convert to NV12 in VRAM and register that CUDA pointer with NVENC; neither allocates or reads a CPU framebuffer. The compressed video bitstream is copied to CPU memory for libdatachannel. GPU conversion and the optional GPU buffer copy still occur: “zero-copy” here means zero raw-frame **host** copies.

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

CTest verifies glTF/GLB ingestion, UV/normal/color attributes, sparse/interleaved accessors, hierarchy transforms, malformed inputs, missing-image fallback, alpha modes, actual PBR texture/factor influence, depth occlusion, and 2K PPM dimensions/pixels, full 8K rendering/readback and orbit/zoom, video presets/H.264 levels, AV1 OBU packetization, file-open errors, animation and orbit changes, uniform/framebuffer reuse, malformed controls, concurrent camera updates, hardware encoding, SPS/PPS/IDR/P-frame output, forced keyframes, encoder reinitialization, and a paced 120-frame render/encode run. The browser test uses an isolated temporary Chrome profile and local ports. It checks actual decoding at the requested source resolution, frame rate, decoded pixels, orbit/pan/zoom/reset acknowledgements, reconnection, validation output, and graceful shutdown. It writes `browser-test.png` and `browser-test-server.log` into the chosen artifact directory. Set `OFFSCREEN_TEST_MODEL=/absolute/path/model.glb` to test a loaded model instead of the fallback box. Set `CHROME_BIN` to override the browser executable. Browser correctness tests target 30 FPS by default; `OFFSCREEN_TEST_FPS=60` opts into the 60 FPS throughput check. Set `OFFSCREEN_TEST_RESOLUTION=4k` or `8k` to test those source sizes; 8K requires an AV1 NVENC server. `OFFSCREEN_TEST_CODEC` optionally overrides the codec. For example:

```sh
OFFSCREEN_TEST_RESOLUTION=4k node standalone/offscreen/tests/browser_test.mjs \
  build/webrtc/offscreen_renderer build/webrtc/browser-4k
OFFSCREEN_TEST_ENCODER=hardware OFFSCREEN_TEST_RESOLUTION=8k \
  node standalone/offscreen/tests/browser_test.mjs \
  build/linux-nvenc/offscreen_renderer build/linux-nvenc/browser-8k
```

The 8K AV1 encoder and browser path must be runtime-verified on the target NVIDIA host; local packetization tests and Docker compilation do not establish NVENC runtime correctness.

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

Development verification on this Apple Silicon host: all nine macOS CTests and all seven non-streaming CTests pass with synchronization validation, including glTF ingestion, PBR pixels/depth, alpha rendering, camera controls, readback, and VideoToolbox encoding. Browser tests pass with both the fallback box and a textured Damaged Helmet GLB at 2K, with the Release model stream decoding at approximately 30 FPS for a 30 FPS target. At 4K, the textured model achieves approximately 18 FPS: it fails the 30 FPS throughput check and passes the 20 FPS target check. Orbit, pan, zoom, reset, and reconnect pass at both resolutions. These MoltenVK validation runs report no Vulkan validation errors. Full 8K render/readback checks also pass; 60 FPS throughput is not guaranteed.

The ARM64 Linux Docker image builds with Ubuntu 22.04's packaged CMake and native nvcc 12.4. Textured GLB and alpha-material render tests pass under Linux llvmpipe with the Khronos validation layer and synchronization validation enabled, with no validation errors. Linux x264 fallback has also been smoke-tested. The updated PBR renderer still requires runtime verification with NVIDIA Vulkan/CUDA/NVENC on the target host; local software rendering does not establish NVIDIA interop correctness. The previously working RunPod H.264 path, new 8K AV1 path, and x86-64 container should be retested after deploying this update.

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

Mount models into the runtime container; GLTF external buffers and images must remain alongside their model:

```sh
docker run --rm --gpus all --network host -v "$PWD/assets:/assets:ro" vulkan-webrtc \
  --model /assets/model.glb --bind 0.0.0.0 --encoder hardware
```

The multi-stage Dockerfile uses CUDA 12.4.1/Ubuntu 22.04, builds Release without a GPU, and runs as UID 10001 under `tini` for signal forwarding. Driver capabilities are `compute,utility,graphics,video`; `NVIDIA_VISIBLE_DEVICES=all` is set. Select individual GPUs with Docker's `--gpus` option. The runtime includes the Vulkan loader, Mesa fallback, OpenSSL, and x264. Use `--encoder software` to verify fallback explicitly; use `--nvenc-input buffer` to test the mapped-buffer path. The image contains no authentication/TLS termination: restrict network access or place signaling behind authenticated WSS before public deployment.

## Portable snapshot build

The portable Vulkan renderer remains available without the Apple encoder:

```sh
cmake -S standalone/offscreen -B build/offscreen -DOFFSCREEN_STREAMING=OFF -DCMAKE_BUILD_TYPE=Release
cmake --build build/offscreen --parallel
./build/offscreen/offscreen_renderer --snapshot
ctest --test-dir build/offscreen --output-on-failure
```

This build includes the same fastgltf/VMA/GLM PBR scene pipeline and build-time shader compilation, with no encoder, CUDA, or WebRTC dependency.
