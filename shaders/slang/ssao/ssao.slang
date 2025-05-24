/* Copyright (c) 2025, Sascha Willems
 *
 * SPDX-License-Identifier: MIT
 *
 */

import types;

Sampler2D samplerPositionDepth;
Sampler2D samplerNormal;
Sampler2D ssaoNoiseSampler;

struct UBOSSAOKernel
{
	float4 samples[64];
};
ConstantBuffer<UBOSSAOKernel> uboSSAOKernel;

struct UBO
{
	float4x4 projection;
};
ConstantBuffer<UBO> ubo;

[[SpecializationConstant]] const int SSAO_KERNEL_SIZE = 64;
[[SpecializationConstant]] const float SSAO_RADIUS = 0.5;

[shader("fragment")]
float fragmentMain(VSOutput input)
{
    // Get G-Buffer values
    float3 fragPos = samplerPositionDepth.Sample(input.UV).rgb;
    float3 normal = normalize(samplerNormal.Sample(input.UV).rgb * 2.0 - 1.0);

	// Get a random vector using a noise lookup
    int2 texDim;
    samplerPositionDepth.GetDimensions(texDim.x, texDim.y);
    int2 noiseDim;
    ssaoNoiseSampler.GetDimensions(noiseDim.x, noiseDim.y);
    const float2 noiseUV = float2(float(texDim.x) / float(noiseDim.x), float(texDim.y) / (noiseDim.y)) * input.UV;
    float3 randomVec = ssaoNoiseSampler.Sample(noiseUV).xyz * 2.0 - 1.0;

	// Create TBN matrix
	float3 tangent = normalize(randomVec - normal * dot(randomVec, normal));
	float3 bitangent = cross(tangent, normal);
	float3x3 TBN = transpose(float3x3(tangent, bitangent, normal));

	// Calculate occlusion value
	float occlusion = 0.0f;
	for(int i = 0; i < SSAO_KERNEL_SIZE; i++)
	{
		float3 samplePos = mul(TBN, uboSSAOKernel.samples[i].xyz);
		samplePos = fragPos + samplePos * SSAO_RADIUS;

		// project
		float4 offset = float4(samplePos, 1.0f);
		offset = mul(ubo.projection, offset);
		offset.xyz /= offset.w;
		offset.xyz = offset.xyz * 0.5f + 0.5f;

        float sampleDepth = -samplerPositionDepth.Sample(offset.xy).w;

		float rangeCheck = smoothstep(0.0f, 1.0f, SSAO_RADIUS / abs(fragPos.z - sampleDepth));
		occlusion += (sampleDepth >= samplePos.z ? 1.0f : 0.0f) * rangeCheck;
	}
	occlusion = 1.0 - (occlusion / float(SSAO_KERNEL_SIZE));

	return occlusion;
}

