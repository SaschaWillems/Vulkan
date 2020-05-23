// Copyright 2020 Google LLC

struct VSInput
{
[[vk::location(0)]] float3 Pos : POSITION0;
[[vk::location(1)]] float3 Normal : NORMAL0;
[[vk::location(2)]] float3 Color : COLOR0;
};

struct PushConsts
{
	float4x4 mvp;
	float3 color;
};
[[vk::push_constant]]PushConsts pushConsts;

struct VSOutput
{
	float4 Pos : SV_POSITION;
[[vk::location(0)]] float3 Normal : NORMAL0;
[[vk::location(1)]] float3 Color : COLOR0;
[[vk::location(3)]] float3 ViewVec : TEXCOORD1;
[[vk::location(4)]] float3 LightVec : TEXCOORD2;
};

VSOutput main(VSInput input)
{
	VSOutput output = (VSOutput)0;
	output.Normal = input.Normal;

	if ( (input.Color.r == 1.0) && (input.Color.g == 0.0) && (input.Color.b == 0.0))
	{
		output.Color = pushConsts.color;
	}
	else
	{
		output.Color = input.Color;
	}

	output.Pos = mul(pushConsts.mvp, float4(input.Pos.xyz, 1.0));

    float4 pos = mul(pushConsts.mvp, float4(input.Pos, 1.0));
    output.Normal = mul((float3x3)pushConsts.mvp, input.Normal);
//	float3 lPos = ubo.lightPos.xyz;
float3 lPos = float3(0.0, 0.0, 0.0);
    output.LightVec = lPos - pos.xyz;
    output.ViewVec = -pos.xyz;
	return output;
}