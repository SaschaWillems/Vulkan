/* Copyright (c) 2025, Sascha Willems
 *
 * SPDX-License-Identifier: MIT
 *
 */

struct VSInput
{
    float3 Pos;
    float2 UV;
};


struct VSOutput
{
    float4 Pos : SV_POSITION;
    float2 UV;
};

struct UBO
{
    float4x4 projection;
    float4x4 model;
    float4 outlineColor;
    float outlineWidth;
    float outline;
};
ConstantBuffer<UBO> ubo;
Sampler2D samplerColor;

[shader("vertex")]
VSOutput vertexMain(VSInput input)
{
    VSOutput output;
    output.UV = input.UV;
    output.Pos = mul(ubo.projection, mul(ubo.model, float4(input.Pos.xyz, 1.0)));
    return output;
}

[shader("fragment")]
float4 fragmentMain(VSOutput input)
{
    float dist = samplerColor.Sample(input.UV).a;
    float smoothWidth = fwidth(dist);
    float alpha = smoothstep(0.5 - smoothWidth, 0.5 + smoothWidth, dist);
	float3 rgb = alpha.xxx;

	if (ubo.outline > 0.0)
	{
		float w = 1.0 - ubo.outlineWidth;
		alpha = smoothstep(w - smoothWidth, w + smoothWidth, dist);
        rgb += lerp(alpha.xxx, ubo.outlineColor.rgb, alpha);
    }

    return float4(rgb, alpha);
}