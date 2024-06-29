#version 450

#extension GL_EXT_scalar_block_layout: require
#extension GL_EXT_buffer_reference : require

layout (location = 0) in vec3 inPos;
layout (location = 1) in vec3 inNormal;
layout (location = 2) in vec2 inUV;
layout (location = 3) in vec3 inColor;

layout (buffer_reference, scalar) readonly buffer ModelDataReference {
	mat4 matrix;
	//vec3 color;
};

layout (set = 0, binding = 0) uniform UBOMatrices {
	mat4 projection;
	mat4 view;
} uboMatrices;

layout (push_constant) uniform PushConstants
{
	// Pointer to the actual buffer
	ModelDataReference modelDataReference;
} pushConstants;

layout (location = 0) out vec3 outNormal;
layout (location = 1) out vec3 outColor;
layout (location = 2) out vec2 outUV;

void main() 
{
	ModelDataReference modelData = pushConstants.modelDataReference;

	outNormal = inNormal;
	outColor = inColor;
	outUV = inUV;
	gl_Position = uboMatrices.projection * uboMatrices.view * modelData.matrix * vec4(inPos.xyz, 1.0);
}