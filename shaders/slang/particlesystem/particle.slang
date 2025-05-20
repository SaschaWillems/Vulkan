/* Copyright (c) 2025, Sascha Willems
 *
 * SPDX-License-Identifier: MIT
 *
 */

struct VSInput
{
	float4 Pos;
	float4 Color;
	float Alpha;
	float Size;
	float Rotation;
	int Type;
};

struct VSOutput
{
    float4 Pos : SV_POSITION;
    float PSize : SV_PointSize;
    float4 Color;
    float Alpha;
    int Type;
    float Rotation;
    float2 CenterPos;
    float PointSize;
};

struct UBO
{
    float4x4 projection;
    float4x4 modelview;
    float2 viewportDim;
    float pointSize;
};
ConstantBuffer<UBO> ubo;

Sampler2D samplerSmoke;
Sampler2D samplerFire;

[shader("vertex")]
VSOutput vertexMain(VSInput input)
{
    VSOutput output;
    output.Color = input.Color;
    output.Alpha = input.Alpha;
    output.Type = input.Type;
    output.Rotation = input.Rotation;

    output.Pos = mul(ubo.projection, mul(ubo.modelview, float4(input.Pos.xyz, 1.0)));

    // Base size of the point sprites
    float spriteSize = 8.0 * input.Size;

    // Scale particle size depending on camera projection
    float4 eyePos = mul(ubo.modelview, float4(input.Pos.xyz, 1.0));
    float4 projectedCorner = mul(ubo.projection, float4(0.5 * spriteSize, 0.5 * spriteSize, eyePos.z, eyePos.w));
    output.PointSize = output.PSize = ubo.viewportDim.x * projectedCorner.x / projectedCorner.w;
    output.CenterPos = ((output.Pos.xy / output.Pos.w) + 1.0) * 0.5 * ubo.viewportDim;
    return output;
}

[shader("fragment")]
float4 fragmentMain(VSOutput input)
{
	float4 color;
	float alpha = (input.Alpha <= 1.0) ? input.Alpha : 2.0 - input.Alpha;

	// Rotate texture coordinates
	// Rotate UV
	float rotCenter = 0.5;
	float rotCos = cos(input.Rotation);
	float rotSin = sin(input.Rotation);

	float2 PointCoord = (input.Pos.xy - input.CenterPos.xy) / input.PointSize + 0.5;

	float2 rotUV = float2(
		rotCos * (PointCoord.x - rotCenter) + rotSin * (PointCoord.y - rotCenter) + rotCenter,
		rotCos * (PointCoord.y - rotCenter) - rotSin * (PointCoord.x - rotCenter) + rotCenter);

	float4 outFragColor;
	if (input.Type == 0)
	{
        // Flame
        color = samplerFire.Sample(rotUV);
		outFragColor.a = 0.0;
	}
	else
	{
        // Smoke
        color = samplerSmoke.Sample(rotUV);
		outFragColor.a = color.a * alpha;
	}

	outFragColor.rgb = color.rgb * input.Color.rgb * alpha;
	return outFragColor;
}