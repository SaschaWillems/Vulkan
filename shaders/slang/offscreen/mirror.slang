/* Copyright (c) 2025, Sascha Willems
 *
 * SPDX-License-Identifier: MIT
 *
 */

struct VSInput
{
	float3 Pos;
};

struct VSOutput
{
    float4 Pos : SV_POSITION;
    float4 ProjCoord;
};

struct UBO
{
    float4x4 projection;
    float4x4 view;
    float4x4 model;
};
ConstantBuffer<UBO> ubo;

Sampler2D samplerColor;

[shader("vertex")]
VSOutput vertexMain(VSInput input)
{
    VSOutput output;
    output.ProjCoord = mul(ubo.projection, mul(ubo.view, mul(ubo.model, float4(input.Pos.xyz, 1.0))));
    output.Pos = output.ProjCoord;
    return output;
}

[shader("fragment")]
float4 fragmentMain(VSOutput input, bool FrontFacing : SV_IsFrontFace)
{
	float4 tmp = (1.0 / input.ProjCoord.w).xxxx;
	float4 projCoord = input.ProjCoord * tmp;

	// Scale and bias
	projCoord += float4(1.0, 1.0, 1.0, 1.0);
	projCoord *= float4(0.5, 0.5, 0.5, 0.5);

	// Slow single pass blur
	// For demonstration purposes only
	const float blurSize = 1.0 / 512.0;

	float4 color = float4(float3(0.0, 0.0, 0.0), 1.);

	if (FrontFacing)
	{
		// Only render mirrored scene on front facing (upper) side of mirror surface
		float4 reflection = float4(0.0, 0.0, 0.0, 0.0);
		for (int x = -3; x <= 3; x++)
		{
			for (int y = -3; y <= 3; y++)
            {
                reflection += samplerColor.Sample(float2(projCoord.x + x * blurSize, projCoord.y + y * blurSize)) / 49.0;
			}
		}
		color += reflection;
	}

	return color;
}