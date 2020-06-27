# glTF scene rendering

<img src="../../screenshots/gltf_scene.jpg" height="256px">

## Synopsis

Render a complete scene loaded from an glTF file. The sample is based on the [glTF scene](../gltfscene) sample, and adds data structures, functions and shaders required to render a more complex scene using Crytek's Sponza model.

## Description

This example demonstrates how to render a more complex scene loaded from a glTF model.

It builds on the basic glTF scene sample but instad of using global pipelines, it adds per-material pipelines that are dynamically created from the material definitions of the glTF model.

Those pipelines pass per-material parameters to the shader so different materials for e.g. displaying opqaue and transparent objects can be built from a single shader.

It also adds data structures, loading functions and shaders to do normal mapping and an easy way of toggling visibility for the scene nodes.

Note that this is not a full glTF implementation as this would be beyond the scope of a simple example. For a complete glTF Vulkan implementation see https://github.com/SaschaWillems/Vulkan-glTF-PBR/.

## Points of interest

**Note:** Points of interest are marked with a **POI** in the code comments:

```cpp
// POI: This sample uses normal mapping, so we also need to load the tangents from the glTF file
```

For this sample, those points of interest mark additions and changes compared to the basic glTF sample.

### Materials 

#### New Material poperties

```cpp
struct Material 
{
	glm::vec4 baseColorFactor = glm::vec4(1.0f);
	uint32_t baseColorTextureIndex;
	uint32_t normalTextureIndex;
	std::string alphaMode = "OPAQUE";
	float alphaCutOff;
	bool doubleSided = false;
	VkDescriptorSet descriptorSet;
	VkPipeline pipeline;
};
```

Several new properties have been added to the material class for this example that are taken from the glTF source.

Along with the base color we now also get the index of the normal map for that material in ```normalTextureIndex```, and store several material properties required to render the different materials in this scene:

- ```alphaMode```<br/>
The alpha mode defines how the alpha value for this material is determined. For opaque materials it's ignored, for masked materials the shader will discard fragments based on the alpha cutuff.
- ```alphaCutOff```<br/>
For masked materials, this value speficies the threshold between fully opaque and fully transparent. This is used to discard fragments in the fragment shader.
- ```doubleSided```<br/>
This property is used to select the appropriate culling mode for this material. For double-sided materials, culling will be disabled.

#### Per-Material pipelines

Unlike most of the other samples that use a few fixed pipelines, this sample will dynamically generate per-material pipelines based on material properties in the ````VulkanExample::preparePipelines()``` function

We first setup pipeline state that's common for all materials:

```cpp
// Setup common pipeline state properties...
VkPipelineInputAssemblyStateCreateInfo inputAssemblyStateCI = ...
VkPipelineRasterizationStateCreateInfo rasterizationStateCI = ...
VkPipelineColorBlendAttachmentState blendAttachmentStateCI = ...
...

for (auto &material : glTFScene.materials) 
{
	...
```

For each material we then set constant properties for the fragment shader using specialization constants:

```cpp
	struct MaterialSpecializationData {
		bool alphaMask;
		float alphaMaskCutoff;
	} materialSpecializationData;

	materialSpecializationData.alphaMask = material.alphaMode == "MASK";
	materialSpecializationData.alphaMaskCutoff = material.alphaCutOff;

	std::vector<VkSpecializationMapEntry> specializationMapEntries = {
		vks::initializers::specializationMapEntry(0, offsetof(MaterialSpecializationData, alphaMask), sizeof(MaterialSpecializationData::alphaMask)),
		vks::initializers::specializationMapEntry(1, offsetof(MaterialSpecializationData, alphaMaskCutoff), sizeof(MaterialSpecializationData::alphaMaskCutoff)),
	};
	VkSpecializationInfo specializationInfo = vks::initializers::specializationInfo(specializationMapEntries, sizeof(materialSpecializationData), &materialSpecializationData);
	shaderStages[1].pSpecializationInfo = &specializationInfo;
	...
```

We also set the culling mode depending on wether this material is double-sided:

```cpp
	// For double sided materials, culling will be disabled
	rasterizationStateCI.cullMode = material.doubleSided ? VK_CULL_MODE_NONE : VK_CULL_MODE_BACK_BIT;
```

With those setup we create a pipeline for the current material and store it as a property of the material class:

```cpp
	VK_CHECK_RESULT(vkCreateGraphicsPipelines(device, pipelineCache, 1, &pipelineCI, nullptr, &material.pipeline));
}
```

The material now also get's it's own ```pipeline```.

The alpha mask properties are used in the fragment shader to distinguish between opaque and transparent materials (```scene.frag```).

Specialization constant declartion in the shaders's header:

```glsl
layout (constant_id = 0) const bool ALPHA_MASK = false;
layout (constant_id = 1) const float ALPHA_MASK_CUTOFF = 0.0;
```

For alpha masked materials, fragments below the cutoff threshold are discarded:

```glsl
	vec4 color = texture(samplerColorMap, inUV) * vec4(inColor, 1.0);

	if (ALPHA_MASK) {
		if (color.a < ALPHA_MASK_CUTOFF) {
			discard;
		}
	}
```

### Normal mapping




### Rendering the scene