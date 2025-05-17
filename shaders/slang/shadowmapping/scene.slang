/* Copyright (c) 2025, Sascha Willems
 *
 * SPDX-License-Identifier: MIT
 *
 */

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
	float3 ViewVec;
	float3 LightVec;
	float4 ShadowCoord;
};

struct UBO
{
    float4x4 projection;
    float4x4 view;
    float4x4 model;
    float4x4 lightSpace;
    float4 lightPos;
    float zNear;
    float zFar;
};
ConstantBuffer<UBO> ubo;
Sampler2D shadowMapSampler;

[SpecializationConstant] const int enablePCF = 0;

#define ambient 0.1

float textureProj(float4 shadowCoord, float2 off)
{
	float shadow = 1.0;
	if ( shadowCoord.z > -1.0 && shadowCoord.z < 1.0 )
    {
        float dist = shadowMapSampler.Sample(shadowCoord.xy + off).r;
		if ( shadowCoord.w > 0.0 && dist < shadowCoord.z )
		{
			shadow = ambient;
		}
	}
	return shadow;
}

float filterPCF(float4 sc)
{
    int2 texDim;
    shadowMapSampler.GetDimensions(texDim.x, texDim.y);
	float scale = 1.5;
	float dx = scale * 1.0 / float(texDim.x);
	float dy = scale * 1.0 / float(texDim.y);

	float shadowFactor = 0.0;
	int count = 0;
	int range = 1;

	for (int x = -range; x <= range; x++)
	{
		for (int y = -range; y <= range; y++)
		{
			shadowFactor += textureProj(sc, float2(dx*x, dy*y));
			count++;
		}

	}
	return shadowFactor / count;
}

static const float4x4 biasMat = float4x4(
    0.5, 0.0, 0.0, 0.5,
    0.0, 0.5, 0.0, 0.5,
    0.0, 0.0, 1.0, 0.0,
    0.0, 0.0, 0.0, 1.0);

[shader("vertex")]
VSOutput vertexMain(VSInput input)
{
    VSOutput output = (VSOutput)0;
    output.Color = input.Color;
    output.Normal = input.Normal;

    output.Pos = mul(ubo.projection, mul(ubo.view, mul(ubo.model, float4(input.Pos.xyz, 1.0))));

    float4 pos = mul(ubo.model, float4(input.Pos, 1.0));
    output.Normal = mul((float3x3)ubo.model, input.Normal);
    output.LightVec = normalize(ubo.lightPos.xyz - input.Pos);
    output.ViewVec = -pos.xyz;

    output.ShadowCoord = mul(biasMat, mul(ubo.lightSpace, mul(ubo.model, float4(input.Pos, 1.0))));
    return output;
}

[shader("fragment")]
float4 fragmentMain(VSOutput input)
{
	float shadow = (enablePCF == 1) ? filterPCF(input.ShadowCoord / input.ShadowCoord.w) : textureProj(input.ShadowCoord / input.ShadowCoord.w, float2(0.0, 0.0));

	float3 N = normalize(input.Normal);
	float3 L = normalize(input.LightVec);
	float3 V = normalize(input.ViewVec);
	float3 R = normalize(-reflect(L, N));
	float3 diffuse = max(dot(N, L), ambient) * input.Color;

	return float4(diffuse * shadow, 1.0);
}
