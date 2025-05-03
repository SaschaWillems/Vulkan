/* Copyright (c) 2025, Sascha Willems
 *
 * SPDX-License-Identifier: MIT
 *
 */

struct VSInput
{
	float3 Pos;
	float3 Normal;
	float3 Color;
};

struct VSOutput
{
    float4 Pos : SV_POSITION;
    float3 Normal;
    float3 Color;
};

struct GSOutput
{
    float4 Pos : SV_POSITION;
    uint ViewportIndex : SV_ViewportArrayIndex;
    uint PrimitiveID : SV_PrimitiveID;
    float3 Normal;
    float3 Color;
    float3 ViewVec;
    float3 LightVec;
}

struct UBO
{
    float4x4 projection[2];
    float4x4 modelview[2];
    float4 lightPos;
};
ConstantBuffer<UBO> ubo;

[shader("vertex")]
VSOutput vertexMain(VSInput input)
{
    VSOutput output;
    output.Color = input.Color;
    output.Normal = input.Normal;
    output.Pos = float4(input.Pos.xyz, 1.0);
    return output;
}

[shader("geometry")]
[maxvertexcount(3)]
[instance(2)]
void geometryMain(triangle VSOutput input[3], inout TriangleStream<GSOutput> outStream, uint InvocationID: SV_GSInstanceID, uint PrimitiveID: SV_PrimitiveID)
{
    for (int i = 0; i < 3; i++)
    {
        GSOutput output;
        output.Normal = mul((float3x3)ubo.modelview[InvocationID], input[i].Normal);
        output.Color = input[i].Color;

        float4 pos = input[i].Pos;
        float4 worldPos = mul(ubo.modelview[InvocationID], pos);

        float3 lPos = mul(ubo.modelview[InvocationID], ubo.lightPos).xyz;
        output.LightVec = lPos - worldPos.xyz;
        output.ViewVec = -worldPos.xyz;

        output.Pos = mul(ubo.projection[InvocationID], worldPos);

        // Set the viewport index that the vertex will be emitted to
        output.ViewportIndex = InvocationID;
        output.PrimitiveID = PrimitiveID;
        outStream.Append(output);
    }

    outStream.RestartStrip();
}

[shader("fragment")]
float4 fragmentMain(GSOutput input)
{
	float3 N = normalize(input.Normal);
	float3 L = normalize(input.LightVec);
	float3 V = normalize(input.ViewVec);
	float3 R = reflect(-L, N);
	float3 ambient = float3(0.1, 0.1, 0.1);
	float3 diffuse = max(dot(N, L), 0.0) * float3(1.0, 1.0, 1.0);
	float3 specular = pow(max(dot(R, V), 0.0), 16.0) * float3(0.75, 0.75, 0.75);
	return float4((ambient + diffuse) * input.Color.rgb + specular, 1.0);
}