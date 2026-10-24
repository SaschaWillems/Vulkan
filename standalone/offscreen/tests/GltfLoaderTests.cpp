#include "GltfLoader.h"
#include <cmath>
#include <iostream>
#include <stdexcept>

namespace {
void Require(bool test, const char* message) {
    if (!test)
        throw std::runtime_error(message);
}
bool Near(glm::vec3 a, glm::vec3 b) { return glm::length(a - b) < 0.0001f; }
} // namespace
int main(int argc, char** argv) {
    try {
        if (argc != 2)
            throw std::runtime_error("Expected fixture directory");
        const std::filesystem::path directory(argv[1]);
        for (const auto* name :
             {"hierarchy.gltf", "hierarchy.glb", "indices32.glb", "embedded.gltf", "sparse.gltf"}) {
            const auto scene = LoadGltf(directory / name);
            Require(scene.vertices.size() == 6 && scene.indices.size() == 6 && scene.draws.size() == 2,
                    "Mesh or primitive ingestion failed");
            Require(Near(scene.boundsMin, {-3, -1, 0}) && Near(scene.boundsMax, {5, 4, 4}),
                    "Node TRS/matrix hierarchy was not applied");
            Require(scene.images.at(0).width == 2 && scene.images[0].height == 2, "PNG was not decoded");
            Require(scene.images[0].rgba[0] == 255 && scene.images[0].rgba[1] == 64,
                    "Image channel order changed");
            Require(scene.materials[0].baseTexture && scene.materials[0].metallicRoughnessTexture,
                    "PBR texture references lost");
            Require(std::abs(scene.materials[0].metallic - 0.3f) < 0.0001f &&
                        std::abs(scene.materials[0].roughness - 0.7f) < 0.0001f,
                    "PBR factors lost");
            Require(scene.vertices[0].color.r == 1 && scene.vertices[0].color.g > 0.49f &&
                        scene.vertices[0].color.g < 0.51f,
                    "Normalized vertex colors were not read");
            for (const auto& vertex : scene.vertices)
                Require(Near(vertex.normal, {0, 0, 1}), "Missing normals were not generated");
            Require(scene.textures[0].wrapU == VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE &&
                        scene.textures[0].wrapV == VK_SAMPLER_ADDRESS_MODE_MIRRORED_REPEAT,
                    "Sampler wrap modes lost");
        }
        const auto missing = LoadGltf(directory / "missing-image.gltf");
        Require(missing.images[0].width == 1 && missing.images[0].rgba[0] == 255,
                "Missing-image fallback failed");
        const auto interleaved = LoadGltf(directory / "interleaved.gltf");
        Require(Near(interleaved.vertices[0].normal, {0, 1, 0}) &&
                    Near(interleaved.vertices[3].normal, {0, 0, 1}),
                "Interleaved attributes/authored normals were not read");
        const auto transformed = LoadGltf(directory / "uv-transform.gltf");
        Require(glm::length(transformed.vertices[0].uv - glm::vec2(0.1f, 0.95f)) < 0.0001f &&
                    glm::length(transformed.vertices[0].mrUv - glm::vec2(0, 1)) < 0.0001f,
                "Independent UV sets/texture transforms failed");
        Require(LoadGltf(directory / "tiny-scale.gltf").draws.size() == 1,
                "Valid small-scale node was discarded");
        const auto alpha = LoadGltf(directory / "alpha.gltf");
        Require(alpha.materials[0].alphaMode == 1 && alpha.materials[1].alphaMode == 2 &&
                    alpha.materials[0].alphaCutoff == 0.25f,
                "Alpha modes were not parsed");
        for (const auto* name :
             {"invalid.gltf", "truncated-accessor.gltf", "cycle.gltf", "bad-index.gltf", "absent.glb"}) {
            bool rejected = false;
            try {
                (void)LoadGltf(directory / name);
            } catch (const std::exception&) {
                rejected = true;
            }
            Require(rejected, "Malformed glTF was accepted");
        }
        const auto box = MakeFallbackBox();
        Require(box.vertices.size() == 24 && box.indices.size() == 36 && box.draws.size() == 1,
                "Generated fallback box failed");
        std::cout << "glTF/GLB buffers, indices, sparse accessors, PBR textures, hierarchy, normals, and "
                     "error handling passed\n";
        return 0;
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
