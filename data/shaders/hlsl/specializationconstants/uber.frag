// Copyright 2020 Google LLC

Texture2D textureColormap : register(t1);
SamplerState samplerColormap : register(s1);
Texture2D textureDiscard : register(t2);
SamplerState samplerDiscard : register(s2);

struct VSOutput
{
[[vk::location(0)]] float3 Normal : NORMAL0;
[[vk::location(1)]] float3 Color : COLOR0;
[[vk::location(2)]] float2 UV : TEXCOORD0;
[[vk::location(3)]] float3 ViewVec : TEXCOORD1;
[[vk::location(4)]] float3 LightVec : TEXCOORD2;
};

// We use this constant to control the flow of the shader depending on the
// lighting model selected at pipeline creation time
[[vk::constant_id(0)]] const int LIGHTING_MODEL = 0;
// Parameter for the toon shading part of the shader
[[vk::constant_id(1)]] const /*float*/int PARAM_TOON_DESATURATION = 0.0f;

float4 main(VSOutput input) : SV_TARGET
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
			float4 color = textureColormap.Sample(samplerColormap, input.UV).rrra;
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