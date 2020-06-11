// Copyright 2020 Google LLC

struct VSOutput
{
[[vk::location(0)]] float4 Pos : POSITION0;
[[vk::location(1)]] float3 LightPos : POSITION1;
};

float main(VSOutput input) : SV_TARGET
{
	// Store distance to light as 32 bit float value
    float3 lightVec = input.Pos.xyz - input.LightPos;
    return length(lightVec);
}