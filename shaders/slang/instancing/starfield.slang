/* Copyright (c) 2025, Sascha Willems
 *
 * SPDX-License-Identifier: MIT
 *
 */

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

struct VSOutput
{
    float4 Pos : SV_POSITION;
    float3 UVW;
};

[shader("vertex")]
VSOutput vertexMain(uint VertexIndex: SV_VertexID)
{
    VSOutput output;
    output.UVW = float3((VertexIndex << 1) & 2, VertexIndex & 2, VertexIndex & 2);
    output.Pos = float4(output.UVW.xy * 2.0f - 1.0f, 0.0f, 1.0f);
    return output;
}

[shader("fragment")]
float4 fragmentMain(VSOutput input)
{
	return float4(starField(input.UVW), 1.0);
}