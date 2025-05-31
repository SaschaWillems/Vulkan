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
};

struct VSOutput
{
    float4 Pos : SV_POSITION;
    float3 Normal;
    float3 Color;
    float2 UV;
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

Sampler2D samplerColormap;
Sampler2D samplerDiscard;

// We use this constant to control the flow of the shader depending on the
// lighting model selected at pipeline creation time
[[SpecializationConstant]] const int LIGHTING_MODEL = 0;
// Parameter for the toon shading part of the shader
[[SpecializationConstant]] const int PARAM_TOON_DESATURATION = 0;

[shader("vertex")]
VSOutput vertexMain(VSInput input)
{
    VSOutput output;
    output.Normal = input.Normal;
    output.Color = input.Color;
    output.UV = input.UV;
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
	switch (LIGHTING_MODEL) {
		case 0: // Phong
		{
			float3 ambient = input.Color * float3(0.25, 0.25, 0.25);
			float3 N = normalize(input.Normal);
			float3 L = normalize(input.LightVec);
			float3 V = normalize(input.ViewVec);
			float3 R = reflect(-L, N);
			float3 diffuse = max(dot(N, L), 0.0) * input.Color;
			float3 specular = pow(max(dot(R, V), 0.0), 32.0) * float3(0.75, 0.75, 0.75);
			return float4(ambient + diffuse * 1.75 + specular, 1.0);
		}
		case 1: // Toon
		{

			float3 N = normalize(input.Normal);
			float3 L = normalize(input.LightVec);
			float intensity = dot(N,L);
			float3 color;
			if (intensity > 0.98)
				color = input.Color * 1.5;
			else if  (intensity > 0.9)
				color = input.Color * 1.0;
			else if (intensity > 0.5)
				color = input.Color * 0.6;
			else if (intensity > 0.25)
				color = input.Color * 0.4;
			else
				color = input.Color * 0.2;
			// Desaturate a bit
			color = float3(lerp(color, dot(float3(0.2126,0.7152,0.0722), color).xxx, asfloat(PARAM_TOON_DESATURATION)));
			return float4(color, 1);
		}
		case 2: // Textured
        {
            float4 color = samplerColormap.Sample(input.UV).rrra;
			float3 ambient = color.rgb * float3(0.25, 0.25, 0.25) * input.Color;
			float3 N = normalize(input.Normal);
			float3 L = normalize(input.LightVec);
			float3 V = normalize(input.ViewVec);
			float3 R = reflect(-L, N);
			float3 diffuse = max(dot(N, L), 0.0) * color.rgb;
			float specular = pow(max(dot(R, V), 0.0), 32.0) * color.a;
			return float4(ambient + diffuse + specular.xxx, 1.0);
		}
	}

	return float4(0, 0, 0, 0);
}