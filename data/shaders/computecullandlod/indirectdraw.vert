#version 450

#extension GL_ARB_separate_shader_objects : enable
#extension GL_ARB_shading_language_420pack : enable

// Vertex attributes
layout (location = 0) in vec4 inPos;
layout (location = 1) in vec3 inNormal;
layout (location = 2) in vec3 inColor;

// Instanced attributes
layout (location = 4) in vec3 instancePos;
layout (location = 5) in float instanceScale;

layout (binding = 0) uniform UBO 
{
	mat4 projection;
	mat4 modelview;
} ubo;

layout (location = 0) out vec3 outNormal;
layout (location = 1) out vec3 outColor;
layout (location = 2) out vec3 outViewVec;
layout (location = 3) out vec3 outLightVec;

out gl_PerVertex
{
	vec4 gl_Position;
};

void main() 
{
	outColor = inColor;
		
	outNormal = inNormal;
	
	vec4 pos = vec4((inPos.xyz * instanceScale) + instancePos, 1.0);

	gl_Position = ubo.projection * ubo.modelview * pos;
	
	vec4 wPos = ubo.modelview * vec4(pos.xyz, 1.0); 
	vec4 lPos = vec4(0.0, 10.0, 50.0, 1.0);
	outLightVec = lPos.xyz - pos.xyz;
	outViewVec = -pos.xyz;	
}
