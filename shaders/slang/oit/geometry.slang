/* Copyright (c) 2025, Sascha Willems
 *
 * SPDX-License-Identifier: MIT
 *
 */

struct VSInput
{
    float4 Pos : POSITION0;
};

struct VSOutput
{
	float4 Pos : SV_POSITION;
};

struct RenderPassUBO
{
    float4x4 projection;
    float4x4 view;
};
ConstantBuffer<RenderPassUBO> renderPassUBO;

struct GeometrySBO
{
    uint count;
    uint maxNodeCount;
};
// Binding 0 : Position storage buffer
RWStructuredBuffer<GeometrySBO> geometrySBO;

struct Node
{
    float4 color;
    float depth;
    uint next;
};
RWTexture2D<uint> headIndexImage;

RWStructuredBuffer<Node> nodes;

struct PushConsts {
    float4x4 model;
    float4 color;
};

[shader("vertex")]
VSOutput vertexMain(VSInput input, uniform PushConsts pushConsts)
{
    VSOutput output;
    output.Pos = mul(renderPassUBO.projection, mul(renderPassUBO.view, mul(pushConsts.model, input.Pos)));
    return output;
}

[shader("fragment")]
[earlydepthstencil]
void fragmentMain(VSOutput input, uniform PushConsts pushConsts)
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
        nodes[nodeIdx].color = pushConsts.color;
        nodes[nodeIdx].depth = input.Pos.z;
        nodes[nodeIdx].next = prevHeadIdx;
    }
}