// Copyright 2020 Sascha Willems

struct VSInput
{
[[vk::location(0)]] float4 Pos : POSITION0;
};

struct RenderPassUBO
{
    float4x4 projection;
    float4x4 view;
};

cbuffer renderPassUBO : register(b0) { RenderPassUBO renderPassUBO; }

struct PushConsts {
	float4x4 model;
	float4 color;
};
[[vk::push_constant]] PushConsts pushConsts;

struct VSOutput
{
	float4 Pos : SV_POSITION;
};

VSOutput main(VSInput input)
{
	VSOutput output = (VSOutput)0;
	output.Pos = mul(renderPassUBO.projection, mul(renderPassUBO.view, mul(pushConsts.model, input.Pos)));
    return output;
}
