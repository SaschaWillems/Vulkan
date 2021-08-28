/*
	Vulkan Example - Cascaded shadow mapping for directional light sources
	Copyright by Sascha Willems - www.saschawillems.de
	This code is licensed under the MIT license (MIT) (http://opensource.org/licenses/MIT)
*/

/*
	This example implements projective cascaded shadow mapping. This technique splits up the camera frustum into
	multiple frustums with each getting its own full-res shadow map, implemented as a layered depth-only image.
	The shader then selects the proper shadow map layer depending on what split of the frustum the depth value
	to compare fits into.

	This results in a better shadow map resolution distribution that can be tweaked even further by increasing
	the number of frustum splits.

	A further optimization could be done using a geometry shader to do a single-pass render for the depth map
	cascades instead of multiple passes (geometry shaders are not supported on all target devices).
*/

#include "vulkanexamplebase.h"
#include "VulkanglTFModel.h"

#define ENABLE_VALIDATION false

#if defined(__ANDROID__)
#define SHADOWMAP_DIM 2048
#else
#define SHADOWMAP_DIM 4096
#endif

#define SHADOW_MAP_CASCADE_COUNT 4

class VulkanExample : public VulkanExampleBase
{
public:
	bool displayDepthMap = false;
	int32_t displayDepthMapCascadeIndex = 0;
	bool colorCascades = false;
	bool filterPCF = false;

	float cascadeSplitLambda = 0.95f;

	float zNear = 0.5f;
	float zFar = 48.0f;

	glm::vec3 lightPos = glm::vec3();

	struct Models {
		vkglTF::Model terrain;
		vkglTF::Model tree;
	} models;

	struct uniformBuffers {
		vks::Buffer VS;
		vks::Buffer FS;
	} uniformBuffers;

	struct UBOVS {
		glm::mat4 projection;
		glm::mat4 view;
		glm::mat4 model;
		glm::vec3 lightDir;
	} uboVS;

	struct UBOFS {
		float cascadeSplits[4];
		glm::mat4 cascadeViewProjMat[4];
		glm::mat4 inverseViewMat;
		glm::vec3 lightDir;
		float _pad;
		int32_t colorCascades;
	} uboFS;

	VkPipelineLayout pipelineLayout;
	struct Pipelines {
		VkPipeline debugShadowMap;
		VkPipeline sceneShadow;
		VkPipeline sceneShadowPCF;
	} pipelines;

	struct DescriptorSetLayouts {
		VkDescriptorSetLayout base;
	} descriptorSetLayouts;
	VkDescriptorSet descriptorSet;

	// For simplicity all pipelines use the same push constant block layout
	struct PushConstBlock {
		glm::vec4 position;
		uint32_t cascadeIndex;
	};

	// Resources of the depth map generation pass
	struct DepthPass {
		VkRenderPass renderPass;
		VkPipelineLayout pipelineLayout;
		VkPipeline pipeline;
		vks::Buffer uniformBuffer;

		struct UniformBlock {
			std::array<glm::mat4, SHADOW_MAP_CASCADE_COUNT> cascadeViewProjMat;
		} ubo;

	} depthPass;

	// Layered depth image containing the shadow cascade depths
	struct DepthImage {
		VkImage image;
		VkDeviceMemory mem;
		VkImageView view;
		VkSampler sampler;
		void destroy(VkDevice device) {
			vkDestroyImageView(device, view, nullptr);
			vkDestroyImage(device, image, nullptr);
			vkFreeMemory(device, mem, nullptr);
			vkDestroySampler(device, sampler, nullptr);
		}
	} depth;

	// Contains all resources required for a single shadow map cascade
	struct Cascade {
		VkFramebuffer frameBuffer;
		VkDescriptorSet descriptorSet;
		VkImageView view;

		float splitDepth;
		glm::mat4 viewProjMatrix;

		void destroy(VkDevice device) {
			vkDestroyImageView(device, view, nullptr);
			vkDestroyFramebuffer(device, frameBuffer, nullptr);
		}
	};
	std::array<Cascade, SHADOW_MAP_CASCADE_COUNT> cascades;

	VulkanExample() : VulkanExampleBase(ENABLE_VALIDATION)
	{
		title = "Cascaded shadow mapping";
		timerSpeed *= 0.025f;
		camera.type = Camera::CameraType::firstperson;
		camera.movementSpeed = 2.5f;
		camera.setPerspective(45.0f, (float)width / (float)height, zNear, zFar);
		camera.setPosition(glm::vec3(-0.12f, 1.14f, -2.25f));
		camera.setRotation(glm::vec3(-17.0f, 7.0f, 0.0f));
		timer = 0.2f;
	}

	~VulkanExample()
	{
		for (auto cascade : cascades) {
			cascade.destroy(device);
		}
		depth.destroy(device);

		vkDestroyRenderPass(device, depthPass.renderPass, nullptr);

		vkDestroyPipeline(device, pipelines.debugShadowMap, nullptr);
		vkDestroyPipeline(device, depthPass.pipeline, nullptr);
		vkDestroyPipeline(device, pipelines.sceneShadow, nullptr);
		vkDestroyPipeline(device, pipelines.sceneShadowPCF, nullptr);

		vkDestroyPipelineLayout(device, pipelineLayout, nullptr);
		vkDestroyPipelineLayout(device, depthPass.pipelineLayout, nullptr);

		vkDestroyDescriptorSetLayout(device, descriptorSetLayouts.base, nullptr);

		depthPass.uniformBuffer.destroy();
		uniformBuffers.VS.destroy();
		uniformBuffers.FS.destroy();
	}

	virtual void getEnabledFeatures()
	{
		enabledFeatures.samplerAnisotropy = deviceFeatures.samplerAnisotropy;
		// Depth clamp to avoid near plane clipping
		enabledFeatures.depthClamp = deviceFeatures.depthClamp;
	}

	/*
		Render the example scene with given command buffer, pipeline layout and descriptor set
		Used by the scene rendering and depth pass generation command buffer
	*/
	void renderScene(VkCommandBuffer commandBuffer, VkPipelineLayout pipelineLayout, VkDescriptorSet descriptorSet, uint32_t cascadeIndex = 0) {
		// We use push constants for passing shadow cascade info to the shaders
		PushConstBlock pushConstBlock = { glm::vec4(0.0f), cascadeIndex };

		// Set 0 contains the vertex and fragment shader uniform buffers, set 1 for images will be set by the glTF model class at draw time
		vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipelineLayout, 0, 1, &descriptorSet, 0, nullptr);

		// Floor
		vkCmdPushConstants(commandBuffer, pipelineLayout, VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof(PushConstBlock), &pushConstBlock);
		models.terrain.draw(commandBuffer, vkglTF::RenderFlags::BindImages, pipelineLayout);

		// Trees
		const std::vector<glm::vec3> positions = {
			glm::vec3(0.0f, 0.0f, 0.0f),
			glm::vec3(1.25f, 0.25f, 1.25f),
			glm::vec3(-1.25f, -0.2f, 1.25f),
			glm::vec3(1.25f, 0.1f, -1.25f),
			glm::vec3(-1.25f, -0.25f, -1.25f),
		};

		for (auto position : positions) {
			pushConstBlock.position = glm::vec4(position, 0.0f);
			vkCmdPushConstants(commandBuffer, pipelineLayout, VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof(PushConstBlock), &pushConstBlock);
			vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipelineLayout, 0, 1, &descriptorSet, 0, nullptr);
			models.tree.draw(commandBuffer, vkglTF::RenderFlags::BindImages, pipelineLayout);
		}
	}

	/*
		Setup resources used by the depth pass
		The depth image is layered with each layer storing one shadow map cascade
	*/
	void prepareDepthPass()
	{
		VkFormat depthFormat = vulkanDevice->getSupportedDepthFormat(true);

		/*
			Depth map renderpass
		*/

		VkAttachmentDescription attachmentDescription{};
		attachmentDescription.format = depthFormat;
		attachmentDescription.samples = VK_SAMPLE_COUNT_1_BIT;
		attachmentDescription.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
		attachmentDescription.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
		attachmentDescription.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
		attachmentDescription.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
		attachmentDescription.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
		attachmentDescription.finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL;

		VkAttachmentReference depthReference = {};
		depthReference.attachment = 0;
		depthReference.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

		VkSubpassDescription subpass = {};
		subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
		subpass.colorAttachmentCount = 0;
		subpass.pDepthStencilAttachment = &depthReference;

		// Use subpass dependencies for layout transitions
		std::array<VkSubpassDependency, 2> dependencies;

		dependencies[0].srcSubpass = VK_SUBPASS_EXTERNAL;
		dependencies[0].dstSubpass = 0;
		dependencies[0].srcStageMask = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
		dependencies[0].dstStageMask = VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
		dependencies[0].srcAccessMask = VK_ACCESS_SHADER_READ_BIT;
		dependencies[0].dstAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
		dependencies[0].dependencyFlags = VK_DEPENDENCY_BY_REGION_BIT;

		dependencies[1].srcSubpass = 0;
		dependencies[1].dstSubpass = VK_SUBPASS_EXTERNAL;
		dependencies[1].srcStageMask = VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT;
		dependencies[1].dstStageMask = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
		dependencies[1].srcAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
		dependencies[1].dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
		dependencies[1].dependencyFlags = VK_DEPENDENCY_BY_REGION_BIT;

		VkRenderPassCreateInfo renderPassCreateInfo = vks::initializers::renderPassCreateInfo();
		renderPassCreateInfo.attachmentCount = 1;
		renderPassCreateInfo.pAttachments = &attachmentDescription;
		renderPassCreateInfo.subpassCount = 1;
		renderPassCreateInfo.pSubpasses = &subpass;
		renderPassCreateInfo.dependencyCount = static_cast<uint32_t>(dependencies.size());
		renderPassCreateInfo.pDependencies = dependencies.data();

		VK_CHECK_RESULT(vkCreateRenderPass(device, &renderPassCreateInfo, nullptr, &depthPass.renderPass));

		/*
			Layered depth image and views
		*/

		VkImageCreateInfo imageInfo = vks::initializers::imageCreateInfo();
		imageInfo.imageType = VK_IMAGE_TYPE_2D;
		imageInfo.extent.width = SHADOWMAP_DIM;
		imageInfo.extent.height = SHADOWMAP_DIM;
		imageInfo.extent.depth = 1;
		imageInfo.mipLevels = 1;
		imageInfo.arrayLayers = SHADOW_MAP_CASCADE_COUNT;
		imageInfo.samples = VK_SAMPLE_COUNT_1_BIT;
		imageInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
		imageInfo.format = depthFormat;
		imageInfo.usage = VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;
		VK_CHECK_RESULT(vkCreateImage(device, &imageInfo, nullptr, &depth.image));
		VkMemoryAllocateInfo memAlloc = vks::initializers::memoryAllocateInfo();
		VkMemoryRequirements memReqs;
		vkGetImageMemoryRequirements(device, depth.image, &memReqs);
		memAlloc.allocationSize = memReqs.size;
		memAlloc.memoryTypeIndex = vulkanDevice->getMemoryType(memReqs.memoryTypeBits, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
		VK_CHECK_RESULT(vkAllocateMemory(device, &memAlloc, nullptr, &depth.mem));
		VK_CHECK_RESULT(vkBindImageMemory(device, depth.image, depth.mem, 0));
		// Full depth map view (all layers)
		VkImageViewCreateInfo viewInfo = vks::initializers::imageViewCreateInfo();
		viewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D_ARRAY;
		viewInfo.format = depthFormat;
		viewInfo.subresourceRange = {};
		viewInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;
		viewInfo.subresourceRange.baseMipLevel = 0;
		viewInfo.subresourceRange.levelCount = 1;
		viewInfo.subresourceRange.baseArrayLayer = 0;
		viewInfo.subresourceRange.layerCount = SHADOW_MAP_CASCADE_COUNT;
		viewInfo.image = depth.image;
		VK_CHECK_RESULT(vkCreateImageView(device, &viewInfo, nullptr, &depth.view));

		// One image and framebuffer per cascade
		for (uint32_t i = 0; i < SHADOW_MAP_CASCADE_COUNT; i++) {
			// Image view for this cascade's layer (inside the depth map)
			// This view is used to render to that specific depth image layer
			VkImageViewCreateInfo viewInfo = vks::initializers::imageViewCreateInfo();
			viewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D_ARRAY;
			viewInfo.format = depthFormat;
			viewInfo.subresourceRange = {};
			viewInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;
			viewInfo.subresourceRange.baseMipLevel = 0;
			viewInfo.subresourceRange.levelCount = 1;
			viewInfo.subresourceRange.baseArrayLayer = i;
			viewInfo.subresourceRange.layerCount = 1;
			viewInfo.image = depth.image;
			VK_CHECK_RESULT(vkCreateImageView(device, &viewInfo, nullptr, &cascades[i].view));
			// Framebuffer
			VkFramebufferCreateInfo framebufferInfo = vks::initializers::framebufferCreateInfo();
			framebufferInfo.renderPass = depthPass.renderPass;
			framebufferInfo.attachmentCount = 1;
			framebufferInfo.pAttachments = &cascades[i].view;
			framebufferInfo.width = SHADOWMAP_DIM;
			framebufferInfo.height = SHADOWMAP_DIM;
			framebufferInfo.layers = 1;
			VK_CHECK_RESULT(vkCreateFramebuffer(device, &framebufferInfo, nullptr, &cascades[i].frameBuffer));
		}

		// Shared sampler for cascade depth reads
		VkSamplerCreateInfo sampler = vks::initializers::samplerCreateInfo();
		sampler.magFilter = VK_FILTER_LINEAR;
		sampler.minFilter = VK_FILTER_LINEAR;
		sampler.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
		sampler.addressModeU = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
		sampler.addressModeV = sampler.addressModeU;
		sampler.addressModeW = sampler.addressModeU;
		sampler.mipLodBias = 0.0f;
		sampler.maxAnisotropy = 1.0f;
		sampler.minLod = 0.0f;
		sampler.maxLod = 1.0f;
		sampler.borderColor = VK_BORDER_COLOR_FLOAT_OPAQUE_WHITE;
		VK_CHECK_RESULT(vkCreateSampler(device, &sampler, nullptr, &depth.sampler));
	}
	void buildCommandBuffers()
	{
		VkCommandBufferBeginInfo cmdBufInfo = vks::initializers::commandBufferBeginInfo();

		VkDeviceSize offsets[1] = { 0 };

		for (int32_t i = 0; i < drawCmdBuffers.size(); i++) {

			VK_CHECK_RESULT(vkBeginCommandBuffer(drawCmdBuffers[i], &cmdBufInfo));

			/*
				Generate depth map cascades

				Uses multiple passes with each pass rendering the scene to the cascade's depth image layer
				Could be optimized using a geometry shader (and layered frame buffer) on devices that support geometry shaders
			*/
			{
				VkClearValue clearValues[1];
				clearValues[0].depthStencil = { 1.0f, 0 };

				VkRenderPassBeginInfo renderPassBeginInfo = vks::initializers::renderPassBeginInfo();
				renderPassBeginInfo.renderPass = depthPass.renderPass;
				renderPassBeginInfo.renderArea.offset.x = 0;
				renderPassBeginInfo.renderArea.offset.y = 0;
				renderPassBeginInfo.renderArea.extent.width = SHADOWMAP_DIM;
				renderPassBeginInfo.renderArea.extent.height = SHADOWMAP_DIM;
				renderPassBeginInfo.clearValueCount = 1;
				renderPassBeginInfo.pClearValues = clearValues;

				VkViewport viewport = vks::initializers::viewport((float)SHADOWMAP_DIM, (float)SHADOWMAP_DIM, 0.0f, 1.0f);
				vkCmdSetViewport(drawCmdBuffers[i], 0, 1, &viewport);

				VkRect2D scissor = vks::initializers::rect2D(SHADOWMAP_DIM, SHADOWMAP_DIM, 0, 0);
				vkCmdSetScissor(drawCmdBuffers[i], 0, 1, &scissor);

				// One pass per cascade
				// The layer that this pass renders to is defined by the cascade's image view (selected via the cascade's descriptor set)
				for (uint32_t j = 0; j < SHADOW_MAP_CASCADE_COUNT; j++) {
					renderPassBeginInfo.framebuffer = cascades[j].frameBuffer;
					vkCmdBeginRenderPass(drawCmdBuffers[i], &renderPassBeginInfo, VK_SUBPASS_CONTENTS_INLINE);
					vkCmdBindPipeline(drawCmdBuffers[i], VK_PIPELINE_BIND_POINT_GRAPHICS, depthPass.pipeline);
					renderScene(drawCmdBuffers[i], depthPass.pipelineLayout, cascades[j].descriptorSet, j);
					vkCmdEndRenderPass(drawCmdBuffers[i]);
				}
			}

			/*
				Note: Explicit synchronization is not required between the render pass, as this is done implicit via sub pass dependencies
			*/

			/*
				Scene rendering using depth cascades for shadow mapping
			*/

			{
				VkClearValue clearValues[2];
				clearValues[0].color = { { 0.0f, 0.0f, 0.2f, 1.0f } };
				clearValues[1].depthStencil = { 1.0f, 0 };

				VkRenderPassBeginInfo renderPassBeginInfo = vks::initializers::renderPassBeginInfo();
				renderPassBeginInfo.renderPass = renderPass;
				renderPassBeginInfo.framebuffer = frameBuffers[i];
				renderPassBeginInfo.renderArea.offset.x = 0;
				renderPassBeginInfo.renderArea.offset.y = 0;
				renderPassBeginInfo.renderArea.extent.width = width;
				renderPassBeginInfo.renderArea.extent.height = height;
				renderPassBeginInfo.clearValueCount = 2;
				renderPassBeginInfo.pClearValues = clearValues;

				vkCmdBeginRenderPass(drawCmdBuffers[i], &renderPassBeginInfo, VK_SUBPASS_CONTENTS_INLINE);

				VkViewport viewport = vks::initializers::viewport((float)width, (float)height, 0.0f, 1.0f);
				vkCmdSetViewport(drawCmdBuffers[i], 0, 1, &viewport);

				VkRect2D scissor = vks::initializers::rect2D(width, height, 0, 0);
				vkCmdSetScissor(drawCmdBuffers[i], 0, 1, &scissor);

				// Visualize shadow map cascade
				if (displayDepthMap) {
					vkCmdBindDescriptorSets(drawCmdBuffers[i], VK_PIPELINE_BIND_POINT_GRAPHICS, pipelineLayout, 0, 1, &descriptorSet, 0, NULL);
					vkCmdBindPipeline(drawCmdBuffers[i], VK_PIPELINE_BIND_POINT_GRAPHICS, pipelines.debugShadowMap);
					PushConstBlock pushConstBlock = {};
					pushConstBlock.cascadeIndex = displayDepthMapCascadeIndex;
					vkCmdPushConstants(drawCmdBuffers[i], pipelineLayout, VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof(PushConstBlock), &pushConstBlock);
					vkCmdDraw(drawCmdBuffers[i], 3, 1, 0, 0);
				}

				// Render shadowed scene
				vkCmdBindPipeline(drawCmdBuffers[i], VK_PIPELINE_BIND_POINT_GRAPHICS, (filterPCF) ? pipelines.sceneShadowPCF : pipelines.sceneShadow);
				renderScene(drawCmdBuffers[i], pipelineLayout, descriptorSet);

				drawUI(drawCmdBuffers[i]);

				vkCmdEndRenderPass(drawCmdBuffers[i]);
			}

			VK_CHECK_RESULT(vkEndCommandBuffer(drawCmdBuffers[i]));
		}
	}

	void loadAssets()
	{
		uint32_t glTFLoadingFlags = vkglTF::FileLoadingFlags::PreTransformVertices | vkglTF::FileLoadingFlags::FlipY;
		models.terrain.loadFromFile(getAssetPath() + "models/terrain_gridlines.gltf", vulkanDevice, queue, glTFLoadingFlags);
		models.tree.loadFromFile(getAssetPath() + "models/oaktree.gltf", vulkanDevice, queue, glTFLoadingFlags);
	}

	void setupLayoutsAndDescriptors()
	{
		/*
			Descriptor pool
		*/
		std::vector<VkDescriptorPoolSize> poolSizes = {
			vks::initializers::descriptorPoolSize(VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 32),
			vks::initializers::descriptorPoolSize(VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 32)
		};
		VkDescriptorPoolCreateInfo descriptorPoolInfo =
			vks::initializers::descriptorPoolCreateInfo(static_cast<uint32_t>(poolSizes.size()), poolSizes.data(), 4 + SHADOW_MAP_CASCADE_COUNT);
		VK_CHECK_RESULT(vkCreateDescriptorPool(device, &descriptorPoolInfo, nullptr, &descriptorPool));

		/*
			Descriptor set layouts
		*/

		// Shared matrices and samplers
		std::vector<VkDescriptorSetLayoutBinding> setLayoutBindings = {
			vks::initializers::descriptorSetLayoutBinding(VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_VERTEX_BIT, 0),
			vks::initializers::descriptorSetLayoutBinding(VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT, 1),
			vks::initializers::descriptorSetLayoutBinding(VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_FRAGMENT_BIT, 2),
		};
		VkDescriptorSetLayoutCreateInfo descriptorLayout =
			vks::initializers::descriptorSetLayoutCreateInfo(setLayoutBindings);
		VK_CHECK_RESULT(vkCreateDescriptorSetLayout(device, &descriptorLayout, nullptr, &descriptorSetLayouts.base));

		/*
			Descriptor sets
		*/

		std::vector<VkWriteDescriptorSet> writeDescriptorSets;

		VkDescriptorImageInfo depthMapDescriptor =
			vks::initializers::descriptorImageInfo(depth.sampler, depth.view, VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL);

		VkDescriptorSetAllocateInfo allocInfo =
			vks::initializers::descriptorSetAllocateInfo(descriptorPool, &descriptorSetLayouts.base, 1);

		// Scene rendering / debug display
		VK_CHECK_RESULT(vkAllocateDescriptorSets(device, &allocInfo, &descriptorSet));
		writeDescriptorSets = {
			vks::initializers::writeDescriptorSet(descriptorSet, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 0, &uniformBuffers.VS.descriptor),
			vks::initializers::writeDescriptorSet(descriptorSet, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1, &depthMapDescriptor),
			vks::initializers::writeDescriptorSet(descriptorSet, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 2, &uniformBuffers.FS.descriptor),
		};
		vkUpdateDescriptorSets(device, static_cast<uint32_t>(writeDescriptorSets.size()), writeDescriptorSets.data(), 0, NULL);

		// Per-cascade descriptor sets
		// Each descriptor set represents a single layer of the array texture
		for (uint32_t i = 0; i < SHADOW_MAP_CASCADE_COUNT; i++) {
			VK_CHECK_RESULT(vkAllocateDescriptorSets(device, &allocInfo, &cascades[i].descriptorSet));
			VkDescriptorImageInfo cascadeImageInfo = vks::initializers::descriptorImageInfo(depth.sampler, depth.view, VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL);
			writeDescriptorSets = {
				vks::initializers::writeDescriptorSet(cascades[i].descriptorSet, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 0, &depthPass.uniformBuffer.descriptor),
				vks::initializers::writeDescriptorSet(cascades[i].descriptorSet, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1, &cascadeImageInfo)
			};
			vkUpdateDescriptorSets(device, static_cast<uint32_t>(writeDescriptorSets.size()), writeDescriptorSets.data(), 0, NULL);
		}

		/*
			Pipeline layouts
		*/

		// Shared pipeline layout (scene and depth map debug display)
		{
			VkPushConstantRange pushConstantRange = vks::initializers::pushConstantRange(VK_SHADER_STAGE_VERTEX_BIT, sizeof(PushConstBlock), 0);
			std::array<VkDescriptorSetLayout, 2> setLayouts = { descriptorSetLayouts.base, vkglTF::descriptorSetLayoutImage };
			VkPipelineLayoutCreateInfo pipelineLayoutCreateInfo = vks::initializers::pipelineLayoutCreateInfo(setLayouts.data(), static_cast<uint32_t>(setLayouts.size()));
			pipelineLayoutCreateInfo.pushConstantRangeCount = 1;
			pipelineLayoutCreateInfo.pPushConstantRanges = &pushConstantRange;
			VK_CHECK_RESULT(vkCreatePipelineLayout(device, &pipelineLayoutCreateInfo, nullptr, &pipelineLayout));
		}

		// Depth pass pipeline layout
		{
			VkPushConstantRange pushConstantRange = vks::initializers::pushConstantRange(VK_SHADER_STAGE_VERTEX_BIT, sizeof(PushConstBlock), 0);
			std::array<VkDescriptorSetLayout, 2> setLayouts = { descriptorSetLayouts.base, vkglTF::descriptorSetLayoutImage };
			VkPipelineLayoutCreateInfo pipelineLayoutCreateInfo = vks::initializers::pipelineLayoutCreateInfo(setLayouts.data(), static_cast<uint32_t>(setLayouts.size()));
			pipelineLayoutCreateInfo.pushConstantRangeCount = 1;
			pipelineLayoutCreateInfo.pPushConstantRanges = &pushConstantRange;
			VK_CHECK_RESULT(vkCreatePipelineLayout(device, &pipelineLayoutCreateInfo, nullptr, &depthPass.pipelineLayout));
		}
	}

	void preparePipelines()
	{
		VkPipelineInputAssemblyStateCreateInfo inputAssemblyState = vks::initializers::pipelineInputAssemblyStateCreateInfo(VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST, 0, VK_FALSE);
		VkPipelineRasterizationStateCreateInfo rasterizationState = vks::initializers::pipelineRasterizationStateCreateInfo(VK_POLYGON_MODE_FILL, VK_CULL_MODE_BACK_BIT, VK_FRONT_FACE_CLOCKWISE, 0);
		VkPipelineColorBlendAttachmentState blendAttachmentState = vks::initializers::pipelineColorBlendAttachmentState(0xf, VK_FALSE);
		VkPipelineColorBlendStateCreateInfo colorBlendState = vks::initializers::pipelineColorBlendStateCreateInfo(1, &blendAttachmentState);
		VkPipelineDepthStencilStateCreateInfo depthStencilState = vks::initializers::pipelineDepthStencilStateCreateInfo(VK_TRUE, VK_TRUE, VK_COMPARE_OP_LESS_OR_EQUAL);
		VkPipelineViewportStateCreateInfo viewportState = vks::initializers::pipelineViewportStateCreateInfo(1, 1, 0);
		VkPipelineMultisampleStateCreateInfo multisampleState = vks::initializers::pipelineMultisampleStateCreateInfo(VK_SAMPLE_COUNT_1_BIT, 0);
		std::vector<VkDynamicState> dynamicStateEnables = { VK_DYNAMIC_STATE_VIEWPORT, VK_DYNAMIC_STATE_SCISSOR };
		VkPipelineDynamicStateCreateInfo dynamicState = vks::initializers::pipelineDynamicStateCreateInfo(dynamicStateEnables);
		std::array<VkPipelineShaderStageCreateInfo, 2> shaderStages;

		VkGraphicsPipelineCreateInfo pipelineCI = vks::initializers::pipelineCreateInfo(pipelineLayout, renderPass, 0);
		pipelineCI.pInputAssemblyState = &inputAssemblyState;
		pipelineCI.pRasterizationState = &rasterizationState;
		pipelineCI.pColorBlendState = &colorBlendState;
		pipelineCI.pMultisampleState = &multisampleState;
		pipelineCI.pViewportState = &viewportState;
		pipelineCI.pDepthStencilState = &depthStencilState;
		pipelineCI.pDynamicState = &dynamicState;
		pipelineCI.stageCount = static_cast<uint32_t>(shaderStages.size());
		pipelineCI.pStages = shaderStages.data();

		// Shadow map cascade debug quad display
		rasterizationState.cullMode = VK_CULL_MODE_BACK_BIT;
		shaderStages[0] = loadShader(getShadersPath() + "shadowmappingcascade/debugshadowmap.vert.spv", VK_SHADER_STAGE_VERTEX_BIT);
		shaderStages[1] = loadShader(getShadersPath() + "shadowmappingcascade/debugshadowmap.frag.spv", VK_SHADER_STAGE_FRAGMENT_BIT);
		// Empty vertex input state
		VkPipelineVertexInputStateCreateInfo emptyInputState = vks::initializers::pipelineVertexInputStateCreateInfo();
		pipelineCI.pVertexInputState = &emptyInputState;
		VK_CHECK_RESULT(vkCreateGraphicsPipelines(device, pipelineCache, 1, &pipelineCI, nullptr, &pipelines.debugShadowMap));

		pipelineCI.pVertexInputState = vkglTF::Vertex::getPipelineVertexInputState({ vkglTF::VertexComponent::Position, vkglTF::VertexComponent::UV, vkglTF::VertexComponent::Color, vkglTF::VertexComponent::Normal });
		/*
			Shadow mapped scene rendering
		*/
		rasterizationState.cullMode = VK_CULL_MODE_NONE;
		shaderStages[0] = loadShader(getShadersPath() + "shadowmappingcascade/scene.vert.spv", VK_SHADER_STAGE_VERTEX_BIT);
		shaderStages[1] = loadShader(getShadersPath() + "shadowmappingcascade/scene.frag.spv", VK_SHADER_STAGE_FRAGMENT_BIT);
		// Use specialization constants to select between horizontal and vertical blur
		uint32_t enablePCF = 0;
		VkSpecializationMapEntry specializationMapEntry = vks::initializers::specializationMapEntry(0, 0, sizeof(uint32_t));
		VkSpecializationInfo specializationInfo = vks::initializers::specializationInfo(1, &specializationMapEntry, sizeof(uint32_t), &enablePCF);
		shaderStages[1].pSpecializationInfo = &specializationInfo;
		VK_CHECK_RESULT(vkCreateGraphicsPipelines(device, pipelineCache, 1, &pipelineCI, nullptr, &pipelines.sceneShadow));
		enablePCF = 1;
		VK_CHECK_RESULT(vkCreateGraphicsPipelines(device, pipelineCache, 1, &pipelineCI, nullptr, &pipelines.sceneShadowPCF));

		/*
			Depth map generation
		*/
		shaderStages[0] = loadShader(getShadersPath() + "shadowmappingcascade/depthpass.vert.spv", VK_SHADER_STAGE_VERTEX_BIT);
		shaderStages[1] = loadShader(getShadersPath() + "shadowmappingcascade/depthpass.frag.spv", VK_SHADER_STAGE_FRAGMENT_BIT);
		// No blend attachment states (no color attachments used)
		colorBlendState.attachmentCount = 0;
		depthStencilState.depthCompareOp = VK_COMPARE_OP_LESS_OR_EQUAL;
		// Enable depth clamp (if available)
		rasterizationState.depthClampEnable = deviceFeatures.depthClamp;
		pipelineCI.layout = depthPass.pipelineLayout;
		pipelineCI.renderPass = depthPass.renderPass;
		VK_CHECK_RESULT(vkCreateGraphicsPipelines(device, pipelineCache, 1, &pipelineCI, nullptr, &depthPass.pipeline));
	}

	void prepareUniformBuffers()
	{
		// Shadow map generation buffer blocks
		VK_CHECK_RESULT(vulkanDevice->createBuffer(
			VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
			VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
			&depthPass.uniformBuffer,
			sizeof(depthPass.ubo)));

		// Scene uniform buffer blocks
		VK_CHECK_RESULT(vulkanDevice->createBuffer(
			VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
			VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
			&uniformBuffers.VS,
			sizeof(uboVS)));
		VK_CHECK_RESULT(vulkanDevice->createBuffer(
			VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
			VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
			&uniformBuffers.FS,
			sizeof(uboFS)));

		// Map persistent
		VK_CHECK_RESULT(depthPass.uniformBuffer.map());
		VK_CHECK_RESULT(uniformBuffers.VS.map());
		VK_CHECK_RESULT(uniformBuffers.FS.map());

		updateLight();
		updateUniformBuffers();
	}

	/*
		Calculate frustum split depths and matrices for the shadow map cascades
		Based on https://johanmedestrom.wordpress.com/2016/03/18/opengl-cascaded-shadow-maps/
	*/
	void updateCascades()
	{
		float cascadeSplits[SHADOW_MAP_CASCADE_COUNT];

		float nearClip = camera.getNearClip();
		float farClip = camera.getFarClip();
		float clipRange = farClip - nearClip;

		float minZ = nearClip;
		float maxZ = nearClip + clipRange;

		float range = maxZ - minZ;
		float ratio = maxZ / minZ;

		// Calculate split depths based on view camera frustum
		// Based on method presented in https://developer.nvidia.com/gpugems/GPUGems3/gpugems3_ch10.html
		for (uint32_t i = 0; i < SHADOW_MAP_CASCADE_COUNT; i++) {
			float p = (i + 1) / static_cast<float>(SHADOW_MAP_CASCADE_COUNT);
			float log = minZ * std::pow(ratio, p);
			float uniform = minZ + range * p;
			float d = cascadeSplitLambda * (log - uniform) + uniform;
			cascadeSplits[i] = (d - nearClip) / clipRange;
		}

		// Calculate orthographic projection matrix for each cascade
		float lastSplitDist = 0.0;
		for (uint32_t i = 0; i < SHADOW_MAP_CASCADE_COUNT; i++) {
			float splitDist = cascadeSplits[i];

			glm::vec3 frustumCorners[8] = {
				glm::vec3(-1.0f,  1.0f, -1.0f),
				glm::vec3( 1.0f,  1.0f, -1.0f),
				glm::vec3( 1.0f, -1.0f, -1.0f),
				glm::vec3(-1.0f, -1.0f, -1.0f),
				glm::vec3(-1.0f,  1.0f,  1.0f),
				glm::vec3( 1.0f,  1.0f,  1.0f),
				glm::vec3( 1.0f, -1.0f,  1.0f),
				glm::vec3(-1.0f, -1.0f,  1.0f),
			};

			// Project frustum corners into world space
			glm::mat4 invCam = glm::inverse(camera.matrices.perspective * camera.matrices.view);
			for (uint32_t i = 0; i < 8; i++) {
				glm::vec4 invCorner = invCam * glm::vec4(frustumCorners[i], 1.0f);
				frustumCorners[i] = invCorner / invCorner.w;
			}

			for (uint32_t i = 0; i < 4; i++) {
				glm::vec3 dist = frustumCorners[i + 4] - frustumCorners[i];
				frustumCorners[i + 4] = frustumCorners[i] + (dist * splitDist);
				frustumCorners[i] = frustumCorners[i] + (dist * lastSplitDist);
			}

			// Get frustum center
			glm::vec3 frustumCenter = glm::vec3(0.0f);
			for (uint32_t i = 0; i < 8; i++) {
				frustumCenter += frustumCorners[i];
			}
			frustumCenter /= 8.0f;

			float radius = 0.0f;
			for (uint32_t i = 0; i < 8; i++) {
				float distance = glm::length(frustumCorners[i] - frustumCenter);
				radius = glm::max(radius, distance);
			}
			radius = std::ceil(radius * 16.0f) / 16.0f;

			glm::vec3 maxExtents = glm::vec3(radius);
			glm::vec3 minExtents = -maxExtents;

			glm::vec3 lightDir = normalize(-lightPos);
			glm::mat4 lightViewMatrix = glm::lookAt(frustumCenter - lightDir * -minExtents.z, frustumCenter, glm::vec3(0.0f, 1.0f, 0.0f));
			glm::mat4 lightOrthoMatrix = glm::ortho(minExtents.x, maxExtents.x, minExtents.y, maxExtents.y, 0.0f, maxExtents.z - minExtents.z);

			// Store split distance and matrix in cascade
			cascades[i].splitDepth = (camera.getNearClip() + splitDist * clipRange) * -1.0f;
			cascades[i].viewProjMatrix = lightOrthoMatrix * lightViewMatrix;

			lastSplitDist = cascadeSplits[i];
		}
	}

	void updateLight()
	{
		float angle = glm::radians(timer * 360.0f);
		float radius = 20.0f;
		lightPos = glm::vec3(cos(angle) * radius, -radius, sin(angle) * radius);
	}

	void updateUniformBuffers()
	{
		/*
			Depth rendering
		*/
		for (uint32_t i = 0; i < SHADOW_MAP_CASCADE_COUNT; i++) {
			depthPass.ubo.cascadeViewProjMat[i] = cascades[i].viewProjMatrix;
		}
		memcpy(depthPass.uniformBuffer.mapped, &depthPass.ubo, sizeof(depthPass.ubo));

		/*
			Scene rendering
		*/
		uboVS.projection = camera.matrices.perspective;
		uboVS.view = camera.matrices.view;
		uboVS.model = glm::mat4(1.0f);

		uboVS.lightDir = normalize(-lightPos);

		memcpy(uniformBuffers.VS.mapped, &uboVS, sizeof(uboVS));

		for (uint32_t i = 0; i < SHADOW_MAP_CASCADE_COUNT; i++) {
			uboFS.cascadeSplits[i] = cascades[i].splitDepth;
			uboFS.cascadeViewProjMat[i] = cascades[i].viewProjMatrix;
		}
		uboFS.inverseViewMat = glm::inverse(camera.matrices.view);
		uboFS.lightDir = normalize(-lightPos);
		uboFS.colorCascades = colorCascades;
		memcpy(uniformBuffers.FS.mapped, &uboFS, sizeof(uboFS));
	}

	void draw()
	{
		VulkanExampleBase::prepareFrame();
		submitInfo.commandBufferCount = 1;
		submitInfo.pCommandBuffers = &drawCmdBuffers[currentBuffer];
		VK_CHECK_RESULT(vkQueueSubmit(queue, 1, &submitInfo, VK_NULL_HANDLE));
		VulkanExampleBase::submitFrame();
	}

	void prepare()
	{
		VulkanExampleBase::prepare();
		loadAssets();
		updateLight();
		updateCascades();
		prepareDepthPass();
		prepareUniformBuffers();
		setupLayoutsAndDescriptors();
		preparePipelines();
		buildCommandBuffers();
		prepared = true;
	}

	virtual void render()
	{
		if (!prepared)
			return;
		draw();
		if (!paused || camera.updated) {
			updateLight();
			updateCascades();
			updateUniformBuffers();
		}
	}

	virtual void OnUpdateUIOverlay(vks::UIOverlay *overlay)
	{
		if (overlay->header("Settings")) {
			if (overlay->sliderFloat("Split lambda", &cascadeSplitLambda, 0.1f, 1.0f)) {
				updateCascades();
				updateUniformBuffers();
			}
			if (overlay->checkBox("Color cascades", &colorCascades)) {
				updateUniformBuffers();
			}
			if (overlay->checkBox("Display depth map", &displayDepthMap)) {
				buildCommandBuffers();
			}
			if (displayDepthMap) {
				if (overlay->sliderInt("Cascade", &displayDepthMapCascadeIndex, 0, SHADOW_MAP_CASCADE_COUNT - 1)) {
					buildCommandBuffers();
				}
			}
			if (overlay->checkBox("PCF filtering", &filterPCF)) {
				buildCommandBuffers();
			}
		}
	}
};

VULKAN_EXAMPLE_MAIN()
