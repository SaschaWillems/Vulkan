/* Copyright (c) 2025, Sascha Willems
 *
 * SPDX-License-Identifier: MIT
 *
 */

struct VSInput
{
    float3 Pos;
    float3 Normal;
    float2 UV;
};

struct VSOutput
{
    float4 Pos : SV_POSITION;
    float3 Normal;
    float2 UV;
};

struct DSOutput
{
	float4 Pos : SV_POSITION;
	float3 Normal;
	float2 UV;
	float3 ViewVec;
	float3 LightVec;
	float3 EyePos;
	float3 WorldPos;
};

struct UBO
{
    float4x4 projection;
    float4x4 modelview;
    float4 lightPos;
    float4 frustumPlanes[6];
    float displacementFactor;
    float tessellationFactor;
    float2 viewportDim;
    float tessellatedEdgeSize;
};
ConstantBuffer<UBO> ubo;

Sampler2D samplerHeight;
Sampler2DArray samplerLayers;

struct HSOutput
{
	float4 Pos : SV_POSITION;
	float3 Normal : NORMAL0;
	float2 UV : TEXCOORD0;
};

struct ConstantsHSOutput
{
    float TessLevelOuter[4] : SV_TessFactor;
    float TessLevelInner[2] : SV_InsideTessFactor;
};

// Calculate the tessellation factor based on screen space
// dimensions of the edge
float screenSpaceTessFactor(float4 p0, float4 p1)
{
    // Calculate edge mid point
    float4 midPoint = 0.5 * (p0 + p1);
    // Sphere radius as distance between the control points
    float radius = distance(p0, p1) / 2.0;

    // View space
    float4 v0 = mul(ubo.modelview, midPoint);

    // Project into clip space
    float4 clip0 = mul(ubo.projection, (v0 - float4(radius, float3(0.0, 0.0, 0.0))));
    float4 clip1 = mul(ubo.projection, (v0 + float4(radius, float3(0.0, 0.0, 0.0))));

    // Get normalized device coordinates
    clip0 /= clip0.w;
    clip1 /= clip1.w;

    // Convert to viewport coordinates
    clip0.xy *= ubo.viewportDim;
    clip1.xy *= ubo.viewportDim;

    // Return the tessellation factor based on the screen size
    // given by the distance of the two edge control points in screen space
    // and a reference (min.) tessellation size for the edge set by the application
    return clamp(distance(clip0, clip1) / ubo.tessellatedEdgeSize * ubo.tessellationFactor, 1.0, 64.0);
}

// Checks the current's patch visibility against the frustum using a sphere check
// Sphere radius is given by the patch size
bool frustumCheck(float4 Pos, float2 inUV)
{
    // Fixed radius (increase if patch size is increased in example)
    const float radius = 8.0f;
    float4 pos = Pos;
    pos.y -= samplerHeight.SampleLevel(inUV, 0.0).r * ubo.displacementFactor;

    // Check sphere against frustum planes
    for (int i = 0; i < 6; i++) {
        if (dot(pos, ubo.frustumPlanes[i]) + radius < 0.0)
        {
            return false;
        }
    }
    return true;
}

ConstantsHSOutput ConstantsHS(InputPatch<VSOutput, 4> patch)
{
    ConstantsHSOutput output;

    if (!frustumCheck(patch[0].Pos, patch[0].UV))
    {
        output.TessLevelInner[0] = 0.0;
        output.TessLevelInner[1] = 0.0;
        output.TessLevelOuter[0] = 0.0;
        output.TessLevelOuter[1] = 0.0;
        output.TessLevelOuter[2] = 0.0;
        output.TessLevelOuter[3] = 0.0;
    }
    else
    {
        if (ubo.tessellationFactor > 0.0)
        {
            output.TessLevelOuter[0] = screenSpaceTessFactor(patch[3].Pos, patch[0].Pos);
            output.TessLevelOuter[1] = screenSpaceTessFactor(patch[0].Pos, patch[1].Pos);
            output.TessLevelOuter[2] = screenSpaceTessFactor(patch[1].Pos, patch[2].Pos);
            output.TessLevelOuter[3] = screenSpaceTessFactor(patch[2].Pos, patch[3].Pos);
            output.TessLevelInner[0] = lerp(output.TessLevelOuter[0], output.TessLevelOuter[3], 0.5);
            output.TessLevelInner[1] = lerp(output.TessLevelOuter[2], output.TessLevelOuter[1], 0.5);
        }
        else
        {
            // Tessellation factor can be set to zero by example
            // to demonstrate a simple passthrough
            output.TessLevelInner[0] = 1.0;
            output.TessLevelInner[1] = 1.0;
            output.TessLevelOuter[0] = 1.0;
            output.TessLevelOuter[1] = 1.0;
            output.TessLevelOuter[2] = 1.0;
            output.TessLevelOuter[3] = 1.0;
        }
    }

    return output;
}

float3 sampleTerrainLayer(float2 inUV)
{
	// Define some layer ranges for sampling depending on terrain height
	float2 layers[6];
	layers[0] = float2(-10.0, 10.0);
	layers[1] = float2(5.0, 45.0);
	layers[2] = float2(45.0, 80.0);
	layers[3] = float2(75.0, 100.0);
	layers[4] = float2(95.0, 140.0);
	layers[5] = float2(140.0, 190.0);

	float3 color = float3(0.0, 0.0, 0.0);

    // Get height from displacement map
    float height = samplerHeight.SampleLevel(inUV, 0.0).r * 255.0;

	for (int i = 0; i < 6; i++)
	{
		float range = layers[i].y - layers[i].x;
		float weight = (range - abs(height - layers[i].y)) / range;
        weight = max(0.0, weight);
        color += weight * samplerLayers.Sample(float3(inUV * 16.0, i)).rgb;
	}

	return color;
}

float fog(float density, float4 FragCoord)
{
	const float LOG2 = -1.442695;
	float dist = FragCoord.z / FragCoord.w * 0.1;
	float d = density * dist;
	return 1.0 - clamp(exp2(d * d * LOG2), 0.0, 1.0);
}

[shader("vertex")]
VSOutput vertexMain(VSInput input)
{
    VSOutput output;
    output.Pos = float4(input.Pos.xyz, 1.0);
    output.UV = input.UV;
    output.Normal = input.Normal;
    return output;
}

[shader("hull")]
[domain("quad")]
[partitioning("integer")]
[outputtopology("triangle_cw")]
[outputcontrolpoints(4)]
[patchconstantfunc("ConstantsHS")]
[maxtessfactor(20.0f)]
HSOutput hullMain(InputPatch<VSOutput, 4> patch, uint InvocationID: SV_OutputControlPointID)
{
    HSOutput output;
    output.Pos = patch[InvocationID].Pos;
    output.Normal = patch[InvocationID].Normal;
    output.UV = patch[InvocationID].UV;
    return output;
}

[shader("domain")]
[domain("quad")]
DSOutput domainMain(ConstantsHSOutput input, float2 TessCoord: SV_DomainLocation, const OutputPatch<HSOutput, 4> patch)
{
    // Interpolate UV coordinates
    DSOutput output;
    float2 uv1 = lerp(patch[0].UV, patch[1].UV, TessCoord.x);
    float2 uv2 = lerp(patch[3].UV, patch[2].UV, TessCoord.x);
    output.UV = lerp(uv1, uv2, TessCoord.y);

    float3 n1 = lerp(patch[0].Normal, patch[1].Normal, TessCoord.x);
    float3 n2 = lerp(patch[3].Normal, patch[2].Normal, TessCoord.x);
    output.Normal = lerp(n1, n2, TessCoord.y);

    // Interpolate positions
    float4 pos1 = lerp(patch[0].Pos, patch[1].Pos, TessCoord.x);
    float4 pos2 = lerp(patch[3].Pos, patch[2].Pos, TessCoord.x);
    float4 pos = lerp(pos1, pos2, TessCoord.y);
    // Displace
    pos.y -= samplerHeight.SampleLevel(output.UV, 0.0).r * ubo.displacementFactor;
    // Perspective projection
    output.Pos = mul(ubo.projection, mul(ubo.modelview, pos));

    // Calculate vectors for lighting based on tessellated position
    output.ViewVec = -pos.xyz;
    output.LightVec = normalize(ubo.lightPos.xyz + output.ViewVec);
    output.WorldPos = pos.xyz;
    output.EyePos = mul(ubo.modelview, pos).xyz;
    return output;
}

[shader("fragment")]
float4 fragmentMain(DSOutput input)
{
	float3 N = normalize(input.Normal);
	float3 L = normalize(input.LightVec);
	float3 ambient = float3(0.5, 0.5, 0.5);
	float3 diffuse = max(dot(N, L), 0.0) * float3(1.0, 1.0, 1.0);
	float4 color = float4((ambient + diffuse) * sampleTerrainLayer(input.UV), 1.0);
	const float4 fogColor = float4(0.47, 0.5, 0.67, 0.0);
	return lerp(color, fogColor, fog(0.25, input.Pos));
}
