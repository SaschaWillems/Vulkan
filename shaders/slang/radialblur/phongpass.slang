/* Copyright (c) 2025, Sascha Willems
 *
 * SPDX-License-Identifier: MIT
 *
 */

struct VSInput
{
    float4 Pos;
    float2 UV;
    float3 Color;
    float3 Normal;
};

struct VSOutput
{
    float4 Pos : SV_POSITION;
    float3 Normal;
    float3 Color;
    float3 EyePos;
    float3 LightVec;
    float2 UV;
};

struct UBO
{
    float4x4 projection;
    float4x4 model;
    float gradientPos;
};
ConstantBuffer<UBO> ubo;

Sampler2D samplerGradientRamp;

[shader("vertex")]
VSOutput vertexMain(VSInput input)
{
    VSOutput output;
    output.Normal = input.Normal;
    output.Color = input.Color;
    output.UV = float2(ubo.gradientPos, 0.0);
    output.Pos = mul(ubo.projection, mul(ubo.model, input.Pos));
    output.EyePos = mul(ubo.model, input.Pos).xyz;
    float4 lightPos = float4(0.0, 0.0, -5.0, 1.0); // * ubo.model;
    output.LightVec = normalize(lightPos.xyz - input.Pos.xyz);
    return output;
}

[shader("fragment")]
float4 fragmentMain(VSOutput input)
{
	// No light calculations for glow color
	// Use max. color channel value
	// to detect bright glow emitters
	if ((input.Color.r >= 0.9) || (input.Color.g >= 0.9) || (input.Color.b >= 0.9))
    {
        return float4(samplerGradientRamp.Sample(input.UV).rgb, 1);
	} else {
		float3 Eye = normalize(-input.EyePos);
		float3 Reflected = normalize(reflect(-input.LightVec, input.Normal));
		float4 IAmbient = float4(0.2, 0.2, 0.2, 1.0);
		float4 IDiffuse = float4(0.5, 0.5, 0.5, 0.5) * max(dot(input.Normal, input.LightVec), 0.0);
		float specular = 0.25;
		float4 ISpecular = float4(0.5, 0.5, 0.5, 1.0) * pow(max(dot(Reflected, Eye), 0.0), 4.0) * specular;
		return float4((IAmbient + IDiffuse) * float4(input.Color, 1.0) + ISpecular);
	}
}