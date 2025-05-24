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
    float3 ViewVec;
    float3 LightVec;
};

struct UBO
{
    float4x4 projection;
    float4x4 model;
    float4 lightPos;
};
ConstantBuffer<UBO> ubo;

// We use this constant to control the flow of the shader depending on the
// lighting model selected at pipeline creation time
[[SpecializationConstant]] const int LIGHTING_MODEL = 0;

[shader("vertex")]
VSOutput vertexMain(VSInput input)
{
    VSOutput output;
    output.Color = input.Color;
    output.Pos = mul(ubo.projection, mul(ubo.model, float4(input.Pos.xyz, 1.0)));
    float4 pos = mul(ubo.model, float4(input.Pos, 1.0));
    output.Normal = mul((float3x3)ubo.model, input.Normal);
    float3 lPos = mul((float3x3)ubo.model, ubo.lightPos.xyz);
    output.LightVec = lPos - pos.xyz;
    output.ViewVec = -pos.xyz;
    return output;
}

[shader("fragment")]
float4 fragmentMain(VSOutput input)
{
    float3 outColor = float3(0.0);

    switch (LIGHTING_MODEL) {
    case 0: // Phong
    {
        float3 ambient = input.Color * float3(0.25, 0.25, 0.25);
        float3 N = normalize(input.Normal);
        float3 L = normalize(input.LightVec);
        float3 V = normalize(input.ViewVec);
        float3 R = reflect(-L, N);
        float3 diffuse = max(dot(N, L), 0.0) * input.Color;
        float3 specular = pow(max(dot(R, V), 0.0), 32.0) * float3(0.75);
        outColor = ambient + diffuse * 1.75 + specular;
		break;
    }
    case 1: // Toon
    {

        float3 N = normalize(input.Normal);
        float3 L = normalize(input.LightVec);
        float intensity = dot(N, L);
        float3 color;
        if (intensity > 0.98)
            color = input.Color * 1.5;
        else if (intensity > 0.9)
            color = input.Color * 1.0;
        else if (intensity > 0.5)
            color = input.Color * 0.6;
        else if (intensity > 0.25)
            color = input.Color * 0.4;
        else
            color = input.Color * 0.2;
        outColor = color;
        break;
    }
    case 2: // No shading
    {
        outColor = input.Color;
        break;
    }
    case 3: // Greyscale
        outColor = dot(input.Color, float3(0.299, 0.587, 0.114));
        break;
    }

    // The scene itself is a bit dark, so brigthen it up a bit
    return float4(outColor * 1.25, 1.0);
}