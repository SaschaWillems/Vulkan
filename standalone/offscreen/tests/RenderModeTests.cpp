#include "RenderMode.h"
#include <iostream>
#include <stdexcept>
#include <thread>
#include <vector>
namespace {
void Require(bool value, const char* reason) {
    if (!value)
        throw std::runtime_error(reason);
}
} // namespace
int main() {
    try {
        Require(ParseRenderMode("raster") == RenderMode::Raster, "Parse raster");
        Require(ParseRenderMode("hybrid") == RenderMode::Hybrid, "Parse hybrid");
        Require(ParseRenderMode("pathtrace") == RenderMode::PathTraced, "Parse pathtrace");
        Require(!ParseRenderMode("unknown"), "Accepted unknown mode");
        RenderControls modes;
        Require(!modes.Set(RenderMode::Hybrid), "Unsupported hybrid accepted");
        Require(!modes.Set(RenderMode::PathTraced), "Unsupported path tracing accepted");
        Require(modes.Load().mode == RenderMode::Raster, "Rejected mode changed state");
        modes.Configure(true, RenderMode::Raster);
        CameraState camera;
        ProgressiveHistory history;
        Require(history.Begin(camera.Load(), modes.Load(), false) == 0, "Initial history not empty");
        history.Complete();
        Require(history.Samples() == 0, "Raster accumulated");
        modes.Set(RenderMode::PathTraced);
        for (uint32_t i = 0; i < 8; ++i) {
            Require(history.Begin(camera.Load(), modes.Load(), false) == i, "Stationary count mismatch");
            history.Complete();
        }
        camera.Rotate(0.1, 0.1);
        Require(history.Begin(camera.Load(), modes.Load(), false) == 0, "Orbit reset failed");
        history.Complete();
        camera.Pan(0.1, 0.1);
        Require(history.Begin(camera.Load(), modes.Load(), false) == 0, "Pan reset failed");
        history.Complete();
        camera.Zoom(0.1);
        Require(history.Begin(camera.Load(), modes.Load(), false) == 0, "Zoom reset failed");
        history.Complete();
        camera.Reset();
        Require(history.Begin(camera.Load(), modes.Load(), false) == 0, "Reset failed");
        history.Complete();
        camera.Rotate(0.1, 0);
        camera.Rotate(-0.1, 0);
        Require(history.Begin(camera.Load(), modes.Load(), false) == 0, "Move-and-return retained samples");
        history.Complete();
        modes.Set(RenderMode::Hybrid);
        modes.Set(RenderMode::PathTraced);
        Require(history.Begin(camera.Load(), modes.Load(), false) == 0,
                "Intermediate switch retained history");
        history.Complete();
        Require(history.Begin(camera.Load(), modes.Load(), true) == 0, "Animation retained history");
        history.Complete();
        Require(history.Begin(camera.Load(), modes.Load(), true) == 0, "Animation retained second frame");
        modes.Set(RenderMode::Raster);
        history.Begin(camera.Load(), modes.Load(), false);
        history.Complete();
        Require(history.Samples() == 0, "Leaving path mode retained history");
        std::vector<std::jthread> writers;
        for (int i = 0; i < 4; ++i)
            writers.emplace_back([&] {
                for (int j = 0; j < 1000; ++j) {
                    modes.Set(RenderMode::Hybrid);
                    modes.Set(RenderMode::PathTraced);
                    modes.Set(RenderMode::Raster);
                    Require(static_cast<uint32_t>(modes.Load().mode) <= 2, "Torn mode state");
                }
            });
        writers.clear();
        std::cout << "Mode capabilities, coherent state, accumulation/reset history passed\n";
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
