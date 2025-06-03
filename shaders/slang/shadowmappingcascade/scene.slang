/* Copyright (c) 2025, Sascha Willems
 *
 * SPDX-License-Identifier: MIT
 *
 */

#define SHADOW_MAP_CASCADE_COUNT 4

struct VSInput
{
    float3 Pos;
    float2 UV;
    float3 Color;
    float3 Normal;
};

struct VSOutput
{
    float4 Pos : SV_POSITION;
    float3 Normal;
    float3 Color;
    float3 ViewPos;
    float3 WorldPos;
    float2 UV;
};

[SpecializationConstant] const int enablePCF = 0;

#define ambient 0.3

struct UBOScene {
    float4x4 projection;
    float4x4 view;
    float4x4 model;
};
ConstantBuffer<UBOScene> uboScene;

[[vk::binding(1, 0)]] Sampler2DArray shadowMapSampler;

struct UBOCascades {
	float4 cascadeSplits;
	float4x4 inverseViewMat;
	float3 lightDir;
	float _pad;
	int colorCascades;
};
[[vk::binding(2, 0)]] ConstantBuffer<UBOCascades> uboCascades;

struct CVPM  {
	float4x4 matrices[SHADOW_MAP_CASCADE_COUNT];
};
[[vk::binding(3, 0)]] ConstantBuffer<CVPM> cascadeViewProjMatrices;

[[vk::binding(0, 1)]] Sampler2D colorMapSampler;

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

    if (shadowCoord.z > -1.0 && shadowCoord.z < 1.0) {
        float dist = shadowMapSampler.Sample(float3(shadowCoord.xy + offset, cascadeIndex)).r;
		if (shadowCoord.w > 0 && dist < shadowCoord.z - bias) {
			shadow = ambient;
		}
	}
	return shadow;
}

float filterPCF(float4 sc, uint cascadeIndex)
{
    int3 texDim;
    shadowMapSampler.GetDimensions(texDim.x, texDim.y, texDim.z);
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

[shader("vertex")]
VSOutput vertexMain(VSInput input, uniform float4 meshPosition)
{
    VSOutput output;
    output.Color = input.Color;
    output.Normal = input.Normal;
    output.UV = input.UV;
    float3 pos = input.Pos + meshPosition.xyz;
    output.WorldPos = pos;
    output.ViewPos = mul(uboScene.view, float4(pos.xyz, 1.0)).xyz;
    output.Pos = mul(uboScene.projection, mul(uboScene.view, mul(uboScene.model, float4(pos.xyz, 1.0))));
    return output;
}

[shader("fragment")]
float4 fragmentMain(VSOutput input)
{
    float4 outFragColor;
    float4 color = colorMapSampler.Sample(input.UV);
	if (color.a < 0.5) {
		clip(-1);
	}

	// Get cascade index for the current fragment's view position
	uint cascadeIndex = 0;
    for (uint i = 0; i < SHADOW_MAP_CASCADE_COUNT - 1; ++i) {
        if (input.ViewPos.z < uboCascades.cascadeSplits[i]) {
			cascadeIndex = i + 1;
		}
	}

    // Depth compare for shadowing
    float4 shadowCoord = mul(biasMat, mul(cascadeViewProjMatrices.matrices[cascadeIndex], float4(input.WorldPos, 1.0)));

	float shadow = 0;
	if (enablePCF == 1) {
		shadow = filterPCF(shadowCoord / shadowCoord.w, cascadeIndex);
	} else {
		shadow = textureProj(shadowCoord / shadowCoord.w, float2(0.0, 0.0), cascadeIndex);
	}

	// Directional light
	float3 N = normalize(input.Normal);
	float3 L = normalize(-uboCascades.lightDir);
	float3 H = normalize(L + input.ViewPos);
	float diffuse = max(dot(N, L), ambient);
	float3 lightColor = float3(1.0, 1.0, 1.0);
	outFragColor.rgb = max(lightColor * (diffuse * color.rgb), float3(0.0, 0.0, 0.0));
	outFragColor.rgb *= shadow;
	outFragColor.a = color.a;

	// Color cascades (if enabled)
	if (uboCascades.colorCascades == 1) {
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
