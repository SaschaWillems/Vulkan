/* Copyright (c) 2025, Sascha Willems
 *
 * SPDX-License-Identifier: MIT
 *
 */

struct VSOutput
{
    float4 Pos : SV_POSITION;
    float2 UV;
};

struct UBO
{
    float4x4 projection;
    float4x4 view;
    float4x4 model;
};
ConstantBuffer<UBO> ubo;

SamplerCube shadowCubeMapSampler;

[shader("vertex")]
VSOutput vertexMain(uint VertexIndex: SV_VertexID)
{
    VSOutput output;
    output.UV = float2((VertexIndex << 1) & 2, VertexIndex & 2);
    output.Pos = float4(output.UV.xy * 2.0f - 1.0f, 0.0f, 1.0f);
    return output;
}

[shader("fragment")]
float4 fragmentMain(VSOutput input)
{
	float4 outFragColor = float4(0, 0, 0, 0);
	outFragColor.rgb = float3(0.05, 0.05, 0.05);

	float3 samplePos = float3(0, 0, 0);

    // Crude statement to visualize different cube map faces based on UV coordinates
    int x = int(floor(input.UV.x / 0.25f));
    int y = int(floor(input.UV.y / (1.0 / 3.0)));
    if (y == 1) {
        float2 uv = float2(input.UV.x * 4.0f, (input.UV.y - 1.0/3.0) * 3.0);
		uv = 2.0 * float2(uv.x - float(x) * 1.0, uv.y) - 1.0;
		switch (x) {
			case 0:	// NEGATIVE_X
				samplePos = float3(-1.0f, uv.y, uv.x);
				break;
			case 1: // POSITIVE_Z
				samplePos = float3(uv.x, uv.y, 1.0f);
				break;
			case 2: // POSITIVE_X
				samplePos = float3(1.0, uv.y, -uv.x);
				break;
			case 3: // NEGATIVE_Z
				samplePos = float3(-uv.x, uv.y, -1.0f);
				break;
		}
	} else {
        if (x == 1) {
            float2 uv = float2((input.UV.x - 0.25) * 4.0, (input.UV.y - float(y) / 3.0) * 3.0);
			uv = 2.0 * uv - 1.0;
			switch (y) {
				case 0: // NEGATIVE_Y
					samplePos = float3(uv.x, -1.0f, uv.y);
					break;
				case 2: // POSITIVE_Y
					samplePos = float3(uv.x, 1.0f, -uv.y);
					break;
			}
		}
	}

    if ((samplePos.x != 0.0f) && (samplePos.y != 0.0f)) {
        float dist = length(shadowCubeMapSampler.Sample(samplePos).xyz) * 0.005;
		outFragColor = float4(dist.xxx, 1.0);
	}
	return outFragColor;
}