// Copyright 2020 Google LLC

Texture2D textureColor : register(t1);
SamplerState samplerColor : register(s1);

struct UBO
{
	float4 outlineColor;
	float outlineWidth;
	float outline;
};

cbuffer ubo : register(b2) { UBO ubo; }

float4 main([[vk::location(0)]] float2 inUV : TEXCOORD0) : SV_TARGET
{
    float dist = textureColor.Sample(samplerColor, inUV).a;
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