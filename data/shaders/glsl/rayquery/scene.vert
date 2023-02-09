#version 450

layout (location = 0) in vec3 inPos;
layout (location = 1) in vec2 inUV;
layout (location = 2) in vec3 inColor;
layout (location = 3) in vec3 inNormal;

layout (binding = 0) uniform UBO 
{
	mat4 projection;
	mat4 view;
	mat4 model;
	vec3 lightPos;
} ubo;

layout (location = 0) out vec3 outNormal;
layout (location = 1) out vec3 outColor;
layout (location = 2) out vec3 outViewVec;
layout (location = 3) out vec3 outLightVec;
layout (location = 4) out vec3 outWorldPos;

void main() 
{
	outColor = inColor;
	outNormal = inNormal;
	gl_Position = ubo.projection * ubo.view * ubo.model * vec4(inPos.xyz, 1.0);
    vec4 pos = ubo.model * vec4(inPos, 1.0);
	outWorldPos = vec3(ubo.model * vec4(inPos, 1.0));
    outNormal = mat3(ubo.model) * inNormal;
    outLightVec = normalize(ubo.lightPos - inPos);
    outViewVec = -pos.xyz;
}

