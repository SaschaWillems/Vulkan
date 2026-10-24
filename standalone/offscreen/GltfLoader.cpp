#include "GltfLoader.h"
#include <fastgltf/core.hpp>
#include <fastgltf/tools.hpp>
#include <fastgltf/glm_element_traits.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <stb_image.h>
#include <algorithm>
#include <cmath>
#include <cstring>
#include <fstream>
#include <functional>
#include <iostream>
#include <limits>
#include <memory>
#include <numeric>
#include <stdexcept>
#include <utility>

namespace {
void Check(VkResult result, const char* operation) {
    if (result != VK_SUCCESS)
        throw std::runtime_error(std::string(operation) + " (VkResult " + std::to_string(result) + ")");
}
template <typename V> bool Finite(V value) {
    for (glm::length_t i = 0; i < value.length(); ++i)
        if (!std::isfinite(value[i]))
            return false;
    return true;
}
std::span<const std::byte> Bytes(const fastgltf::DataSource& source) {
    return std::visit(
        [](const auto& data) -> std::span<const std::byte> {
            if constexpr (requires {
                              data.bytes.data();
                              data.bytes.size();
                          })
                return {data.bytes.data(), data.bytes.size()};
            else
                throw std::runtime_error("glTF buffer is not loaded into memory");
        },
        source);
}
std::span<const std::byte> View(const fastgltf::Asset& asset, size_t index) {
    const auto& view = asset.bufferViews.at(index);
    const auto bytes = Bytes(asset.buffers.at(view.bufferIndex).data);
    if (view.byteOffset > bytes.size() || view.byteLength > bytes.size() - view.byteOffset)
        throw std::runtime_error("glTF bufferView exceeds its buffer");
    return bytes.subspan(view.byteOffset, view.byteLength);
}
void Range(size_t length, size_t offset, size_t count, size_t stride, size_t element) {
    if (!stride || !element || offset > length ||
        (count && (element > length - offset || count - 1 > (length - offset - element) / stride)))
        throw std::runtime_error("glTF accessor exceeds its bufferView");
}
void ValidateAccessor(const fastgltf::Asset& asset, const fastgltf::Accessor& accessor) {
    const auto element = fastgltf::getElementByteSize(accessor.type, accessor.componentType);
    if (accessor.bufferViewIndex) {
        const auto index = *accessor.bufferViewIndex;
        Range(View(asset, index).size(), accessor.byteOffset, accessor.count,
              asset.bufferViews.at(index).byteStride.value_or(element), element);
    } else if (!accessor.sparse)
        throw std::runtime_error("glTF accessor has no data");
    if (accessor.sparse) {
        const auto& sparse = *accessor.sparse;
        if (sparse.count > accessor.count)
            throw std::runtime_error("Invalid sparse accessor count");
        if (sparse.indexComponentType != fastgltf::ComponentType::UnsignedByte &&
            sparse.indexComponentType != fastgltf::ComponentType::UnsignedShort &&
            sparse.indexComponentType != fastgltf::ComponentType::UnsignedInt)
            throw std::runtime_error("Invalid sparse index component type");
        const auto indexSize = fastgltf::getComponentByteSize(sparse.indexComponentType);
        const auto indices = View(asset, sparse.indicesBufferView);
        Range(indices.size(), sparse.indicesByteOffset, sparse.count, indexSize, indexSize);
        Range(View(asset, sparse.valuesBufferView).size(), sparse.valuesByteOffset, sparse.count, element,
              element);
        uint32_t previous = 0;
        for (size_t i = 0; i < sparse.count; ++i) {
            uint32_t index = 0;
            if (indexSize != 1 && indexSize != 2 && indexSize != 4)
                throw std::runtime_error("Invalid sparse index type");
            std::memcpy(&index, indices.data() + sparse.indicesByteOffset + i * indexSize, indexSize);
            if (index >= accessor.count || (i && index <= previous))
                throw std::runtime_error("Invalid sparse indices");
            previous = index;
        }
    }
}
template <typename T, typename F>
void Attribute(const fastgltf::Asset& asset, const fastgltf::Primitive& primitive, std::string_view name,
               size_t count, F apply) {
    const auto attribute = primitive.findAttribute(name);
    if (attribute == primitive.attributes.end())
        return;
    const auto& accessor = asset.accessors.at(attribute->accessorIndex);
    if (accessor.count != count || accessor.type != fastgltf::ElementTraits<T>::type)
        throw std::runtime_error("Invalid glTF attribute " + std::string(name));
    ValidateAccessor(asset, accessor);
    fastgltf::iterateAccessorWithIndex<T>(asset, accessor, [&](T value, size_t index) {
        if (!Finite(value))
            throw std::runtime_error("Non-finite glTF vertex attribute");
        apply(value, index);
    });
}
GltfImage DecodeImage(const fastgltf::Asset& asset, const fastgltf::Image& image,
                      const std::filesystem::path& directory) {
    try {
        std::vector<std::byte> file;
        std::span<const std::byte> bytes;
        if (const auto* view = std::get_if<fastgltf::sources::BufferView>(&image.data))
            bytes = View(asset, view->bufferViewIndex);
        else if (const auto* uri = std::get_if<fastgltf::sources::URI>(&image.data)) {
            if (!uri->uri.isLocalPath())
                throw std::runtime_error("Only local image URIs are supported");
            std::ifstream stream(directory / uri->uri.fspath(), std::ios::binary | std::ios::ate);
            if (!stream)
                throw std::runtime_error("Image file is missing");
            const auto length = stream.tellg();
            if (length < 0 || static_cast<uint64_t>(length) > 256 * 1024 * 1024 ||
                uri->fileByteOffset > static_cast<uint64_t>(length))
                throw std::runtime_error("Invalid image file size");
            file.resize(static_cast<size_t>(length) - uri->fileByteOffset);
            stream.seekg(static_cast<std::streamoff>(uri->fileByteOffset));
            if (!stream.read(reinterpret_cast<char*>(file.data()), static_cast<std::streamsize>(file.size())))
                throw std::runtime_error("Cannot read image file");
            bytes = file;
        } else
            bytes = Bytes(image.data);
        if (bytes.empty() || bytes.size() > std::numeric_limits<int>::max())
            throw std::runtime_error("Invalid image payload size");
        int width = 0, height = 0, components = 0;
        const auto* data = reinterpret_cast<const stbi_uc*>(bytes.data());
        if (!stbi_info_from_memory(data, static_cast<int>(bytes.size()), &width, &height, &components) ||
            width <= 0 || height <= 0 || width > 16384 || height > 16384 ||
            uint64_t(width) * uint64_t(height) > 67108864)
            throw std::runtime_error("Unsupported or oversized PNG/JPEG image");
        std::unique_ptr<stbi_uc, decltype(&stbi_image_free)> decoded(
            stbi_load_from_memory(data, static_cast<int>(bytes.size()), &width, &height, &components, 4),
            stbi_image_free);
        if (!decoded)
            throw std::runtime_error("PNG/JPEG decoding failed");
        GltfImage result;
        result.width = static_cast<uint32_t>(width);
        result.height = static_cast<uint32_t>(height);
        result.rgba.assign(decoded.get(),
                           decoded.get() + static_cast<size_t>(width) * static_cast<size_t>(height) * 4);
        return result;
    } catch (const std::exception& error) {
        std::cerr << "Texture fallback: " << error.what() << '\n';
        return {};
    }
}
VkSamplerAddressMode Wrap(fastgltf::Wrap wrap) {
    switch (wrap) {
    case fastgltf::Wrap::ClampToEdge:
        return VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
    case fastgltf::Wrap::MirroredRepeat:
        return VK_SAMPLER_ADDRESS_MODE_MIRRORED_REPEAT;
    default:
        return VK_SAMPLER_ADDRESS_MODE_REPEAT;
    }
}
void Fit(GltfScene& scene) {
    if (scene.draws.empty())
        throw std::runtime_error("glTF scene contains no renderable triangles");
    glm::vec3 minimum(std::numeric_limits<float>::max()), maximum(-std::numeric_limits<float>::max());
    for (const auto& draw : scene.draws)
        for (uint32_t i = 0; i < draw.indexCount; ++i) {
            const auto position =
                glm::vec3(draw.transform *
                          glm::vec4(scene.vertices.at(scene.indices.at(draw.firstIndex + i)).position, 1));
            if (!Finite(position))
                throw std::runtime_error("Non-finite glTF world position");
            minimum = glm::min(minimum, position);
            maximum = glm::max(maximum, position);
        }
    scene.boundsMin = minimum;
    scene.boundsMax = maximum;
    const double radius = glm::length(glm::dvec3(maximum) - glm::dvec3(minimum)) * 0.5;
    const float scale = static_cast<float>(1 / radius);
    if (!std::isfinite(radius) || radius <= 0 || !std::isfinite(scale) || scale <= 0)
        throw std::runtime_error("glTF scene cannot be normalized to a finite extent");
    scene.normalization =
        glm::scale(glm::mat4(1), glm::vec3(scale)) *
        glm::translate(glm::mat4(1), -glm::vec3((glm::dvec3(minimum) + glm::dvec3(maximum)) * 0.5));
}
} // namespace

GltfScene LoadGltf(const std::filesystem::path& path) {
    fastgltf::Parser parser(fastgltf::Extensions::KHR_mesh_quantization |
                            fastgltf::Extensions::KHR_texture_transform);
    auto data = fastgltf::GltfDataBuffer::FromPath(path);
    if (!data)
        throw std::runtime_error("Cannot open glTF: " + path.string());
    auto parsed = parser.loadGltf(data.get(), path.parent_path(), fastgltf::Options::LoadExternalBuffers);
    if (!parsed)
        throw std::runtime_error("glTF parsing: " + std::string(fastgltf::getErrorMessage(parsed.error())));
    auto asset = std::move(parsed.get());
    if (const auto error = fastgltf::validate(asset); error != fastgltf::Error::None)
        throw std::runtime_error("glTF validation: " + std::string(fastgltf::getErrorMessage(error)));
    for (size_t i = 0; i < asset.bufferViews.size(); ++i)
        (void)View(asset, i);
    GltfScene out;
    for (const auto& image : asset.images)
        out.images.push_back(DecodeImage(asset, image, path.parent_path()));
    for (const auto& texture : asset.textures) {
        GltfTexture result;
        if (!texture.imageIndex || *texture.imageIndex >= out.images.size())
            throw std::runtime_error("Invalid glTF texture image");
        result.image = *texture.imageIndex;
        if (texture.samplerIndex) {
            const auto& sampler = asset.samplers.at(*texture.samplerIndex);
            result.mag = sampler.magFilter.value_or(fastgltf::Filter::Linear) == fastgltf::Filter::Nearest
                             ? VK_FILTER_NEAREST
                             : VK_FILTER_LINEAR;
            const auto min = sampler.minFilter.value_or(fastgltf::Filter::LinearMipMapLinear);
            result.min = min == fastgltf::Filter::Nearest || min == fastgltf::Filter::NearestMipMapNearest ||
                                 min == fastgltf::Filter::NearestMipMapLinear
                             ? VK_FILTER_NEAREST
                             : VK_FILTER_LINEAR;
            result.mipmapped = min != fastgltf::Filter::Nearest && min != fastgltf::Filter::Linear;
            result.mipMode =
                min == fastgltf::Filter::NearestMipMapNearest || min == fastgltf::Filter::LinearMipMapNearest
                    ? VK_SAMPLER_MIPMAP_MODE_NEAREST
                    : VK_SAMPLER_MIPMAP_MODE_LINEAR;
            result.wrapU = Wrap(sampler.wrapS);
            result.wrapV = Wrap(sampler.wrapT);
        }
        out.textures.push_back(result);
    }
    for (const auto& material : asset.materials) {
        GltfMaterial result;
        for (int i = 0; i < 4; ++i)
            result.baseColor[i] = material.pbrData.baseColorFactor[static_cast<size_t>(i)];
        result.metallic = material.pbrData.metallicFactor;
        result.roughness = material.pbrData.roughnessFactor;
        result.doubleSided = material.doubleSided;
        result.alphaCutoff = material.alphaCutoff;
        result.alphaMode = static_cast<int>(material.alphaMode);
        if (!Finite(result.baseColor) || !std::isfinite(result.metallic) ||
            !std::isfinite(result.roughness) || !std::isfinite(result.alphaCutoff))
            throw std::runtime_error("Non-finite glTF material");
        if (glm::any(glm::lessThan(result.baseColor, glm::vec4(0))) ||
            glm::any(glm::greaterThan(result.baseColor, glm::vec4(1))) || result.metallic < 0 ||
            result.metallic > 1 || result.roughness < 0 || result.roughness > 1)
            throw std::runtime_error("glTF PBR factor is outside [0,1]");
        if (material.pbrData.baseColorTexture) {
            result.baseTexture = material.pbrData.baseColorTexture->textureIndex;
            result.baseUv = material.pbrData.baseColorTexture->texCoordIndex;
        }
        if (material.pbrData.metallicRoughnessTexture) {
            result.metallicRoughnessTexture = material.pbrData.metallicRoughnessTexture->textureIndex;
            result.mrUv = material.pbrData.metallicRoughnessTexture->texCoordIndex;
        }
        const auto transform = [](const auto& info, size_t& uv, glm::mat3& output) {
            if (!info || !info->transform)
                return;
            const auto& input = *info->transform;
            uv = input.texCoordIndex.value_or(uv);
            const float c = std::cos(input.rotation), s = std::sin(input.rotation);
            output = glm::mat3(glm::vec3(c * input.uvScale[0], s * input.uvScale[0], 0),
                               glm::vec3(-s * input.uvScale[1], c * input.uvScale[1], 0),
                               glm::vec3(input.uvOffset[0], input.uvOffset[1], 1));
            for (int column = 0; column < 3; ++column)
                if (!Finite(output[column]))
                    throw std::runtime_error("Invalid UV transform");
        };
        transform(material.pbrData.baseColorTexture, result.baseUv, result.baseUvTransform);
        transform(material.pbrData.metallicRoughnessTexture, result.mrUv, result.mrUvTransform);
        out.materials.push_back(result);
    }
    const size_t defaultMaterial = out.materials.size();
    out.materials.emplace_back();
    std::vector<std::vector<GltfDraw>> meshes;
    for (const auto& mesh : asset.meshes) {
        std::vector<GltfDraw> draws;
        for (const auto& primitive : mesh.primitives) {
            const auto position = primitive.findAttribute("POSITION");
            if (position == primitive.attributes.end())
                throw std::runtime_error("glTF primitive has no POSITION");
            const size_t count = asset.accessors.at(position->accessorIndex).count;
            if (!count || count > UINT32_MAX - out.vertices.size())
                throw std::runtime_error("Invalid glTF vertex count");
            std::vector<GltfVertex> vertices(count);
            Attribute<glm::vec3>(asset, primitive, "POSITION", count,
                                 [&](auto v, size_t i) { vertices[i].position = v; });
            Attribute<glm::vec3>(asset, primitive, "NORMAL", count,
                                 [&](auto v, size_t i) { vertices[i].normal = v; });
            size_t materialIndex = primitive.materialIndex.value_or(defaultMaterial);
            GltfMaterial material = out.materials.at(materialIndex);
            const auto baseUv = "TEXCOORD_" + std::to_string(material.baseUv),
                       mrUv = "TEXCOORD_" + std::to_string(material.mrUv);
            Attribute<glm::vec2>(asset, primitive, baseUv, count, [&](auto v, size_t i) {
                vertices[i].uv = glm::vec2(material.baseUvTransform * glm::vec3(v, 1));
            });
            Attribute<glm::vec2>(asset, primitive, mrUv, count, [&](auto v, size_t i) {
                vertices[i].mrUv = glm::vec2(material.mrUvTransform * glm::vec3(v, 1));
            });
            if ((material.baseTexture && primitive.findAttribute(baseUv) == primitive.attributes.end()) ||
                (material.metallicRoughnessTexture &&
                 primitive.findAttribute(mrUv) == primitive.attributes.end())) {
                std::cerr << "Texture fallback: primitive is missing a referenced UV set\n";
                if (primitive.findAttribute(baseUv) == primitive.attributes.end())
                    material.baseTexture.reset();
                if (primitive.findAttribute(mrUv) == primitive.attributes.end())
                    material.metallicRoughnessTexture.reset();
                materialIndex = out.materials.size();
                out.materials.push_back(material);
            }
            if (const auto color = primitive.findAttribute("COLOR_0"); color != primitive.attributes.end()) {
                if (asset.accessors.at(color->accessorIndex).type == fastgltf::AccessorType::Vec3)
                    Attribute<glm::vec3>(asset, primitive, "COLOR_0", count,
                                         [&](auto v, size_t i) { vertices[i].color = glm::vec4(v, 1); });
                else
                    Attribute<glm::vec4>(asset, primitive, "COLOR_0", count,
                                         [&](auto v, size_t i) { vertices[i].color = v; });
            }
            std::vector<uint32_t> source;
            if (primitive.indicesAccessor) {
                const auto& accessor = asset.accessors.at(*primitive.indicesAccessor);
                if (accessor.normalized || accessor.type != fastgltf::AccessorType::Scalar ||
                    (accessor.componentType != fastgltf::ComponentType::UnsignedByte &&
                     accessor.componentType != fastgltf::ComponentType::UnsignedShort &&
                     accessor.componentType != fastgltf::ComponentType::UnsignedInt))
                    throw std::runtime_error("Invalid glTF index type");
                ValidateAccessor(asset, accessor);
                source.reserve(accessor.count);
                fastgltf::iterateAccessor<uint32_t>(asset, accessor,
                                                    [&](uint32_t index) { source.push_back(index); });
            } else {
                source.resize(count);
                std::iota(source.begin(), source.end(), 0u);
            }
            for (auto index : source)
                if (index >= count)
                    throw std::runtime_error("glTF index exceeds vertex count");
            std::vector<uint32_t> indices;
            if (primitive.type == fastgltf::PrimitiveType::Triangles)
                indices = std::move(source);
            else if (primitive.type == fastgltf::PrimitiveType::TriangleStrip ||
                     primitive.type == fastgltf::PrimitiveType::TriangleFan) {
                for (size_t i = 2; i < source.size(); ++i) {
                    if (primitive.type == fastgltf::PrimitiveType::TriangleFan)
                        indices.insert(indices.end(), {source[0], source[i - 1], source[i]});
                    else if (i % 2)
                        indices.insert(indices.end(), {source[i - 1], source[i - 2], source[i]});
                    else
                        indices.insert(indices.end(), {source[i - 2], source[i - 1], source[i]});
                }
            } else
                throw std::runtime_error("Only triangle glTF primitives are supported");
            if (indices.empty() || indices.size() % 3 || indices.size() > UINT32_MAX - out.indices.size())
                throw std::runtime_error("Invalid triangle index count");
            std::vector<glm::vec3> generated(count, glm::vec3(0));
            for (size_t i = 0; i < indices.size(); i += 3) {
                const auto a = indices[i], b = indices[i + 1], c = indices[i + 2];
                const auto n = glm::cross(vertices[b].position - vertices[a].position,
                                          vertices[c].position - vertices[a].position);
                generated[a] += n;
                generated[b] += n;
                generated[c] += n;
            }
            for (size_t i = 0; i < count; ++i) {
                auto n = vertices[i].normal;
                if (glm::dot(n, n) < 1e-12f)
                    n = generated[i];
                vertices[i].normal = glm::dot(n, n) > 1e-12f ? glm::normalize(n) : glm::vec3(0, 1, 0);
                vertices[i].color = glm::clamp(vertices[i].color, glm::vec4(0), glm::vec4(1));
            }
            const auto base = static_cast<uint32_t>(out.vertices.size());
            GltfDraw draw{static_cast<uint32_t>(out.indices.size()), static_cast<uint32_t>(indices.size()),
                          materialIndex, glm::mat4(1)};
            glm::vec3 low(std::numeric_limits<float>::max()), high(-std::numeric_limits<float>::max());
            for (const auto& vertex : vertices) {
                if (!Finite(vertex.uv) || !Finite(vertex.mrUv))
                    throw std::runtime_error("Non-finite transformed UV");
                low = glm::min(low, vertex.position);
                high = glm::max(high, vertex.position);
            }
            draw.center = glm::vec3((glm::dvec3(low) + glm::dvec3(high)) * 0.5);
            for (auto index : indices)
                out.indices.push_back(base + index);
            out.vertices.insert(out.vertices.end(), vertices.begin(), vertices.end());
            draws.push_back(draw);
        }
        meshes.push_back(std::move(draws));
    }
    std::vector<uint8_t> visited(asset.nodes.size(), 0);
    std::function<void(size_t, glm::mat4, size_t)> visit = [&](size_t index, glm::mat4 parent, size_t depth) {
        if (index >= asset.nodes.size() || depth > 256 || visited[index])
            throw std::runtime_error("Invalid/cyclic glTF node hierarchy");
        visited[index] = 1;
        const auto& node = asset.nodes[index];
        const auto matrix = fastgltf::getTransformMatrix(node);
        glm::mat4 local(1);
        for (int c = 0; c < 4; ++c)
            for (int r = 0; r < 4; ++r)
                local[c][r] = matrix[static_cast<size_t>(c)][static_cast<size_t>(r)];
        const auto world = parent * local;
        for (int c = 0; c < 4; ++c)
            if (!Finite(world[c]))
                throw std::runtime_error("Non-finite glTF node transform");
        if (node.meshIndex && glm::determinant(glm::dmat4(world)) != 0.0)
            for (auto draw : meshes.at(*node.meshIndex)) {
                draw.transform = world;
                out.draws.push_back(draw);
            }
        if (node.skinIndex)
            std::cerr << "glTF skin rendered in its static mesh pose\n";
        for (auto child : node.children)
            visit(child, world, depth + 1);
    };
    if (asset.nodes.empty() && asset.scenes.empty()) {
        for (const auto& mesh : meshes)
            for (auto draw : mesh)
                out.draws.push_back(draw);
    } else if (!asset.scenes.empty()) {
        for (auto node : asset.scenes.at(asset.defaultScene.value_or(0)).nodeIndices)
            visit(node, glm::mat4(1), 0);
    } else {
        std::vector<bool> child(asset.nodes.size(), false);
        for (const auto& node : asset.nodes)
            for (auto index : node.children)
                child.at(index) = true;
        for (size_t i = 0; i < child.size(); ++i)
            if (!child[i])
                visit(i, glm::mat4(1), 0);
    }
    Fit(out);
    std::cout << "Loaded " << path << ": " << out.vertices.size() << " vertices, " << out.draws.size()
              << " draws, " << out.materials.size() << " materials\n";
    return out;
}

GltfScene MakeFallbackBox() {
    GltfScene scene;
    GltfMaterial material;
    material.metallic = 0.15f;
    material.roughness = 0.4f;
    scene.materials.push_back(material);
    const glm::vec3 normals[] = {{0, 0, 1}, {0, 0, -1}, {1, 0, 0}, {-1, 0, 0}, {0, 1, 0}, {0, -1, 0}};
    const glm::vec3 ups[] = {{0, 1, 0}, {0, 1, 0}, {0, 1, 0}, {0, 1, 0}, {0, 0, -1}, {0, 0, 1}};
    const glm::vec4 colors[] = {{0.9f, 0.25f, 0.08f, 1}, {0.1f, 0.5f, 0.9f, 1},   {0.1f, 0.8f, 0.3f, 1},
                                {0.65f, 0.15f, 0.8f, 1}, {0.9f, 0.65f, 0.08f, 1}, {0.1f, 0.7f, 0.7f, 1}};
    for (uint32_t face = 0; face < 6; ++face) {
        const auto n = normals[face], up = ups[face], right = glm::cross(up, n);
        const glm::vec2 uvs[] = {{0, 1}, {1, 1}, {1, 0}, {0, 0}};
        const glm::vec3 points[] = {n - right - up, n + right - up, n + right + up, n - right + up};
        for (int i = 0; i < 4; ++i)
            scene.vertices.push_back({points[i], n, uvs[i], uvs[i], colors[face]});
        for (auto index : {0u, 1u, 2u, 0u, 2u, 3u})
            scene.indices.push_back(face * 4 + index);
    }
    const auto orientation = glm::rotate(glm::mat4(1), 0.5f, glm::vec3(0, 1, 0)) *
                             glm::rotate(glm::mat4(1), -0.3f, glm::vec3(1, 0, 0));
    scene.draws.push_back({0, 36, 0, orientation});
    Fit(scene);
    return scene;
}

GpuBuffer::~GpuBuffer() { Reset(); }
GpuBuffer::GpuBuffer(GpuBuffer&& other) noexcept { *this = std::move(other); }
GpuBuffer& GpuBuffer::operator=(GpuBuffer&& other) noexcept {
    if (this != &other) {
        Reset();
        allocator = other.allocator;
        buffer = std::exchange(other.buffer, VK_NULL_HANDLE);
        allocation = std::exchange(other.allocation, nullptr);
        mapped = std::exchange(other.mapped, nullptr);
    }
    return *this;
}
void GpuBuffer::Reset() noexcept {
    if (buffer)
        vmaDestroyBuffer(allocator, buffer, allocation);
    buffer = VK_NULL_HANDLE;
    allocation = nullptr;
    mapped = nullptr;
}
void GpuBuffer::Create(VmaAllocator owner, VkDeviceSize size, VkBufferUsageFlags usage, bool host) {
    Reset();
    allocator = owner;
    VkBufferCreateInfo info{};
    info.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    info.size = size;
    info.usage = usage;
    info.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
    VmaAllocationCreateInfo memory{};
    memory.usage = VMA_MEMORY_USAGE_AUTO;
    if (host)
        memory.flags =
            VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT | VMA_ALLOCATION_CREATE_MAPPED_BIT;
    else
        memory.requiredFlags = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;
    VmaAllocationInfo allocationInfo{};
    Check(vmaCreateBuffer(allocator, &info, &memory, &buffer, &allocation, &allocationInfo),
          "vmaCreateBuffer");
    mapped = allocationInfo.pMappedData;
}
void GpuBuffer::Write(const void* bytes, VkDeviceSize size) {
    if (!mapped)
        throw std::logic_error("Buffer is not host writable");
    std::memcpy(mapped, bytes, static_cast<size_t>(size));
    Check(vmaFlushAllocation(allocator, allocation, 0, size), "vmaFlushAllocation");
}
GpuBuffer UploadGltfBuffer(VmaAllocator allocator, VkCommandBuffer commands, std::span<const std::byte> bytes,
                           VkBufferUsageFlags usage, std::vector<GpuBuffer>& staging) {
    GpuBuffer upload;
    upload.Create(allocator, bytes.size(), VK_BUFFER_USAGE_TRANSFER_SRC_BIT, true);
    upload.Write(bytes.data(), bytes.size());
    GpuBuffer target;
    target.Create(allocator, bytes.size(), usage | VK_BUFFER_USAGE_TRANSFER_DST_BIT);
    VkBufferCopy copy{0, 0, bytes.size()};
    vkCmdCopyBuffer(commands, upload.buffer, target.buffer, 1, &copy);
    VkBufferMemoryBarrier barrier{};
    barrier.sType = VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER;
    barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
    barrier.dstAccessMask = usage & VK_BUFFER_USAGE_INDEX_BUFFER_BIT ? VK_ACCESS_INDEX_READ_BIT
                                                                     : VK_ACCESS_VERTEX_ATTRIBUTE_READ_BIT;
    barrier.srcQueueFamilyIndex = barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    barrier.buffer = target.buffer;
    barrier.size = VK_WHOLE_SIZE;
    vkCmdPipelineBarrier(commands, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_VERTEX_INPUT_BIT, 0, 0,
                         nullptr, 1, &barrier, 0, nullptr);
    staging.push_back(std::move(upload));
    return target;
}
