/* Copyright (c) 2025, Sascha Willems
 *
 * SPDX-License-Identifier: MIT
 *
 */

[[vk::binding(0, 0)]] Sampler2D samplerColorMap;
[[vk::binding(1, 0)]] Sampler2D samplerGradientRamp;

struct VSInput
{
    float2 Pos : POSITION0;
    float4 GradientPos : POSITION1;
};

struct VSOutput
{
    float4 Pos : SV_POSITION;
    float PointSize : SV_PointSize;    
    float4 Color : COLOR0;
    float GradientPos : POSITION0;
};

struct Particle
{
	float2 pos;
	float2 vel;
	float4 gradientPos;
};
// Binding 0 : Position storage buffer
[[vk::binding(0, 0)]] RWStructuredBuffer<Particle> particles;

struct UBO
{
	float deltaT;
	float destX;
	float destY;
	int particleCount;
};
[[vk::binding(1, 0)]] ConstantBuffer<UBO> ubo;

struct PushConsts
{
    float2 screendim;
};

float2 attraction(float2 pos, float2 attractPos)
{
    float2 delta = attractPos - pos;
	const float damp = 0.5;
    float dDampedDot = dot(delta, delta) + damp;
    float invDist = 1.0f / sqrt(dDampedDot);
    float invDistCubed = invDist*invDist*invDist;
    return delta * invDistCubed * 0.0035;
}

float2 repulsion(float2 pos, float2 attractPos)
{
	float2 delta = attractPos - pos;
	float targetDistance = sqrt(dot(delta, delta));
	return delta * (1.0 / (targetDistance * targetDistance * targetDistance)) * -0.000035;
}

[shader("vertex")]
VSOutput vertexMain(VSInput input)
{
    VSOutput output;
    output.PointSize = 8.0;
    output.Color = float4(0.035, 0.035, 0.035, 0.035);
    output.GradientPos = input.GradientPos.x;
    output.Pos = float4(input.Pos.xy, 1.0, 1.0);
    return output;
}

[shader("fragment")]
float4 fragmentMain(VSOutput input, float2 pointCoord: SV_PointCoord)
{
    float3 color = samplerGradientRamp.Sample(float2(input.GradientPos, 0.0)).rgb;
    return float4(samplerColorMap.Sample(pointCoord).rgb * color, 1.0);
}

[shader("compute")]
[numthreads(256, 1, 1)]
void computeMain(uint3 GlobalInvocationID : SV_DispatchThreadID)
{
    // Current SSBO index
    uint index = GlobalInvocationID.x;
    // Don't try to write beyond particle count
    if (index >= ubo.particleCount) {
        return;
    }

    // Read position and velocity
    float2 vVel = particles[index].vel.xy;
    float2 vPos = particles[index].pos.xy;

    float2 destPos = float2(ubo.destX, ubo.destY);

    float2 delta = destPos - vPos;
    float targetDistance = sqrt(dot(delta, delta));
    vVel += repulsion(vPos, destPos.xy) * 0.05;

    // Move by velocity
    vPos += vVel * ubo.deltaT;

    // collide with boundary
    if ((vPos.x < -1.0) || (vPos.x > 1.0) || (vPos.y < -1.0) || (vPos.y > 1.0)) {
        vVel = (-vVel * 0.1) + attraction(vPos, destPos) * 12;
    } else {
        particles[index].pos.xy = vPos;
    }

    // Write back
    particles[index].vel.xy = vVel;
    particles[index].gradientPos.x += 0.02 * ubo.deltaT;
    if (particles[index].gradientPos.x > 1.0) {
        particles[index].gradientPos.x -= 1.0;
    }
}

