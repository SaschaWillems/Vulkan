#version 450

layout (input_attachment_index = 0, binding = 0) uniform subpassInput inputColor;
layout (input_attachment_index = 1, binding = 1) uniform subpassInput inputDepth;

layout (binding = 2) uniform UBO {
	vec2 range;
	int attachmentIndex;
} ubo;

layout (location = 0) out vec4 outColor;

void main() 
{
	// Read values from previous sub pass
	vec3 col = ubo.attachmentIndex == 0 ? subpassLoad(inputColor).rgb : subpassLoad(inputDepth).rrr;

	outColor.rgb = ((col - ubo.range[0]) * 1.0 / (ubo.range[1] - ubo.range[0]));
}