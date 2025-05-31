/* Copyright (c) 2025, Sascha Willems
 *
 * SPDX-License-Identifier: MIT
 *
 */

[[vk::binding(1, 0)]] Texture2DMS samplerPosition;
[[vk::binding(2, 0)]] Texture2DMS samplerNormal;
[[vk::binding(3, 0)]] Texture2DMS samplerAlbedo;

struct Light {
    float4 position;
    float3 color;
    float radius;
};

struct UBO
{
    Light lights[6];
    float4 viewPos;
    int displayDebugTarget;
};
[[vk::binding(4, 0)]] ConstantBuffer<UBO> ubo;

struct VSOutput
{
    float4 Pos : SV_POSITION;
    float2 UV;
};

[[SpecializationConstant]] const int NUM_SAMPLES = 8;
#define NUM_LIGHTS 6

// Manual resolve for MSAA samples
float4 resolve(Texture2DMS<float4> tex, int2 uv)
{
    float4 result = float4(0.0, 0.0, 0.0, 0.0);
    for (int i = 0; i < NUM_SAMPLES; i++)
    {
        uint status = 0;
        float4 val = tex.Load(uv, i, int2(0, 0), status);
        result += val;
    }
    // Average resolved samples
    return result / float(NUM_SAMPLES);
}

float3 calculateLighting(float3 pos, float3 normal, float4 albedo)
{
    float3 result = float3(0.0, 0.0, 0.0);

    for (int i = 0; i < NUM_LIGHTS; ++i)
    {
        // Vector to light
        float3 L = ubo.lights[i].position.xyz - pos;
        // Distance from light to fragment position
        float dist = length(L);

        // Viewer to fragment
        float3 V = ubo.viewPos.xyz - pos;
        V = normalize(V);

        // Light to fragment
        L = normalize(L);

        // Attenuation
        float atten = ubo.lights[i].radius / (pow(dist, 2.0) + 1.0);

        // Diffuse part
        float3 N = normalize(normal);
        float NdotL = max(0.0, dot(N, L));
        float3 diff = ubo.lights[i].color * albedo.rgb * NdotL * atten;

        // Specular part
        float3 R = reflect(-L, N);
        float NdotR = max(0.0, dot(R, V));
        float3 spec = ubo.lights[i].color * albedo.a * pow(NdotR, 8.0) * atten;

        result += diff + spec;
    }
    return result;
}

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
    int2 attDim; int sampleCount;
    samplerPosition.GetDimensions(attDim.x, attDim.y, sampleCount);
    int2 UV = int2(input.UV * attDim);

    float3 fragColor;
    uint status = 0;

    // Debug display
    if (ubo.displayDebugTarget > 0) {
        switch (ubo.displayDebugTarget) {
        case 1:
            fragColor.rgb = samplerPosition.Load(UV, 0, int2(0, 0), status).rgb;
            break;
        case 2:
            fragColor.rgb = samplerNormal.Load(UV, 0, int2(0, 0), status).rgb;
            break;
        case 3:
            fragColor.rgb = samplerAlbedo.Load(UV, 0, int2(0, 0), status).rgb;
            break;
        case 4:
            fragColor.rgb = samplerAlbedo.Load(UV, 0, int2(0, 0), status).aaa;
            break;
        }
        return float4(fragColor, 1.0);
    }

#define ambient 0.15

    // Ambient part
    float4 alb = resolve(samplerAlbedo, UV);
    fragColor = float3(0.0, 0.0, 0.0);

    // Calualte lighting for every MSAA sample
    for (int i = 0; i < NUM_SAMPLES; i++)
    {
        float3 pos = samplerPosition.Load(UV, i, int2(0, 0), status).rgb;
        float3 normal = samplerNormal.Load(UV, i, int2(0, 0), status).rgb;
        float4 albedo = samplerAlbedo.Load(UV, i, int2(0, 0), status);
        fragColor += calculateLighting(pos, normal, albedo);
    }

    fragColor = (alb.rgb * ambient) + fragColor / float(NUM_SAMPLES);

    return float4(fragColor, 1.0);
}