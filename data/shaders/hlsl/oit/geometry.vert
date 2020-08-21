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

struct ObjectUBO
{
    float4x4 model;
    float4 color;
};

cbuffer objectUBO : register(b1) { ObjectUBO objectUBO; }

struct VSOutput
{
	float4 Pos : SV_POSITION;
};

VSOutput main(VSInput input)
{
	VSOutput output = (VSOutput)0;
	output.Pos = mul(renderPassUBO.projection, mul(renderPassUBO.view, mul(objectUBO.model, input.Pos)));
    return output;
}
