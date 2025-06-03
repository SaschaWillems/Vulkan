/* Copyright (c) 2025, Sascha Willems
 *
 * SPDX-License-Identifier: MIT
 *
 */

struct VSOutput
{
    float4 Pos : SV_POSITION;
    float2 UV;
};

[[vk::input_attachment_index(0)]] SubpassInput inputPosition;
[[vk::input_attachment_index(1)]] SubpassInput inputNormal;
[[vk::input_attachment_index(2)]] SubpassInput inputAlbedo;

struct Light {
	float4 position;
	float3 color;
	float radius;
};
RWStructuredBuffer<Light> lights;

[shader("vertex")]
VSOutput vertexMain(uint VertexIndex: SV_VertexID)
{
    VSOutput output;
    output.UV = float2((VertexIndex << 1) & 2, VertexIndex & 2);
    output.Pos = float4(output.UV * 2.0f - 1.0f, 0.0f, 1.0f);
    return output;
}

[shader("fragment")]
float4 fragmentMain()
{
	// Read G-Buffer values from previous sub pass
	float3 fragPos = inputPosition.SubpassLoad().rgb;
	float3 normal = inputNormal.SubpassLoad().rgb;
	float4 albedo = inputAlbedo.SubpassLoad();

	#define ambient 0.05

	// Ambient part
	float3 fragcolor  = albedo.rgb * ambient;

	uint lightsLength;
	uint lightsStride;
	lights.GetDimensions(lightsLength, lightsStride);

	for(int i = 0; i < lightsLength; ++i)
	{
		float3 L = lights[i].position.xyz - fragPos;
		float dist = length(L);

		L = normalize(L);

		float atten = lights[i].radius / (pow(dist, 3.0) + 1.0);
		float3 N = normalize(normal);
		float NdotL = max(0.0, dot(N, L));
		float3 diff = lights[i].color * albedo.rgb * NdotL * atten;

		fragcolor += diff;
	}

	return float4(fragcolor, 1.0);
}