#version 450

layout (location = 0) in vec3 inPos;
layout (location = 1) in vec3 inNormal;
layout (location = 2) in vec2 inUV;
layout (location = 3) in vec3 inColor;

layout (set = 0, binding = 0) uniform UBOMatrices {
	mat4 projection;
	mat4 view;
	mat4 model;
} uboMatrices;

layout (set = 0, binding = 1) uniform UniformInline {
	vec4 color;
} uniformInline;

layout (location = 0) out vec3 outNormal;
layout (location = 1) out vec3 outColor;
layout (location = 2) out vec2 outUV;
layout (location = 3) out vec3 outViewVec;
layout (location = 4) out vec3 outLightVec;

out gl_PerVertex {
	vec4 gl_Position;
};

void main() 
{
	outColor = inColor * uniformInline.color.rgb;
	outUV = inUV;
	gl_Position = uboMatrices.projection * uboMatrices.view * uboMatrices.model * vec4(inPos.xyz, 1.0);

	vec4 pos = uboMatrices.model * vec4(inPos, 1.0);
	outNormal = mat3(transpose(inverse(uboMatrices.model))) * normalize(inNormal);
	vec3 lightPos = vec3(0.0f, -25.0f, 25.0f);
	vec3 lPos = mat3(uboMatrices.model) * lightPos.xyz;
	outLightVec = lPos - pos.xyz;
	outViewVec = -pos.xyz;	
}