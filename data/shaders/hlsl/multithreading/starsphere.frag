// Copyright 2020 Google LLC

#define HASHSCALE3 float3(443.897, 441.423, 437.195)
#define STARFREQUENCY 0.01

// Hash function by Dave Hoskins (https://www.shadertoy.com/view/4djSRW)
float hash33(float3 p3)
{
	p3 = frac(p3 * HASHSCALE3);
    p3 += dot(p3, p3.yxz+float3(19.19, 19.19, 19.19));
    return frac((p3.x + p3.y)*p3.z + (p3.x+p3.z)*p3.y + (p3.y+p3.z)*p3.x);
}

float3 starField(float3 pos)
{
	float3 color = float3(0.0, 0.0, 0.0);
    float threshhold = (1.0 - STARFREQUENCY);
    float rnd = hash33(pos);
    if (rnd >= threshhold)
    {
        float starCol = pow((rnd - threshhold) / (1.0 - threshhold), 16.0);
		color += starCol.xxx;
    }
	return color;
}

float4 main([[vk::location(0)]] float3 inUVW : TEXCOORD0) : SV_TARGET
{
	// Fake atmosphere at the bottom
	float3 atmosphere = clamp(float3(0.1, 0.15, 0.4) * (inUVW.y + 0.25), 0.0, 1.0);

	float3 color = starField(inUVW) + atmosphere;

	return float4(color, 1.0);
}