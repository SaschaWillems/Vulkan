// Copyright 2020 Google LLC

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
    float3 WorldPos;
    float3 Normal;
    float2 UV;
    float3 Tangent;
};

struct UBO  {
	float4x4 projection;
	float4x4 model;
	float4x4 view;
	float3 camPos;
};
ConstantBuffer<UBO> ubo;

struct UBOParams {
	float4 lights[4];
	float exposure;
	float gamma;
};
ConstantBuffer<UBOParams> uboParams;

SamplerCube samplerIrradiance;
Sampler2D samplerBRDFLUT;
SamplerCube prefilteredMapSampler;

Sampler2D albedoMapSampler;
Sampler2D normalMapSampler;
Sampler2D aoMapSampler;
Sampler2D metallicMapSampler;
Sampler2D roughnessMapSampler;

#define PI 3.1415926535897932384626433832795
#define ALBEDO(uv) pow(albedoMapSampler.Sample(uv).rgb, float3(2.2, 2.2, 2.2))

// From http://filmicgames.com/archives/75
float3 Uncharted2Tonemap(float3 x)
{
	float A = 0.15;
	float B = 0.50;
	float C = 0.10;
	float D = 0.20;
	float E = 0.02;
	float F = 0.30;
	return ((x*(A*x+C*B)+D*E)/(x*(A*x+B)+D*F))-E/F;
}

// Normal Distribution function --------------------------------------
float D_GGX(float dotNH, float roughness)
{
	float alpha = roughness * roughness;
	float alpha2 = alpha * alpha;
	float denom = dotNH * dotNH * (alpha2 - 1.0) + 1.0;
	return (alpha2)/(PI * denom*denom);
}

// Geometric Shadowing function --------------------------------------
float G_SchlicksmithGGX(float dotNL, float dotNV, float roughness)
{
	float r = (roughness + 1.0);
	float k = (r*r) / 8.0;
	float GL = dotNL / (dotNL * (1.0 - k) + k);
	float GV = dotNV / (dotNV * (1.0 - k) + k);
	return GL * GV;
}

// Fresnel function ----------------------------------------------------
float3 F_Schlick(float cosTheta, float3 F0)
{
	return F0 + (1.0 - F0) * pow(1.0 - cosTheta, 5.0);
}
float3 F_SchlickR(float cosTheta, float3 F0, float roughness)
{
	return F0 + (max((1.0 - roughness).xxx, F0) - F0) * pow(1.0 - cosTheta, 5.0);
}

float3 prefilteredReflection(float3 R, float roughness)
{
	const float MAX_REFLECTION_LOD = 9.0; // todo: param/const
	float lod = roughness * MAX_REFLECTION_LOD;
	float lodf = floor(lod);
    float lodc = ceil(lod);
    float3 a = prefilteredMapSampler.SampleLevel(R, lodf).rgb;
    float3 b = prefilteredMapSampler.SampleLevel(R, lodc).rgb;
	return lerp(a, b, lod - lodf);
}

float3 specularContribution(float2 inUV, float3 L, float3 V, float3 N, float3 F0, float metallic, float roughness)
{
	// Precalculate vectors and dot products
	float3 H = normalize (V + L);
	float dotNH = clamp(dot(N, H), 0.0, 1.0);
	float dotNV = clamp(dot(N, V), 0.0, 1.0);
	float dotNL = clamp(dot(N, L), 0.0, 1.0);

	// Light color fixed
	float3 lightColor = float3(1.0, 1.0, 1.0);

	float3 color = float3(0.0, 0.0, 0.0);

	if (dotNL > 0.0) {
		// D = Normal distribution (Distribution of the microfacets)
		float D = D_GGX(dotNH, roughness);
		// G = Geometric shadowing term (Microfacets shadowing)
		float G = G_SchlicksmithGGX(dotNL, dotNV, roughness);
		// F = Fresnel factor (Reflectance depending on angle of incidence)
		float3 F = F_Schlick(dotNV, F0);
		float3 spec = D * F * G / (4.0 * dotNL * dotNV + 0.001);
		float3 kD = (float3(1.0, 1.0, 1.0) - F) * (1.0 - metallic);
		color += (kD * ALBEDO(inUV) / PI + spec) * dotNL;
	}

	return color;
}

float3 calculateNormal(VSOutput input)
{
    float3 tangentNormal = normalMapSampler.Sample(input.UV).xyz * 2.0 - 1.0;

	float3 N = normalize(input.Normal);
	float3 T = normalize(input.Tangent);
	float3 B = normalize(cross(N, T));
	float3x3 TBN = transpose(float3x3(T, B, N));

	return normalize(mul(TBN, tangentNormal));
}

[shader("vertex")]
VSOutput vertexMain(VSInput input)
{
	VSOutput output;
	float3 locPos = mul(ubo.model, float4(input.Pos, 1.0)).xyz;
	output.WorldPos = locPos;
	output.Normal = mul((float3x3)ubo.model, input.Normal);
	output.Tangent = mul((float3x3)ubo.model, input.Tangent.xyz);
	output.UV = input.UV;
	output.Pos = mul(ubo.projection, mul(ubo.view, float4(output.WorldPos, 1.0)));
	return output;
}

[shader("fragment")]
float4 fragmentMain(VSOutput input)
{
	float3 N = calculateNormal(input);
	float3 V = normalize(ubo.camPos - input.WorldPos);
	float3 R = reflect(-V, N);

    float metallic = metallicMapSampler.Sample(input.UV).r;
    float roughness = roughnessMapSampler.Sample(input.UV).r;

	float3 F0 = float3(0.04, 0.04, 0.04);
	F0 = lerp(F0, ALBEDO(input.UV), metallic);

	float3 Lo = float3(0.0, 0.0, 0.0);
	for(int i = 0; i < 4; i++) {
		float3 L = normalize(uboParams.lights[i].xyz - input.WorldPos);
		Lo += specularContribution(input.UV, L, V, N, F0, metallic, roughness);
	}

    float2 brdf = samplerBRDFLUT.Sample(float2(max(dot(N, V), 0.0), roughness)).rg;
    float3 reflection = prefilteredReflection(R, roughness).rgb;
    float3 irradiance = samplerIrradiance.Sample(N).rgb;

	// Diffuse based on irradiance
	float3 diffuse = irradiance * ALBEDO(input.UV);

	float3 F = F_SchlickR(max(dot(N, V), 0.0), F0, roughness);

	// Specular reflectance
	float3 specular = reflection * (F * brdf.x + brdf.y);

	// Ambient part
	float3 kD = 1.0 - F;
    kD *= 1.0 - metallic;
    float3 ambient = (kD * diffuse + specular) * aoMapSampler.Sample(input.UV).rrr;

	float3 color = ambient + Lo;

	// Tone mapping
	color = Uncharted2Tonemap(color * uboParams.exposure);
	color = color * (1.0f / Uncharted2Tonemap((11.2f).xxx));
	// Gamma correction
	color = pow(color, (1.0f / uboParams.gamma).xxx);

	return float4(color, 1.0);
}