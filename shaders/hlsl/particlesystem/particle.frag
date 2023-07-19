// Copyright 2020 Google LLC

Texture2D textureSmoke : register(t1);
SamplerState samplerSmoke : register(s1);
Texture2D textureFire : register(t2);
SamplerState samplerFire : register(s2);

struct VSOutput
{
	float4 Pos : SV_POSITION;
[[vk::location(0)]] float4 Color : COLOR0;
[[vk::location(1)]] float Alpha : TEXCOODR0;
[[vk::location(2)]] int Type : TEXCOODR1;
[[vk::location(3)]] float Rotation : TEXCOODR2;
[[vk::location(4)]] float2 CenterPos : POSITION1;
[[vk::location(5)]] float PointSize : TEXCOORD3;
};

float4 main (VSOutput input) : SV_TARGET
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
		color = textureFire.Sample(samplerFire, rotUV);
		outFragColor.a = 0.0;
	}
	else
	{
		// Smoke
		color = textureSmoke.Sample(samplerSmoke, rotUV);
		outFragColor.a = color.a * alpha;
	}

	outFragColor.rgb = color.rgb * input.Color.rgb * alpha;
	return outFragColor;
}