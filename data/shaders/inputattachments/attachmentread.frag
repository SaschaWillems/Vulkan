#version 450

layout (input_attachment_index = 0, binding = 0) uniform subpassInput inputColor;
layout (input_attachment_index = 1, binding = 1) uniform subpassInput inputDepth;

layout (binding = 2) uniform UBO {
	vec2 brightnessContrast;
	vec2 range;
	int attachmentIndex;
} ubo;

layout (location = 0) out vec4 outColor;

vec3 brightnessContrast(vec3 color, float brightness, float contrast) {
	return (color - 0.5) * contrast + 0.5 + brightness;
}

void main() 
{
	// Apply brightness and contrast filer to color input
	if (ubo.attachmentIndex == 0) {
		// Read color from previous color input attachment
		vec3 color = subpassLoad(inputColor).rgb;
		outColor.rgb = brightnessContrast(color, ubo.brightnessContrast[0], ubo.brightnessContrast[1]);
	}

	// Visualize depth input range
	if (ubo.attachmentIndex == 1) {
		// Read depth from previous depth input attachment
		float depth = subpassLoad(inputDepth).r;
		outColor.rgb = vec3((depth - ubo.range[0]) * 1.0 / (ubo.range[1] - ubo.range[0]));
	}
}