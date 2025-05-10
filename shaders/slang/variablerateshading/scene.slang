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
    float3 Color;
    float4 Tangent;
};

struct VSOutput
{
    float4 Pos : SV_POSITION;
    float3 Normal;
    float3 Color;
    float2 UV;
    float3 ViewVec;
    float3 LightVec;
    float4 Tangent;
}; 

struct UBO
{
	float4x4 projection;
	float4x4 view;
	float4x4 model;
	float4 lightPos;
	float4 viewPos;
	int colorShadingRates;
};
ConstantBuffer<UBO> ubo;

[[vk::binding(0, 1)]] Sampler2D samplerColorMap;
[[vk::binding(1, 1)]] Sampler2D samplerNormalMap;

[[SpecializationConstant]] const bool ALPHA_MASK = false;
[[SpecializationConstant]] const float ALPHA_MASK_CUTOFF = 0.0;

[shader("vertex")]
VSOutput vertexMain(VSInput input)
{
    VSOutput output;
    output.Normal = input.Normal;
    output.Color = input.Color;
    output.UV = input.UV;
    output.Tangent = input.Tangent;

    float4x4 modelView = mul(ubo.view, ubo.model);

    output.Pos = mul(ubo.projection, mul(modelView, float4(input.Pos.xyz, 1.0)));

    output.Normal = mul((float3x3)ubo.model, input.Normal);
    float4 pos = mul(ubo.model, float4(input.Pos, 1.0));
    output.LightVec = ubo.lightPos.xyz - pos.xyz;
    output.ViewVec = ubo.viewPos.xyz - pos.xyz;
    return output;
}

static const uint SHADING_RATE_PER_PIXEL = 0;
static const uint SHADING_RATE_PER_2X1_PIXELS = 6;
static const uint SHADING_RATE_PER_1X2_PIXELS = 7;
static const uint SHADING_RATE_PER_2X2_PIXELS = 8;
static const uint SHADING_RATE_PER_4X2_PIXELS = 9;
static const uint SHADING_RATE_PER_2X4_PIXELS = 10;

[shader("fragment")]
float4 fragmentMain(VSOutput input, uint shadingRate : SV_ShadingRate)
{
    float4 color = samplerColorMap.Sample(input.UV) * float4(input.Color, 1.0);

	if (ALPHA_MASK) {
        if (color.a < ALPHA_MASK_CUTOFF) {
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
	color =  float4(diffuse * color.rgb + specular, color.a);

	if (ubo.colorShadingRates == 1) {
        switch (shadingRate) {
        	case SHADING_RATE_PER_PIXEL:
				return color * float4(0.0, 0.8, 0.4, 1.0);
			case SHADING_RATE_PER_2X1_PIXELS:
				return color * float4(0.2, 0.6, 1.0, 1.0);
			case SHADING_RATE_PER_1X2_PIXELS:
				return color * float4(0.0, 0.4, 0.8, 1.0);
			case SHADING_RATE_PER_2X2_PIXELS:
				return color * float4(1.0, 1.0, 0.2, 1.0);
			case SHADING_RATE_PER_4X2_PIXELS:
				return color * float4(0.8, 0.8, 0.0, 1.0);
			case SHADING_RATE_PER_2X4_PIXELS:
				return color * float4(1.0, 0.4, 0.2, 1.0);
		default:
			return color * float4(0.8, 0.0, 0.0, 1.0);
		}
	}

	return color;
}