#version 450

#define MAX_FRAGMENT_COUNT 75

layout (early_fragment_tests) in;

struct Node
{
    vec4 color;
    float depth;
    uint next;
};

layout (set = 0, binding = 1) uniform ObjectUBO
{
    mat4 model;
    vec4 color;
} objectUBO;

layout (set = 0, binding = 2) buffer GeometrySBO
{
    uint count;
    uint maxNodeCount;
};

layout (set = 0, binding = 3, r32ui) uniform uimage2D headIndexImage;

layout (set = 0, binding = 4) buffer LinkedListSBO
{
    Node nodes[];
};

void main()
{
    // Increase the node count
    uint nodeIdx = atomicAdd(count, 1);

    // Check LinkedListSBO is full
    if (nodeIdx < maxNodeCount)
    {
        // Exchange new head index and previous head index
        uint prevHeadIdx = imageAtomicExchange(headIndexImage, ivec2(gl_FragCoord.xy), nodeIdx);

        // Store node data
        nodes[nodeIdx].color = objectUBO.color;
        nodes[nodeIdx].depth = gl_FragCoord.z;
        nodes[nodeIdx].next = prevHeadIdx;
    }
}