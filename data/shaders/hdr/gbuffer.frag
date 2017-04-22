#version 450

#extension GL_ARB_separate_shader_objects : enable
#extension GL_ARB_shading_language_420pack : enable

layout (binding = 1) uniform samplerCube samplerEnvMap;

layout (location = 0) in vec3 inUVW;
layout (location = 1) in vec3 inPos;
layout (location = 2) in vec3 inNormal;
layout (location = 3) in vec3 inViewVec;
layout (location = 4) in vec3 inLightVec;
layout (location = 5) in mat4 inInvModelView;

layout (location = 0) out vec4 outColor0;
layout (location = 1) out vec4 outColor1;

layout (constant_id = 0) const int type = 0;

#define PI 3.1415926
#define TwoPI (2.0 * PI)

layout (binding = 2) uniform UBO {
	float exposure;
} ubo;

void main() 
{
	vec4 color;
	vec3 wcNormal;

	switch (type) {
		case 0: // Skybox			
			{
				vec3 normal = normalize(inUVW);
				color = texture(samplerEnvMap, normal);
			}
			break;
		
		case 1: // Reflect
			{
				vec3 wViewVec = mat3(inInvModelView) * normalize(inViewVec);
				vec3 normal = normalize(inNormal);
				vec3 wNormal = mat3(inInvModelView) * normal;

				float NdotL = max(dot(normal, inLightVec), 0.0);

				vec3 eyeDir = normalize(inViewVec);		
				vec3 halfVec = normalize(inLightVec + eyeDir);
				float NdotH = max(dot(normal, halfVec), 0.0); 
				float NdotV = max(dot(normal, eyeDir), 0.0); 
				float VdotH = max(dot(eyeDir, halfVec), 0.0);
		
				// Geometric attenuation
				float NH2 = 2.0 * NdotH;
				float g1 = (NH2 * NdotV) / VdotH;
				float g2 = (NH2 * NdotL) / VdotH;
				float geoAtt = min(1.0, min(g1, g2));

				const float F0 = 0.6;
				const float k = 0.2;

				// Fresnel (schlick approximation)
				float fresnel = pow(1.0 - VdotH, 5.0);
				fresnel *= (1.0 - F0);
				fresnel += F0;
		
				float spec = (fresnel * geoAtt) / (NdotV * NdotL * 3.14);
 
				color = texture(samplerEnvMap, reflect(-wViewVec, wNormal));	 	

				color = vec4(color.rgb * NdotL * (k + spec * (1.0 - k)), 1.0);
			}
			break;

		case 2: // Refract			
			{
				vec3 wViewVec = mat3(inInvModelView) * normalize(inViewVec);
				vec3 wNormal = mat3(inInvModelView) * inNormal;
				color = texture(samplerEnvMap, refract(-wViewVec, wNormal, 1.0/1.6));		
			}
			break;
	}


	// Color with manual exposure into attachment 0
	outColor0.rgb = vec3(1.0) - exp(-color.rgb * ubo.exposure);

	// Bright parts for bloom into attachment 1
	float l = dot(outColor0.rgb, vec3(0.2126, 0.7152, 0.0722));
	float threshold = 0.75;
	outColor1.rgb = (l > threshold) ? outColor0.rgb : vec3(0.0);
	outColor1.a = 1.0;
}