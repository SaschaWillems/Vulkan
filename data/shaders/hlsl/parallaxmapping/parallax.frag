// Copyright 2020 Google LLC

Texture2D textureColorMap : register(t1);
SamplerState samplerColorMap : register(s1);
Texture2D textureNormalHeightMap : register(t2);
SamplerState samplerNormalHeightMap : register(s2);

struct UBO
{
	float heightScale;
	float parallaxBias;
	float numLayers;
	int mappingMode;
};

cbuffer ubo : register(b3) { UBO ubo; }

struct VSOutput
{
[[vk::location(0)]] float2 UV : TEXCOORD0;
[[vk::location(1)]] float3 TangentLightPos : TEXCOORD1;
[[vk::location(2)]] float3 TangentViewPos : TEXCOORD2;
[[vk::location(3)]] float3 TangentFragPos : TEXCOORD3;
};

float2 parallaxMapping(float2 uv, float3 viewDir)
{
	float height = 1.0 - textureNormalHeightMap.SampleLevel(samplerNormalHeightMap, uv, 0.0).a;
	float2 p = viewDir.xy * (height * (ubo.heightScale * 0.5) + ubo.parallaxBias) / viewDir.z;
	return uv - p;
}

float2 steepParallaxMapping(float2 uv, float3 viewDir)
{
	float layerDepth = 1.0 / ubo.numLayers;
	float currLayerDepth = 0.0;
	float2 deltaUV = viewDir.xy * ubo.heightScale / (viewDir.z * ubo.numLayers);
	float2 currUV = uv;
	float height = 1.0 - textureNormalHeightMap.SampleLevel(samplerNormalHeightMap, currUV, 0.0).a;
	for (int i = 0; i < ubo.numLayers; i++) {
		currLayerDepth += layerDepth;
		currUV -= deltaUV;
		height = 1.0 - textureNormalHeightMap.SampleLevel(samplerNormalHeightMap, currUV, 0.0).a;
		if (height < currLayerDepth) {
			break;
		}
	}
	return currUV;
}

float2 parallaxOcclusionMapping(float2 uv, float3 viewDir)
{
	float layerDepth = 1.0 / ubo.numLayers;
	float currLayerDepth = 0.0;
	float2 deltaUV = viewDir.xy * ubo.heightScale / (viewDir.z * ubo.numLayers);
	float2 currUV = uv;
	float height = 1.0 - textureNormalHeightMap.SampleLevel(samplerNormalHeightMap, currUV, 0.0).a;
	for (int i = 0; i < ubo.numLayers; i++) {
		currLayerDepth += layerDepth;
		currUV -= deltaUV;
		height = 1.0 - textureNormalHeightMap.SampleLevel(samplerNormalHeightMap, currUV, 0.0).a;
		if (height < currLayerDepth) {
			break;
		}
	}
	float2 prevUV = currUV + deltaUV;
	float nextDepth = height - currLayerDepth;
	float prevDepth = 1.0 - textureNormalHeightMap.SampleLevel(samplerNormalHeightMap, prevUV, 0.0).a - currLayerDepth + layerDepth;
	return lerp(currUV, prevUV, nextDepth / (nextDepth - prevDepth));
}

float4 main(VSOutput input) : SV_TARGET
{
	float3 V = normalize(input.TangentViewPos - input.TangentFragPos);
	float2 uv = input.UV;

	if (ubo.mappingMode == 0) {
		// Color only
		return textureColorMap.Sample(samplerColorMap, input.UV);
	} else {
		switch(ubo.mappingMode) {
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

		float3 N = normalize(textureNormalHeightMap.SampleLevel(samplerNormalHeightMap, uv, 0.0).rgb * 2.0 - 1.0);
		float3 L = normalize(input.TangentLightPos - input.TangentFragPos);
		float3 R = reflect(-L, N);
		float3 H = normalize(L + V);

		float3 color = textureColorMap.Sample(samplerColorMap, uv).rgb;
		float3 ambient = 0.2 * color;
		float3 diffuse = max(dot(L, N), 0.0) * color;
		float3 specular = float3(0.15, 0.15, 0.15) * pow(max(dot(N, H), 0.0), 32.0);

		return float4(ambient + diffuse + specular, 1.0f);
	}
}
