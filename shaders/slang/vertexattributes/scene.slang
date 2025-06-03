/* Copyright (c) 2025, Sascha Willems
 *
 * SPDX-License-Identifier: MIT
 *
 */

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
    float4 Tangent;
};

struct VSOutput
{
    float4 Pos : SV_POSITION;
    float3 Normal;
    float2 UV;
    float3 ViewVec;
    float3 LightVec;
    float4 Tangent;
};

struct UBO
{
    float4x4 projection;
    float4x4 view;
    float4 lightPos;
    float4 viewPos;
};
ConstantBuffer<UBO> ubo;

[[vk::binding(0, 1)]] Sampler2D samplerColorMap;
[[vk::binding(1, 1)]] Sampler2D samplerNormalMap;

[shader("vertex")]
VSOutput vertexMain(VSInput input, uniform float4x4 modelMat)
{
    VSOutput output;
    output.Normal = input.Normal;
    output.UV = input.UV;
    output.Tangent = input.Tangent;

    float4x4 modelView = mul(ubo.view, modelMat);

    output.Pos = mul(ubo.projection, mul(modelView, float4(input.Pos.xyz, 1.0)));

    output.Normal = mul((float3x3)modelMat, input.Normal);
    float4 pos = mul(modelMat, float4(input.Pos, 1.0));
    output.LightVec = ubo.lightPos.xyz - pos.xyz;
    output.ViewVec = ubo.viewPos.xyz - pos.xyz;
    return output;
}

[shader("fragment")]
float4 fragmentMain(VSOutput input, uniform float4x4 modelMat, uniform uint alphaMask, uniform float alphaMaskCutoff)
{
    float4 color = samplerColorMap.Sample(input.UV);

    if (alphaMask == 1) {
        if (color.a < alphaMaskCutoff) {
            discard;
        }
    }

    float3 N = normalize(input.Normal);
    float3 T = normalize(input.Tangent.xyz);
    float3 B = cross(input.Normal, input.Tangent.xyz) * input.Tangent.w;
    float3x3 TBN = float3x3(T, B, N);
    N = mul(normalize(samplerNormalMap.Sample(input.UV).xyz * 2.0 - float3(1.0, 1.0, 1.0)), TBN);

    const float ambient = 0.1;
    float3 L = normalize(input.LightVec);
    float3 V = normalize(input.ViewVec);
    float3 R = reflect(-L, N);
    float3 diffuse = max(dot(N, L), ambient).rrr;
    float3 specular = pow(max(dot(R, V), 0.0), 32.0);
    return float4(diffuse * color.rgb + specular, color.a);
}