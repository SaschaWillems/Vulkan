#version 450

layout (location = 0) in vec3 inPos;
layout (location = 1) in vec3 inColor;
layout (location = 2) in vec3 inNormal;

layout (binding = 0) uniform UBO {
	mat4 projection;
	mat4 model;
	mat4 view;
} ubo;

layout (location = 0) out vec3 outColor;
layout (location = 1) out vec3 outNormal;
layout (location = 2) out vec3 outViewVec;
layout (location = 3) out vec3 outLightVec;

out gl_PerVertex {
	vec4 gl_Position;
};

void main() 
{
	gl_Position = ubo.projection * ubo.view * ubo.model * vec4(inPos, 1.0);
	outColor = inColor;
	outNormal = inNormal;
	outLightVec = vec3(0.0f, 5.0f, 15.0f) - inPos;
	outViewVec = -inPos.xyz;		
}
