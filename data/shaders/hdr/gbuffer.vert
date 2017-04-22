#version 450

#extension GL_ARB_separate_shader_objects : enable
#extension GL_ARB_shading_language_420pack : enable

layout (location = 0) in vec3 inPos;
layout (location = 1) in vec3 inNormal;

layout (constant_id = 0) const int type = 0;

layout (binding = 0) uniform UBO {
	mat4 projection;
	mat4 modelview;
} ubo;

layout (location = 0) out vec3 outUVW;
layout (location = 1) out vec3 outPos;
layout (location = 2) out vec3 outNormal;
layout (location = 3) out vec3 outViewVec;
layout (location = 4) out vec3 outLightVec;
layout (location = 5) out mat4 outInvModelView;

out gl_PerVertex 
{
	vec4 gl_Position;
};

void main() 
{
	outUVW = inPos;

	switch(type) {
		case 0: // Skybox
			outPos = vec3(mat3(ubo.modelview) * inPos);
			gl_Position = vec4(ubo.projection * vec4(outPos, 1.0));
			break;
		case 1: // Object
			outPos = vec3(ubo.modelview * vec4(inPos, 1.0));
			gl_Position = ubo.projection * ubo.modelview * vec4(inPos.xyz, 1.0);
			break;
	}
	outPos = vec3(ubo.modelview * vec4(inPos, 1.0));
	outNormal = mat3(ubo.modelview) * inNormal;	
	
	outInvModelView = inverse(ubo.modelview);

	vec3 lightPos = vec3(0.0f, -5.0f, 5.0f);
	outLightVec = lightPos.xyz - outPos.xyz;
	outViewVec = -outPos.xyz;		
}
