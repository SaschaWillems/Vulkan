#version 450

layout (binding = 0) uniform UBO 
{
	mat4 projection;
	mat4 modelview;
	vec4 lightPos;
	float tessAlpha;
	float tessStrength;
	float tessLevel;
} ubo; 

layout (binding = 1) uniform sampler2D displacementMap; 

layout(triangles, equal_spacing, cw) in;

layout (location = 0) in vec3 inNormal[];
layout (location = 1) in vec2 inUV[];
 
layout (location = 0) out vec3 outNormal;
layout (location = 1) out vec2 outUV;
layout (location = 2) out vec3 outEyesPos;
layout (location = 3) out vec3 outLightVec;

void main()
{
	gl_Position = (gl_TessCoord.x * gl_in[0].gl_Position) + (gl_TessCoord.y * gl_in[1].gl_Position) + (gl_TessCoord.z * gl_in[2].gl_Position); 
	outUV = gl_TessCoord.x * inUV[0] + gl_TessCoord.y * inUV[1] + gl_TessCoord.z * inUV[2];
	outNormal = gl_TessCoord.x * inNormal[0] + gl_TessCoord.y * inNormal[1] + gl_TessCoord.z * inNormal[2]; 
				
	gl_Position.xyz += normalize(outNormal) * (max(textureLod(displacementMap, outUV.st, 0.0).a, 0.0) * ubo.tessStrength);
				
	outEyesPos = (gl_Position).xyz;
	outLightVec = normalize(ubo.lightPos.xyz - outEyesPos);	
		
	gl_Position = ubo.projection * ubo.modelview * gl_Position;
}