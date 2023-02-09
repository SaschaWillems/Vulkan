// Copyright 2021 Sascha Willems
// Non-uniform access is enabled at compile time via SPV_EXT_descriptor_indexing (see compile.py)

Texture2D textures[] : register(t1);
SamplerState samplerColorMap : register(s1);

struct VSOutput
{
	float4 Pos : SV_POSITION;
[[vk::location(0)]] int TextureIndex : TEXTUREINDEX0;
[[vk::location(1)]] float2 UV : TEXCOORD0;
};

float4 main(VSOutput input) : SV_TARGET
{
	return textures[NonUniformResourceIndex(input.TextureIndex)].Sample(samplerColorMap, input.UV); 
}