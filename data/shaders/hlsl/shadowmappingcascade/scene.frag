// Copyright 2020 Google LLC

#define SHADOW_MAP_CASCADE_COUNT 4

Texture2DArray shadowMapTexture : register(t1);
SamplerState shadowMapSampler : register(s1);
Texture2D colorMapTexture : register(t0, space1);
SamplerState colorMapSampler : register(s0, space1);

struct VSOutput
{
[[vk::location(0)]] float3 Normal : NORMAL0;
[[vk::location(1)]] float3 Color : COLOR0;
[[vk::location(2)]] float3 ViewPos : POSITION1;
[[vk::location(3)]] float3 Pos : POSITION0;
[[vk::location(4)]] float2 UV : TEXCOORD0;
};

[[vk::constant_id(0)]] const int enablePCF = 0;

#define ambient 0.3

struct UBO {
	float4 cascadeSplits;
	float4x4 cascadeViewProjMat[SHADOW_MAP_CASCADE_COUNT];
	float4x4 inverseViewMat;
	float3 lightDir;
	float _pad;
	int colorCascades;
};
cbuffer ubo : register(b2) { UBO ubo; };

static const float4x4 biasMat = float4x4(
	0.5, 0.0, 0.0, 0.5,
	0.0, 0.5, 0.0, 0.5,
	0.0, 0.0, 1.0, 0.0,
	0.0, 0.0, 0.0, 1.0
);

float textureProj(float4 shadowCoord, float2 offset, uint cascadeIndex)
{
	float shadow = 1.0;
	float bias = 0.005;

	if ( shadowCoord.z > -1.0 && shadowCoord.z < 1.0 ) {
		float dist = shadowMapTexture.Sample(shadowMapSampler, float3(shadowCoord.xy + offset, cascadeIndex)).r;
		if (shadowCoord.w > 0 && dist < shadowCoord.z - bias) {
			shadow = ambient;
		}
	}
	return shadow;

}

float filterPCF(float4 sc, uint cascadeIndex)
{
	int3 texDim;
	shadowMapTexture.GetDimensions(texDim.x, texDim.y, texDim.z);
	float scale = 0.75;
	float dx = scale * 1.0 / float(texDim.x);
	float dy = scale * 1.0 / float(texDim.y);

	float shadowFactor = 0.0;
	int count = 0;
	int range = 1;

	for (int x = -range; x <= range; x++) {
		for (int y = -range; y <= range; y++) {
			shadowFactor += textureProj(sc, float2(dx*x, dy*y), cascadeIndex);
			count++;
		}
	}
	return shadowFactor / count;
}

float4 main(VSOutput input) : SV_TARGET
{
	float4 outFragColor;
	float4 color = colorMapTexture.Sample(colorMapSampler, input.UV);
	if (color.a < 0.5) {
		clip(-1);
	}

	// Get cascade index for the current fragment's view position
	uint cascadeIndex = 0;
	for(uint i = 0; i < SHADOW_MAP_CASCADE_COUNT - 1; ++i) {
		if(input.ViewPos.z < ubo.cascadeSplits[i]) {
			cascadeIndex = i + 1;
		}
	}

	// Depth compare for shadowing
	float4 shadowCoord = mul(biasMat, mul(ubo.cascadeViewProjMat[cascadeIndex], float4(input.Pos, 1.0)));

	float shadow = 0;
	if (enablePCF == 1) {
		shadow = filterPCF(shadowCoord / shadowCoord.w, cascadeIndex);
	} else {
		shadow = textureProj(shadowCoord / shadowCoord.w, float2(0.0, 0.0), cascadeIndex);
	}

	// Directional light
	float3 N = normalize(input.Normal);
	float3 L = normalize(-ubo.lightDir);
	float3 H = normalize(L + input.ViewPos);
	float diffuse = max(dot(N, L), ambient);
	float3 lightColor = float3(1.0, 1.0, 1.0);
	outFragColor.rgb = max(lightColor * (diffuse * color.rgb), float3(0.0, 0.0, 0.0));
	outFragColor.rgb *= shadow;
	outFragColor.a = color.a;

	// Color cascades (if enabled)
	if (ubo.colorCascades == 1) {
		switch(cascadeIndex) {
			case 0 :
				outFragColor.rgb *= float3(1.0f, 0.25f, 0.25f);
				break;
			case 1 :
				outFragColor.rgb *= float3(0.25f, 1.0f, 0.25f);
				break;
			case 2 :
				outFragColor.rgb *= float3(0.25f, 0.25f, 1.0f);
				break;
			case 3 :
				outFragColor.rgb *= float3(1.0f, 1.0f, 0.25f);
				break;
		}
	}

	return outFragColor;
}
