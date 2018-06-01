#version 450

#extension GL_EXT_multiview : enable

layout (location = 0) in vec3 inPos;
layout (location = 1) in vec3 inNormal;
layout (location = 2) in vec3 inColor;

layout (location = 0) out vec3 outNormal;
layout (location = 1) out vec3 outColor;
layout (location = 2) out vec3 outViewVec;
layout (location = 3) out vec3 outLightVec;


layout (binding = 0) uniform UBO 
{
	mat4 projection[2];
	mat4 modelview[2];
	vec4 lightPos;
} ubo;

out gl_PerVertex
{
	vec4 gl_Position;
};

void main() 
{
	outColor = inColor;
	outNormal = mat3(ubo.modelview[gl_ViewIndex]) * inNormal;

	vec4 pos = vec4(inPos.xyz, 1.0);
	vec4 worldPos = ubo.modelview[gl_ViewIndex] * pos;
		
	vec3 lPos = vec3(ubo.modelview[gl_ViewIndex] * ubo.lightPos);
	outLightVec = lPos - worldPos.xyz;
	outViewVec = -worldPos.xyz;	

	gl_Position = ubo.projection[gl_ViewIndex] * worldPos;
}
