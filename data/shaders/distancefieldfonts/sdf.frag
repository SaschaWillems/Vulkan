#version 450

#extension GL_ARB_separate_shader_objects : enable
#extension GL_ARB_shading_language_420pack : enable

layout (binding = 1) uniform sampler2D samplerColor;

layout (binding = 2) uniform UBO 
{
	vec4 outlineColor;
	float outlineWidth;
	float outline;
} ubo;

layout (location = 0) in vec2 inUV;

layout (location = 0) out vec4 outFragColor;

void main() 
{
    float distance = texture(samplerColor, inUV).a;
    float smoothWidth = fwidth(distance);	
    float alpha = smoothstep(0.5 - smoothWidth, 0.5 + smoothWidth, distance);
	vec3 rgb = vec3(alpha);
									 
	if (ubo.outline > 0.0) 
	{
		float w = 1.0 - ubo.outlineWidth;
		alpha = smoothstep(w - smoothWidth, w + smoothWidth, distance);
        rgb += mix(vec3(alpha), ubo.outlineColor.rgb, alpha);
    }									 
									 
    outFragColor = vec4(rgb, alpha);	
	
}