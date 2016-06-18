#version 450 core

layout (location = 0) in vec2 inUV;

layout (set = 0, binding = 1) uniform sampler2D samplerColorMap;

layout (location = 0) out vec4 outFragColor;

void main(void)
{
	vec4 color = texture(samplerColorMap, inUV);
	outFragColor = vec4(color.rgb, 1.0);
}
