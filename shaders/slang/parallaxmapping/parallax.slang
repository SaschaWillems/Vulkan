/* Copyright (c) 2025, Sascha Willems
 *
 * SPDX-License-Identifier: MIT
 *
 */

struct UBOScene
{
    float4x4 projection;
    float4x4 view;
    float4x4 model;
    float4 lightPos;
    float4 cameraPos;
};
ConstantBuffer<UBOScene> uboScene;

Sampler2D samplerColorMap;
Sampler2D samplerNormalHeightMap;

struct UBOParams
{
	float heightScale;
	float parallaxBias;
	float numLayers;
	int mappingMode;
};
ConstantBuffer<UBOParams> uboParams;

struct VSInput
{
    float3 Pos;
    float2 UV;
    float3 Normal;
    float4 Tangent;
};

struct VSOutput
{
    float4 Pos : SV_POSITION;
    float2 UV;
    float3 TangentLightPos;
    float3 TangentViewPos;
    float3 TangentFragPos;
};

float2 parallaxMapping(float2 uv, float3 viewDir)
{
    float height = 1.0 - samplerNormalHeightMap.SampleLevel(uv, 0.0).a;
    float2 p = viewDir.xy * (height * (uboParams.heightScale * 0.5) + uboParams.parallaxBias) / viewDir.z;
	return uv - p;
}

float2 steepParallaxMapping(float2 uv, float3 viewDir)
{
    float layerDepth = 1.0 / uboParams.numLayers;
    float currLayerDepth = 0.0;
    float2 deltaUV = viewDir.xy * uboParams.heightScale / (viewDir.z * uboParams.numLayers);
    float2 currUV = uv;
    float height = 1.0 - samplerNormalHeightMap.SampleLevel(currUV, 0.0).a;
    for (int i = 0; i < uboParams.numLayers; i++) {
		currLayerDepth += layerDepth;
        currUV -= deltaUV;
        height = 1.0 - samplerNormalHeightMap.SampleLevel(currUV, 0.0).a;
		if (height < currLayerDepth) {
			break;
		}
	}
	return currUV;
}

float2 parallaxOcclusionMapping(float2 uv, float3 viewDir)
{
    float layerDepth = 1.0 / uboParams.numLayers;
    float currLayerDepth = 0.0;
    float2 deltaUV = viewDir.xy * uboParams.heightScale / (viewDir.z * uboParams.numLayers);
    float2 currUV = uv;
    float height = 1.0 - samplerNormalHeightMap.SampleLevel(currUV, 0.0).a;
    for (int i = 0; i < uboParams.numLayers; i++) {
		currLayerDepth += layerDepth;
        currUV -= deltaUV;
        height = 1.0 - samplerNormalHeightMap.SampleLevel(currUV, 0.0).a;
		if (height < currLayerDepth) {
			break;
		}
	}
	float2 prevUV = currUV + deltaUV;
    float nextDepth = height - currLayerDepth;
    float prevDepth = 1.0 - samplerNormalHeightMap.SampleLevel(prevUV, 0.0).a - currLayerDepth + layerDepth;
	return lerp(currUV, prevUV, nextDepth / (nextDepth - prevDepth));
}

[shader("vertex")]

VSOutput vertexMain(VSInput input)
{
    VSOutput output;
    output.Pos = mul(uboScene.projection, mul(uboScene.view, mul(uboScene.model, float4(input.Pos, 1.0f))));
    output.UV = input.UV;

    float3 N = normalize(input.Normal);
    float3 T = normalize(input.Tangent.xyz);
    float3 B = normalize(cross(N, T));
    float3x3 TBN = float3x3(T, B, N);

    output.TangentLightPos = mul(TBN, uboScene.lightPos.xyz);
    output.TangentViewPos = mul(TBN, uboScene.cameraPos.xyz);
    output.TangentFragPos = mul(TBN, mul(uboScene.model, float4(input.Pos, 1.0)).xyz);
    return output;
}

[shader("fragment")]
float4 fragmentMain(VSOutput input)
{
	float3 V = normalize(input.TangentViewPos - input.TangentFragPos);
	float2 uv = input.UV;

    if (uboParams.mappingMode == 0) {
        // Color only
        return samplerColorMap.Sample(input.UV);
    } else {
        switch (uboParams.mappingMode) {
			case 2:
				uv = parallaxMapping(input.UV, V);
				break;
			case 3:
				uv = steepParallaxMapping(input.UV, V);
				break;
			case 4:
				uv = parallaxOcclusionMapping(input.UV, V);
				break;
		}

		// Discard fragments at texture border
		if (uv.x < 0.0 || uv.x > 1.0 || uv.y < 0.0 || uv.y > 1.0) {
			clip(-1);
		}

        float3 N = normalize(samplerNormalHeightMap.SampleLevel(uv, 0.0).rgb * 2.0 - 1.0);
		float3 L = normalize(input.TangentLightPos - input.TangentFragPos);
		float3 R = reflect(-L, N);
		float3 H = normalize(L + V);

        float3 color = samplerColorMap.Sample(uv).rgb;
		float3 ambient = 0.2 * color;
		float3 diffuse = max(dot(L, N), 0.0) * color;
		float3 specular = float3(0.15, 0.15, 0.15) * pow(max(dot(N, H), 0.0), 32.0);

		return float4(ambient + diffuse + specular, 1.0f);
	}
}
