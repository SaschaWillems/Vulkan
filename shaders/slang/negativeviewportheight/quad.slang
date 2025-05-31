// Copyright 2020 Google LLC

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

Sampler2D samplerColor;

[shader("vertex")]
VSOutput vertexMain(VSInput input)
{
    VSOutput output;
    output.UV = input.UV;
    output.Pos = float4(input.Pos, 1.0f);
    return output;
}

[shader("fragment")]
float4 fragmentMain(VSOutput input)
{
    return samplerColor.Sample(input.UV);
}