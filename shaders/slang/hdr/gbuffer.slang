/* Copyright (c) 2025, Sascha Willems
 *
 * SPDX-License-Identifier: MIT
 *
 */

struct VSInput
{
    float3 Pos;
    float3 Normal;
};

struct VSOutput
{
    float4 Pos : SV_POSITION;
    float3 UVW;
    float3 WorldPos;
    float3 Normal;
    float3 ViewVec;
    float3 LightVec;
};

struct FSOutput
{
	float4 Color0 : SV_TARGET0;
	float4 Color1 : SV_TARGET1;
};

struct UBO  {
	float4x4 projection;
	float4x4 modelview;
	float4x4 inverseModelview;
	float exposure;
};
ConstantBuffer<UBO> ubo;

SamplerCube samplerEnvMap;

[[SpecializationConstant]] const int objectType = 0;

[shader("vertex")]
VSOutput vertexMain(VSInput input)
{
    VSOutput output;
    output.UVW = input.Pos;

    switch (objectType) {
		case 0: // Skybox
			output.WorldPos = mul((float4x3)ubo.modelview, input.Pos).xyz;
			output.Pos = mul(ubo.projection, float4(output.WorldPos, 1.0));
			break;
		case 1: // Object
			output.WorldPos = mul(ubo.modelview, float4(input.Pos, 1.0)).xyz;
			output.Pos = mul(ubo.projection, mul(ubo.modelview, float4(input.Pos.xyz, 1.0)));
            break;
	}
    output.WorldPos = mul(ubo.modelview, float4(input.Pos, 1.0)).xyz;
    output.Normal = mul((float4x3)ubo.modelview, input.Normal).xyz;
    float3 lightPos = float3(0.0f, -5.0f, 5.0f);
    output.LightVec = lightPos.xyz - output.WorldPos.xyz;
    output.ViewVec = -output.WorldPos.xyz;
    return output;
}

[shader("fragment")]
FSOutput fragmentMain(VSOutput input)
{
	FSOutput output;
	float4 color;
	float3 wcNormal;

    switch (objectType) {
		case 0: // Skybox
			{
                float3 normal = normalize(input.UVW);
                color = samplerEnvMap.Sample(normal);
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

				float spec = (fresnel * geoAtt) / (NdotV * NdotL * float.getPi());

                color = samplerEnvMap.Sample(reflect(-wViewVec, wNormal));

				color = float4(color.rgb * NdotL * (k + spec * (1.0 - k)), 1.0);
			}
			break;

		case 2: // Refract
			{
				float3 wViewVec = mul((float4x3)ubo.inverseModelview, normalize(input.ViewVec)).xyz;
                float3 wNormal = mul((float4x3)ubo.inverseModelview, input.Normal).xyz;
                color = samplerEnvMap.Sample(refract(-wViewVec, wNormal, 1.0/1.6));
			}
			break;
	}


	// Color with manual exposure into attachment 0
	output.Color0.rgb = float3(1.0, 1.0, 1.0) - exp(-color.rgb * ubo.exposure);

	// Bright parts for bloom into attachment 1
	float l = dot(output.Color0.rgb, float3(0.2126, 0.7152, 0.0722));
	float threshold = 0.75;
	output.Color1.rgb = (l > threshold) ? output.Color0.rgb : float3(0.0, 0.0, 0.0);
	output.Color1.a = 1.0;
	return output;
}