// Copyright 2020 Sascha Willems

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

RWTexture2D<uint> headIndexImage : register(u0);

struct Particle
{
	float2 pos;
	float2 vel;
	float4 gradientPos;
};

// Binding 0 : Position storage buffer
RWStructuredBuffer<Node> nodes : register(u1);

float4 main(VSOutput input) : SV_TARGET
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