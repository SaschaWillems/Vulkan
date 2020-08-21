// Copyright 2020 Sascha Willems

#define MAX_FRAGMENT_COUNT 75

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

struct ObjectUBO
{
    float4x4 model;
    float4 color;
};

cbuffer ubo : register(b1) { ObjectUBO objectUBO; }

struct GeometrySBO
{
    uint count;
    uint maxNodeCount;
};
// Binding 0 : Position storage buffer
RWStructuredBuffer<GeometrySBO> geometrySBO : register(u2);

RWTexture2D<uint> headIndexImage : register(u3);

RWStructuredBuffer<Node> nodes : register(u4);

[earlydepthstencil]
void main(VSOutput input)
{
    // Increase the node count
    uint nodeIdx;
    InterlockedAdd(geometrySBO[0].count, 1, nodeIdx);

    // Check LinkedListSBO is full
    if (nodeIdx < geometrySBO[0].maxNodeCount)
    {
        // Exchange new head index and previous head index
        uint prevHeadIdx;
        InterlockedExchange(headIndexImage[uint2(input.Pos.xy)], nodeIdx, prevHeadIdx);

        // Store node data
        nodes[nodeIdx].color = objectUBO.color;
        nodes[nodeIdx].depth = input.Pos.z;
        nodes[nodeIdx].next = prevHeadIdx;
    }
}