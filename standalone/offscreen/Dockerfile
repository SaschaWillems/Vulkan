# syntax=docker/dockerfile:1
FROM nvidia/cuda:12.4.1-devel-ubuntu22.04 AS build
ENV DEBIAN_FRONTEND=noninteractive
RUN apt-get update && apt-get install -y --no-install-recommends \
    ca-certificates build-essential cmake ninja-build git pkg-config \
    libvulkan-dev vulkan-tools mesa-vulkan-drivers glslang-tools libssl-dev libx264-dev \
    && rm -rf /var/lib/apt/lists/*
WORKDIR /src
COPY . .
ARG CUDA_ARCHITECTURES="52;75;86;89"
RUN cmake -S . -B /build -G Ninja -DCMAKE_BUILD_TYPE=Release \
      -DOFFSCREEN_STREAMING=ON -DENABLE_NVENC=ON -DBUILD_TESTING=OFF \
      "-DCMAKE_CUDA_ARCHITECTURES=${CUDA_ARCHITECTURES}" \
    && cmake --build /build --parallel 4

FROM nvidia/cuda:12.4.1-base-ubuntu22.04 AS runtime
ENV NVIDIA_DRIVER_CAPABILITIES=compute,utility,graphics,video \
    NVIDIA_VISIBLE_DEVICES=all
RUN apt-get update && apt-get install -y --no-install-recommends \
    ca-certificates libvulkan1 vulkan-tools mesa-vulkan-drivers libssl3 libx264-163 tini \
    && rm -rf /var/lib/apt/lists/* \
    && groupadd --gid 10001 renderer \
    && useradd --uid 10001 --gid renderer --create-home renderer
WORKDIR /opt/offscreen
COPY --from=build /build/offscreen_renderer ./offscreen_renderer
COPY --from=build /src/index.html /src/README.md ./
USER renderer
EXPOSE 8080/tcp
STOPSIGNAL SIGTERM
ENTRYPOINT ["/usr/bin/tini", "--", "/opt/offscreen/offscreen_renderer"]
CMD ["--bind", "0.0.0.0"]
