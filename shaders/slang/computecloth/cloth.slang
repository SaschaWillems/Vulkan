/* Copyright (c) 2025, Sascha Willems
 *
 * SPDX-License-Identifier: MIT
 *
 */

struct VSInput
{
    float3 Pos;
    float2 UV;
    float3 Normal;
};

struct VSOutput
{
    float4 Pos : SV_POSITION;
    float2 UV;
    float3 Normal;
    float3 ViewVec;
    float3 LightVec;
};

struct UBO
{
    float4x4 projection;
    float4x4 modelview;
    float4 lightPos;
};
[[vk::binding(0,0)]] ConstantBuffer<UBO> ubo;
[[vk::binding(1,0)]] Sampler2D samplerColor;

struct Particle {
	float4 pos;
	float4 vel;
	float4 uv;
	float4 normal;
};

[[vk::binding(0,0)]] StructuredBuffer<Particle> particleIn;
[[vk::binding(1,0)]] RWStructuredBuffer<Particle> particleOut;

struct UBOCompute
{
	float deltaT;
	float particleMass;
	float springStiffness;
	float damping;
	float restDistH;
	float restDistV;
	float restDistD;
	float sphereRadius;
	float4 spherePos;
	float4 gravity;
	int2 particleCount;
};
[[vk::binding(2, 0)]] ConstantBuffer<UBOCompute> params;

float3 springForce(float3 p0, float3 p1, float restDist)
{
	float3 dist = p0 - p1;
	return normalize(dist) * params.springStiffness * (length(dist) - restDist);
}

[shader("vertex")]
VSOutput vertexMain(VSInput input)
{
    VSOutput output;
    output.UV = input.UV;
    output.Normal = input.Normal.xyz;
    float4 eyePos = mul(ubo.modelview, float4(input.Pos.x, input.Pos.y, input.Pos.z, 1.0));
    output.Pos = mul(ubo.projection, eyePos);
    float4 pos = float4(input.Pos, 1.0);
    float3 lPos = ubo.lightPos.xyz;
    output.LightVec = lPos - pos.xyz;
    output.ViewVec = -pos.xyz;
    return output;
}

[shader("fragment")]
float4 fragmentMain(VSOutput input)
{
    float3 color = samplerColor.Sample(input.UV).rgb;
    float3 N = normalize(input.Normal);
    float3 L = normalize(input.LightVec);
    float3 V = normalize(input.ViewVec);
    float3 R = reflect(-L, N);
    float3 diffuse = max(dot(N, L), 0.15) * float3(1, 1, 1);
    float3 specular = pow(max(dot(R, V), 0.0), 8.0) * float3(0.2, 0.2, 0.2);
    return float4(diffuse * color.rgb + specular, 1.0);
}

[shader("compute")]
[numthreads(10, 10, 1)]
void computeMain(uint3 id: SV_DispatchThreadID, uniform uint calculateNormals)
{
	uint index = id.y * params.particleCount.x + id.x;
	if (index > params.particleCount.x * params.particleCount.y)
		return;

	// Initial force from gravity
	float3 force = params.gravity.xyz * params.particleMass;

	float3 pos = particleIn[index].pos.xyz;
	float3 vel = particleIn[index].vel.xyz;

	// Spring forces from neighboring particles
	// left
	if (id.x > 0) {
		force += springForce(particleIn[index-1].pos.xyz, pos, params.restDistH);
	}
	// right
	if (id.x < params.particleCount.x - 1) {
		force += springForce(particleIn[index + 1].pos.xyz, pos, params.restDistH);
	}
	// upper
	if (id.y < params.particleCount.y - 1) {
		force += springForce(particleIn[index + params.particleCount.x].pos.xyz, pos, params.restDistV);
	}
	// lower
	if (id.y > 0) {
		force += springForce(particleIn[index - params.particleCount.x].pos.xyz, pos, params.restDistV);
	}
	// upper-left
	if ((id.x > 0) && (id.y < params.particleCount.y - 1)) {
		force += springForce(particleIn[index + params.particleCount.x - 1].pos.xyz, pos, params.restDistD);
	}
	// lower-left
	if ((id.x > 0) && (id.y > 0)) {
		force += springForce(particleIn[index - params.particleCount.x - 1].pos.xyz, pos, params.restDistD);
	}
	// upper-right
	if ((id.x < params.particleCount.x - 1) && (id.y < params.particleCount.y - 1)) {
		force += springForce(particleIn[index + params.particleCount.x + 1].pos.xyz, pos, params.restDistD);
	}
	// lower-right
	if ((id.x < params.particleCount.x - 1) && (id.y > 0)) {
		force += springForce(particleIn[index - params.particleCount.x + 1].pos.xyz, pos, params.restDistD);
	}

	force += (-params.damping * vel);

	// Integrate
	float3 f = force * (1.0 / params.particleMass);
	particleOut[index].pos = float4(pos + vel * params.deltaT + 0.5 * f * params.deltaT * params.deltaT, 1.0);
	particleOut[index].vel = float4(vel + f * params.deltaT, 0.0);

	// Sphere collision
	float3 sphereDist = particleOut[index].pos.xyz - params.spherePos.xyz;
	if (length(sphereDist) < params.sphereRadius + 0.01) {
		// If the particle is inside the sphere, push it to the outer radius
		particleOut[index].pos.xyz = params.spherePos.xyz + normalize(sphereDist) * (params.sphereRadius + 0.01);
		// Cancel out velocity
		particleOut[index].vel = float4(0, 0, 0, 0);
	}

	// Normals
	if (calculateNormals == 1) {
		float3 normal = float3(0, 0, 0);
		float3 a, b, c;
		if (id.y > 0) {
			if (id.x > 0) {
				a = particleIn[index - 1].pos.xyz - pos;
				b = particleIn[index - params.particleCount.x - 1].pos.xyz - pos;
				c = particleIn[index - params.particleCount.x].pos.xyz - pos;
				normal += cross(a,b) + cross(b,c);
			}
			if (id.x < params.particleCount.x - 1) {
				a = particleIn[index - params.particleCount.x].pos.xyz - pos;
				b = particleIn[index - params.particleCount.x + 1].pos.xyz - pos;
				c = particleIn[index + 1].pos.xyz - pos;
				normal += cross(a,b) + cross(b,c);
			}
		}
		if (id.y < params.particleCount.y - 1) {
			if (id.x > 0) {
				a = particleIn[index + params.particleCount.x].pos.xyz - pos;
				b = particleIn[index + params.particleCount.x - 1].pos.xyz - pos;
				c = particleIn[index - 1].pos.xyz - pos;
				normal += cross(a,b) + cross(b,c);
			}
			if (id.x < params.particleCount.x - 1) {
				a = particleIn[index + 1].pos.xyz - pos;
				b = particleIn[index + params.particleCount.x + 1].pos.xyz - pos;
				c = particleIn[index + params.particleCount.x].pos.xyz - pos;
				normal += cross(a,b) + cross(b,c);
			}
		}
		particleOut[index].normal = float4(normalize(normal), 0.0f);
	}
}