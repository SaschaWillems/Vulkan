#version 450

#define SHADOW_MAP_CASCADE_COUNT 4

layout (set = 0, binding = 1) uniform sampler2DArray shadowMap;
layout (set = 1, binding = 0) uniform sampler2D colorMap;

layout (location = 0) in vec3 inNormal;
layout (location = 1) in vec3 inColor;
layout (location = 2) in vec3 inViewPos;
layout (location = 3) in vec3 inPos;
layout (location = 4) in vec2 inUV;

layout (constant_id = 0) const int enablePCF = 0;

layout (location = 0) out vec4 outFragColor;

#define ambient 0.3

layout (set = 0, binding = 2) uniform UBO {
	vec4 cascadeSplits;
	mat4 cascadeViewProjMat[SHADOW_MAP_CASCADE_COUNT];
	mat4 inverseViewMat;
	vec3 lightDir;
	float _pad;
	int colorCascades;
} ubo;

const mat4 biasMat = mat4( 
	0.5, 0.0, 0.0, 0.0,
	0.0, 0.5, 0.0, 0.0,
	0.0, 0.0, 1.0, 0.0,
	0.5, 0.5, 0.0, 1.0 
);

float textureProj(vec4 P, vec2 offset, uint cascadeIndex)
{
	float shadow = 1.0;
	float bias = 0.005;

	vec4 shadowCoord = P / P.w;
	if ( shadowCoord.z > -1.0 && shadowCoord.z < 1.0 ) {
		float dist = texture(shadowMap, vec3(shadowCoord.st + offset, cascadeIndex)).r;
		if (shadowCoord.w > 0 && dist < shadowCoord.z - bias) {
			shadow = ambient;
		}
	}
	return shadow;

}

float filterPCF(vec4 sc, uint cascadeIndex)
{
	ivec2 texDim = textureSize(shadowMap, 0).xy;
	float scale = 0.75;
	float dx = scale * 1.0 / float(texDim.x);
	float dy = scale * 1.0 / float(texDim.y);

	float shadowFactor = 0.0;
	int count = 0;
	int range = 1;
	
	for (int x = -range; x <= range; x++) {
		for (int y = -range; y <= range; y++) {
			shadowFactor += textureProj(sc, vec2(dx*x, dy*y), cascadeIndex);
			count++;
		}
	}
	return shadowFactor / count;
}

void main() 
{	
	vec4 color = texture(colorMap, inUV);
	if (color.a < 0.5) {
		discard;
	}

	// Get cascade index for the current fragment's view position
	uint cascadeIndex = 0;
	for(uint i = 0; i < SHADOW_MAP_CASCADE_COUNT - 1; ++i) {
		if(inViewPos.z < ubo.cascadeSplits[i]) {	
			cascadeIndex = i + 1;
		}
	}

	// Depth compare for shadowing
	vec4 shadowCoord = (biasMat * ubo.cascadeViewProjMat[cascadeIndex]) * vec4(inPos, 1.0);	

	float shadow = 0;
	if (enablePCF == 1) {
		shadow = filterPCF(shadowCoord / shadowCoord.w, cascadeIndex);
	} else {
		shadow = textureProj(shadowCoord / shadowCoord.w, vec2(0.0), cascadeIndex);
	}

	// Directional light
	vec3 N = normalize(inNormal);
	vec3 L = normalize(-ubo.lightDir);
	vec3 H = normalize(L + inViewPos);
	float diffuse = max(dot(N, L), ambient);
	vec3 lightColor = vec3(1.0);
	outFragColor.rgb = max(lightColor * (diffuse * color.rgb), vec3(0.0));
	outFragColor.rgb *= shadow;
	outFragColor.a = color.a;

	// Color cascades (if enabled)
	if (ubo.colorCascades == 1) {
		switch(cascadeIndex) {
			case 0 : 
				outFragColor.rgb *= vec3(1.0f, 0.25f, 0.25f);
				break;
			case 1 : 
				outFragColor.rgb *= vec3(0.25f, 1.0f, 0.25f);
				break;
			case 2 : 
				outFragColor.rgb *= vec3(0.25f, 0.25f, 1.0f);
				break;
			case 3 : 
				outFragColor.rgb *= vec3(1.0f, 1.0f, 0.25f);
				break;
		}
	}
}