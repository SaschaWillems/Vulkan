/* Copyright (c) 2025, Sascha Willems
 *
 * SPDX-License-Identifier: MIT
 *
 */

struct Particle
{
	float4 pos;
	float4 vel;
};
// Binding 0 : Position storage buffer
RWStructuredBuffer<Particle> particles;

struct UBO
{
	float deltaT;
	int particleCount;
	float gravity;
	float power;
	float soften;
};
ConstantBuffer<UBO> ubo;

#define MAX_SHARED_DATA_SIZE 1024
[[SpecializationConstant]] const int SHARED_DATA_SIZE = 512;
[[SpecializationConstant]] const float GRAVITY = 0.002;
[[SpecializationConstant]] const float POWER = 0.75;
[[SpecializationConstant]] const float SOFTEN = 0.0075;

// Share data between computer shader invocations to speed up caluclations
groupshared float4 sharedData[MAX_SHARED_DATA_SIZE];

[shader("compute")]
[numthreads(256, 1, 1)]
void computeMain(uint3 GlobalInvocationID : SV_DispatchThreadID, uint3 LocalInvocationID : SV_GroupThreadID)
{
	// Current SSBO index
	uint index = GlobalInvocationID.x;
	if (index >= ubo.particleCount)
		return;

	float4 position = particles[index].pos;
	float4 velocity = particles[index].vel;
	float4 acceleration = float4(0, 0, 0, 0);

	for (int i = 0; i < ubo.particleCount; i += SHARED_DATA_SIZE)
	{
		if (i + LocalInvocationID.x < ubo.particleCount)
		{
			sharedData[LocalInvocationID.x] = particles[i + LocalInvocationID.x].pos;
		}
		else
		{
			sharedData[LocalInvocationID.x] = float4(0, 0, 0, 0);
		}

		GroupMemoryBarrierWithGroupSync();

		for (int j = 0; j < 256; j++)
		{
			float4 other = sharedData[j];
			float3 len = other.xyz - position.xyz;
			acceleration.xyz += ubo.gravity * len * other.w / pow(dot(len, len) + ubo.soften, ubo.power);
		}

		GroupMemoryBarrierWithGroupSync();
	}

	particles[index].vel.xyz += ubo.deltaT * acceleration.xyz;

	// Gradient texture position
	particles[index].vel.w += 0.1 * ubo.deltaT;
	if (particles[index].vel.w > 1.0) {
		particles[index].vel.w -= 1.0;
	}
}