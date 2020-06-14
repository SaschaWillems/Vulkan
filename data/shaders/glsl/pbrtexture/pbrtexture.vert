#version 450

layout (location = 0) in vec3 inPos;
layout (location = 1) in vec3 inNormal;
layout (location = 2) in vec2 inUV;
layout (location = 3) in vec4 inTangent;

layout (binding = 0) uniform UBO 
{
	mat4 projection;
	mat4 model;
	mat4 view;
	//mat4 normal;
	vec3 camPos;
} ubo;

layout (location = 0) out vec3 outWorldPos;
layout (location = 1) out vec3 outNormal;
layout (location = 2) out vec2 outUV;
layout (location = 3) out vec4 outTangent;

void main() 
{
	vec3 locPos = vec3(ubo.model * vec4(inPos, 1.0));
	outWorldPos = locPos;
	outNormal = mat3(ubo.model) * inNormal;
	outTangent = vec4(mat3(ubo.model) * inTangent.xyz, inTangent.w);
	outUV = inUV;
	gl_Position =  ubo.projection * ubo.view * vec4(outWorldPos, 1.0);
}
