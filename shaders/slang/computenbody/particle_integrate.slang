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
};
ConstantBuffer<UBO> ubo;

[shader("compute")]
[numthreads(256, 1, 1)]
void computeMain(uint3 GlobalInvocationID : SV_DispatchThreadID)
{
	int index = int(GlobalInvocationID.x);
	float4 position = particles[index].pos;
	float4 velocity = particles[index].vel;
	position += ubo.deltaT * velocity;
	particles[index].pos = position;
}