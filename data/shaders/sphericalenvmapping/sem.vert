#version 450

#extension GL_ARB_separate_shader_objects : enable
#extension GL_ARB_shading_language_420pack : enable

layout (location = 0) in vec4 inPos;
layout (location = 1) in vec3 inNormal;
layout (location = 3) in vec3 inColor;

layout (binding = 0) uniform UBO 
{
	mat4 projection;
	mat4 model;
	mat4 normal;
	mat4 view;
	int texIndex;
} ubo;

layout (location = 0) out vec3 outColor;
layout (location = 1) out vec3 outEyePos;
layout (location = 2) out vec3 outNormal;
layout (location = 3) out flat int outTexIndex;

void main() 
{
	outColor = inColor;
	mat4 modelView = ubo.view * ubo.model;
	outEyePos = normalize( vec3( modelView * inPos ) );
	outTexIndex = ubo.texIndex;
	outNormal = normalize( mat3(ubo.normal) * inNormal );
	vec3 r = reflect( outEyePos, outNormal );
	float m = 2.0 * sqrt( pow(r.x, 2.0) + pow(r.y, 2.0) + pow(r.z + 1.0, 2.0));
	gl_Position = ubo.projection * modelView * inPos;	
}
