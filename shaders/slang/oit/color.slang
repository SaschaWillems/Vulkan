/* Copyright (c) 2025, Sascha Willems
 *
 * SPDX-License-Identifier: MIT
 *
 */

#define MAX_FRAGMENT_COUNT 128

struct VSOutput
{
	float4 Pos : SV_POSITION;
};

struct Node
{
    float4 color;
    float depth;
    uint next;
};
RWTexture2D<uint> headIndexImage;

struct Particle
{
	float2 pos;
	float2 vel;
	float4 gradientPos;
};
RWStructuredBuffer<Node> nodes;

[shader("vertex")]
VSOutput vertexMain(uint VertexIndex: SV_VertexID)
{
    VSOutput output;
    float2 UV = float2((VertexIndex << 1) & 2, VertexIndex & 2);
    output.Pos = float4(UV * 2.0f - 1.0f, 0.0f, 1.0f);
    return output;
}

[shader("fragment")]
float4 fragmentMain(VSOutput input)
{
    Node fragments[MAX_FRAGMENT_COUNT];
    int count = 0;

    uint nodeIdx = headIndexImage[uint2(input.Pos.xy)].r;

    while (nodeIdx != 0xffffffff && count < MAX_FRAGMENT_COUNT)
    {
        fragments[count] = nodes[nodeIdx];
        nodeIdx = fragments[count].next;
        ++count;
    }
    
    // Do the insertion sort
    for (uint i = 1; i < count; ++i)
    {
        Node insert = fragments[i];
        uint j = i;
        while (j > 0 && insert.depth > fragments[j - 1].depth)
        {
            fragments[j] = fragments[j-1];
            --j;
        }
        fragments[j] = insert;
    }

    // Do blending
    float4 color = float4(0.025, 0.025, 0.025, 1.0f);
    for (uint f = 0; f < count; ++f)
    {
        color = lerp(color, fragments[f].color, fragments[f].color.a);
    }

    return color;
}