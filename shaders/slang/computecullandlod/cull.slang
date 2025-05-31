/* Copyright (c) 2025, Sascha Willems
 *
 * SPDX-License-Identifier: MIT
 *
 */

#define MAX_LOD_LEVEL_COUNT 6
[[SpecializationConstant]] const int MAX_LOD_LEVEL = 5;

struct InstanceData
{
	float3 pos;
	float scale;
};
StructuredBuffer<InstanceData> instances;

// Same layout as VkDrawIndexedIndirectCommand
struct IndexedIndirectCommand
{
	uint indexCount;
	uint instanceCount;
	uint firstIndex;
	uint vertexOffset;
	uint firstInstance;
};
RWStructuredBuffer<IndexedIndirectCommand> indirectDraws;

// Binding 2: Uniform block object with matrices
struct UBO
{
	float4x4 projection;
	float4x4 modelview;
	float4 cameraPos;
	float4 frustumPlanes[6];
};
ConstantBuffer<UBO> ubo;

// Binding 3: Indirect draw stats
struct UBOOut
{
	uint drawCount;
	uint lodCount[MAX_LOD_LEVEL_COUNT];
};
RWStructuredBuffer<UBOOut> uboOut;

// Binding 4: level-of-detail information
struct LOD
{
	uint firstIndex;
	uint indexCount;
	float distance;
	float _pad0;
};
StructuredBuffer<LOD> lods;

bool frustumCheck(float4 pos, float radius)
{
	// Check sphere against frustum planes
	for (int i = 0; i < 6; i++)
	{
		if (dot(pos, ubo.frustumPlanes[i]) + radius < 0.0)
		{
			return false;
		}
	}
	return true;
}

[shader("compute")]
[numthreads(16, 1, 1)]
void computeMain(uint3 GlobalInvocationID : SV_DispatchThreadID)
{
	uint idx = GlobalInvocationID.x;
	uint temp;

	// Clear stats on first invocation
	if (idx == 0)
	{
		InterlockedExchange(uboOut[0].drawCount, 0, temp);
		for (uint i = 0; i < MAX_LOD_LEVEL + 1; i++)
		{
			InterlockedExchange(uboOut[0].lodCount[i], 0, temp);
		}
	}

	float4 pos = float4(instances[idx].pos.xyz, 1.0);

	// Check if object is within current viewing frustum
	if (frustumCheck(pos, 1.0))
	{
		indirectDraws[idx].instanceCount = 1;

		// Increase number of indirect draw counts
		InterlockedAdd(uboOut[0].drawCount, 1, temp);

		// Select appropriate LOD level based on distance to camera
		uint lodLevel = MAX_LOD_LEVEL;
		for (uint i = 0; i < MAX_LOD_LEVEL; i++)
		{
			if (distance(instances[idx].pos.xyz, ubo.cameraPos.xyz) < lods[i].distance)
			{
				lodLevel = i;
				break;
			}
		}
		indirectDraws[idx].firstIndex = lods[lodLevel].firstIndex;
		indirectDraws[idx].indexCount = lods[lodLevel].indexCount;
		// Update stats
		InterlockedAdd(uboOut[0].lodCount[lodLevel], 1, temp);
	}
	else
	{
		indirectDraws[idx].instanceCount = 0;
	}
}