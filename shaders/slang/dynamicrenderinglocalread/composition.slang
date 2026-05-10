/* Copyright (c) 2026, Sascha Willems
 *
 * SPDX-License-Identifier: MIT
 *
 */

[[vk::input_attachment_index(0)]] SubpassInput positionDepthAttachment;
[[vk::input_attachment_index(1)]] SubpassInput normalAttachment;
[[vk::input_attachment_index(2)]] SubpassInput albedoAttachment;

struct VSOutput
{
    float4 Pos : SV_POSITION;
    float2 UV;
};

struct Light
{
	float4 position;
	float3 color;
	float radius;
};
// Binding 0 : Position storage buffer
StructuredBuffer<Light> lights;

[shader("vertex")]
VSOutput vertexMain(uint VertexIndex: SV_VertexID)
{
    VSOutput output;
    output.UV = float2((VertexIndex << 1) & 2, VertexIndex & 2);
    output.Pos = float4(output.UV * 2.0f - 1.0f, 0.0f, 1.0f);
    return output;
}

[shader("fragment")]
float4 fragmentMain(VSOutput input) 
{
	// Read G-Buffer values from previous sub pass
    float3 fragPos = positionDepthAttachment.SubpassLoad().rgb;
    float3 normal = normalAttachment.SubpassLoad().rgb;
    float4 albedo = albedoAttachment.SubpassLoad();

	#define ambient 0.15

    // Ambient part
    float3 fragcolor = albedo.rgb * ambient;

    uint lightsLength;
    uint lightsStride;
    lights.GetDimensions(lightsLength, lightsStride);
	
	for(int i = 0; i < lightsLength; ++i)
    {
        float3 L = lights[i].position.xyz - fragPos;
		float dist = length(L);

		float attenuation = lights[i].radius / (pow(dist, 8.0) + 1.0);

        float NdotL = max(0.0, dot(normalize(normal), normalize(L)));
        float3 diffuse = lights[i].color * albedo.rgb * NdotL * attenuation;

		fragcolor += diffuse;
	}

    return float4(fragcolor, 1.0);
}