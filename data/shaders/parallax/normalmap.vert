#version 450

#extension GL_ARB_separate_shader_objects : enable
#extension GL_ARB_shading_language_420pack : enable

layout (location = 0) in vec3 inPos;
layout (location = 1) in vec2 inUV;
layout (location = 2) in vec3 inNormal;
layout (location = 3) in vec3 inTangent;
layout (location = 4) in vec3 inBiTangent;

layout (binding = 0) uniform UBO 
{
	mat4 projection;
	mat4 model;
	mat4 normal;
	vec4 lightPos;
	vec4 cameraPos;
} ubo;

layout (location = 0) out vec2 outUV;
layout (location = 1) out vec3 outLightVec;
layout (location = 2) out vec3 outLightVecB;
layout (location = 3) out vec3 outSpecular;
layout (location = 4) out vec3 outEyeVec;
layout (location = 5) out vec3 outLightDir;
layout (location = 6) out vec3 outViewVec;

void main(void) 
{
    vec3 vertexPosition = vec3(ubo.model *  vec4(inPos, 1.0));
  	outLightDir = normalize(ubo.lightPos.xyz - vertexPosition);

    // Setup (t)angent-(b)inormal-(n)ormal matrix for converting
    // object coordinates into tangent space
    mat3 tbnMatrix;
    tbnMatrix[0] =  mat3(ubo.normal) * inTangent;
    tbnMatrix[1] =  mat3(ubo.normal) * inBiTangent;
    tbnMatrix[2] =  mat3(ubo.normal) * inNormal;

    outEyeVec = vec3(-vertexPosition) * tbnMatrix;

    outLightVec.xyz = vec3(ubo.lightPos.xyz - vertexPosition.xyz) * tbnMatrix;

    vec3 lightDist = ubo.lightPos.xyz - inPos.xyz;
    outLightVecB.x = dot(inTangent.xyz, lightDist);
    outLightVecB.y = dot(inBiTangent.xyz, lightDist);
    outLightVecB.z = dot(inNormal, lightDist);

    vec3 camPos = vec3(ubo.normal * ubo.cameraPos);

    vec3 camVec = camPos - inPos.xyz;
    outViewVec.x = dot(inTangent, camVec);
    outViewVec.y = dot(inBiTangent, camVec);
    outViewVec.z = dot(inNormal, camVec);

    vec3 reflectVec = reflect(-camVec, inNormal);
    vec3 outViewVec = outLightDir;
    float specIntensity = pow(max(dot(reflectVec, outViewVec), 0.0), 8.0);
    outSpecular = vec3(specIntensity * 0.3);

    outUV = inUV;
	
    gl_Position = ubo.projection * ubo.model * vec4(inPos, 1.0);
}
