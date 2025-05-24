/* Copyright (c) 2025, Sascha Willems
 *
 * SPDX-License-Identifier: MIT
 *
 */

import types;

Sampler2D samplerposition;
Sampler2D samplerNormal;
Sampler2D samplerAlbedo;
Sampler2D samplerSSAO;
Sampler2D samplerSSAOBlur;

struct UBO
{
	float4x4 _dummy;
	int ssao;
	int ssaoOnly;
	int ssaoBlur;
};
ConstantBuffer<UBO> uboParams;

[shader("fragment")]
float4 fragmentMain(VSOutput input)
{
    float3 fragPos = samplerposition.Sample(input.UV).rgb;
    float3 normal = normalize(samplerNormal.Sample(input.UV).rgb * 2.0 - 1.0);
    float4 albedo = samplerAlbedo.Sample(input.UV);

    float ssao = (uboParams.ssaoBlur == 1) ? samplerSSAOBlur.Sample(input.UV).r : samplerSSAO.Sample(input.UV).r;

	float3 lightPos = float3(0.0, 0.0, 0.0);
	float3 L = normalize(lightPos - fragPos);
	float NdotL = max(0.5, dot(normal, L));

	float4 outFragColor;
	if (uboParams.ssaoOnly == 1)
	{
		outFragColor.rgb = ssao.rrr;
	}
	else
	{
		float3 baseColor = albedo.rgb * NdotL;

		if (uboParams.ssao == 1)
		{
			outFragColor.rgb = ssao.rrr;

			if (uboParams.ssaoOnly != 1)
				outFragColor.rgb *= baseColor;
		}
		else
		{
			outFragColor.rgb = baseColor;
		}
	}
	return outFragColor;
}