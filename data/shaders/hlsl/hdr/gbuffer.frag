// Copyright 2020 Google LLC

TextureCube textureEnvMap : register(t1);
SamplerState samplerEnvMap : register(s1);

struct VSOutput
{
[[vk::location(0)]] float3 UVW : TEXCOORD0;
[[vk::location(1)]] float3 Pos : POSITION0;
[[vk::location(2)]] float3 Normal : NORMAL0;
[[vk::location(3)]] float3 ViewVec : TEXCOORD1;
[[vk::location(4)]] float3 LightVec : TEXCOORD2;
};

struct FSOutput
{
	float4 Color0 : SV_TARGET0;
	float4 Color1 : SV_TARGET1;
};

[[vk::constant_id(0)]] const int type = 0;

#define PI 3.1415926
#define TwoPI (2.0 * PI)

struct UBO  {
	float4x4 projection;
	float4x4 modelview;
	float4x4 inverseModelview;
};

cbuffer ubo : register(b0) { UBO ubo; }

cbuffer Exposure : register(b2)
{
	float exposure;
}

FSOutput main(VSOutput input)
{
	FSOutput output = (FSOutput)0;
	float4 color;
	float3 wcNormal;

	switch (type) {
		case 0: // Skybox
			{
				float3 normal = normalize(input.UVW);
				color = textureEnvMap.Sample(samplerEnvMap, normal);
			}
			break;

		case 1: // Reflect
			{
				float3 wViewVec = mul((float4x3)ubo.inverseModelview, normalize(input.ViewVec)).xyz;
				float3 normal = normalize(input.Normal);
				float3 wNormal = mul((float4x3)ubo.inverseModelview, normal).xyz;

				float NdotL = max(dot(normal, input.LightVec), 0.0);

				float3 eyeDir = normalize(input.ViewVec);
				float3 halfVec = normalize(input.LightVec + eyeDir);
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

				color = textureEnvMap.Sample(samplerEnvMap, reflect(-wViewVec, wNormal));

				color = float4(color.rgb * NdotL * (k + spec * (1.0 - k)), 1.0);
			}
			break;

		case 2: // Refract
			{
				float3 wViewVec = mul((float4x3)ubo.inverseModelview, normalize(input.ViewVec)).xyz;
				float3 wNormal = mul((float4x3)ubo.inverseModelview, input.Normal).xyz;
				color = textureEnvMap.Sample(samplerEnvMap, refract(-wViewVec, wNormal, 1.0/1.6));
			}
			break;
	}


	// Color with manual exposure into attachment 0
	output.Color0.rgb = float3(1.0, 1.0, 1.0) - exp(-color.rgb * exposure);

	// Bright parts for bloom into attachment 1
	float l = dot(output.Color0.rgb, float3(0.2126, 0.7152, 0.0722));
	float threshold = 0.75;
	output.Color1.rgb = (l > threshold) ? output.Color0.rgb : float3(0.0, 0.0, 0.0);
	output.Color1.a = 1.0;
	return output;
}