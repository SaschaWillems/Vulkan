#version 450

#extension GL_ARB_separate_shader_objects : enable
#extension GL_ARB_shading_language_420pack : enable

layout (binding = 1) uniform sampler2D samplerColor;

layout (binding = 2) uniform UBO 
{
	int texWidth;
	int texHeight;
	float radialBlurScale;
	float radialBlurStrength;
	vec2 radialOrigin;
} ubo;

layout (location = 0) in vec2 inUV;

layout (location = 0) out vec4 outFragColor;

void main() 
{
	vec2 radialSize = vec2(1.0 / ubo.texWidth, 1.0 / ubo.texHeight); 
	
	vec2 UV = inUV;
 
	vec4 color = vec4(0.0, 0.0, 0.0, 0.0);
	UV += radialSize * 0.5 - ubo.radialOrigin;
 
	#define samples 16

	for (int i = 0; i < samples; i++) 
	{
		float scale = 1.0 - ubo.radialBlurScale * (float(i) / float(samples-1));
		color += texture(samplerColor, UV * scale + ubo.radialOrigin);
	}
 
	outFragColor = (color / samples) * ubo.radialBlurStrength;
}