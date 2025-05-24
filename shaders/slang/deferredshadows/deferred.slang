// Copyright 2020 Google LLC

#define LIGHT_COUNT 3
#define SHADOW_FACTOR 0.25
#define AMBIENT_LIGHT 0.1
#define USE_PCF

struct VSOutput
{
    float4 Pos : SV_POSITION;
    float2 UV;
};

[[vk::binding(1, 0)]] Sampler2D samplerPosition;
[[vk::binding(2, 0)]] Sampler2D samplerNormal;
[[vk::binding(3, 0)]] Sampler2D samplerAlbedo;

struct Light
{
    float4 position;
    float4 target;
    float4 color;
    float4x4 viewMatrix;
};

struct UBO
{
	float4 viewPos;
	Light lights[LIGHT_COUNT];
	int useShadows;
	int displayDebugTarget;
};
[[vk::binding(4, 0)]] ConstantBuffer<UBO> ubo;

// Depth from the light's point of view
// layout (binding = 5) uniform sampler2DShadow samplerShadowMap;
[[vk::binding(5, 0)]] Sampler2DArray samplerShadowMap;

float textureProj(float4 P, float layer, float2 offset)
{
	float shadow = 1.0;
	float4 shadowCoord = P / P.w;
	shadowCoord.xy = shadowCoord.xy * 0.5 + 0.5;

	if (shadowCoord.z > -1.0 && shadowCoord.z < 1.0)
    {
        float dist = samplerShadowMap.Sample(float3(shadowCoord.xy + offset, layer)).r;
		if (shadowCoord.w > 0.0 && dist < shadowCoord.z)
		{
			shadow = SHADOW_FACTOR;
		}
	}
	return shadow;
}

float filterPCF(float4 sc, float layer)
{
    int2 texDim; int elements; int levels;
    samplerShadowMap.GetDimensions(0, texDim.x, texDim.y, elements, levels);
	float scale = 1.5;
	float dx = scale * 1.0 / float(texDim.x);
	float dy = scale * 1.0 / float(texDim.y);

	float shadowFactor = 0.0;
	int count = 0;
	int range = 1;

	for (int x = -range; x <= range; x++)
	{
		for (int y = -range; y <= range; y++)
		{
			shadowFactor += textureProj(sc, layer, float2(dx*x, dy*y));
			count++;
		}

	}
	return shadowFactor / count;
}

float3 shadow(float3 fragcolor, float3 fragPos)
{
	for (int i = 0; i < LIGHT_COUNT; ++i)
	{
		float4 shadowClip = mul(ubo.lights[i].viewMatrix, float4(fragPos.xyz, 1.0));

		float shadowFactor;
		#ifdef USE_PCF
			shadowFactor= filterPCF(shadowClip, i);
		#else
			shadowFactor = textureProj(shadowClip, i, float2(0.0, 0.0));
		#endif

		fragcolor *= shadowFactor;
	}
	return fragcolor;
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
    // Get G-Buffer values
    float3 fragPos = samplerPosition.Sample(input.UV).rgb;
    float3 normal = samplerNormal.Sample(input.UV).rgb;
    float4 albedo = samplerAlbedo.Sample(input.UV);

	float3 fragcolor;

	// Debug display
	if (ubo.displayDebugTarget > 0) {
		switch (ubo.displayDebugTarget) {
			case 1: 
				fragcolor.rgb = shadow(float3(1.0, 1.0, 1.0), fragPos);
				break;
			case 2: 
				fragcolor.rgb = fragPos;
				break;
			case 3: 
				fragcolor.rgb = normal;
				break;
			case 4: 
				fragcolor.rgb = albedo.rgb;
				break;
			case 5: 
				fragcolor.rgb = albedo.aaa;
				break;
		}		
		return float4(fragcolor, 1.0);
	}

	// Ambient part
	fragcolor  = albedo.rgb * AMBIENT_LIGHT;

	float3 N = normalize(normal);

	for(int i = 0; i < LIGHT_COUNT; ++i)
	{
		// Vector to light
		float3 L = ubo.lights[i].position.xyz - fragPos;
		// Distance from light to fragment position
		float dist = length(L);
		L = normalize(L);

		// Viewer to fragment
		float3 V = ubo.viewPos.xyz - fragPos;
		V = normalize(V);

		float lightCosInnerAngle = cos(radians(15.0));
		float lightCosOuterAngle = cos(radians(25.0));
		float lightRange = 100.0;

		// Direction vector from source to target
		float3 dir = normalize(ubo.lights[i].position.xyz - ubo.lights[i].target.xyz);

		// Dual cone spot light with smooth transition between inner and outer angle
		float cosDir = dot(L, dir);
		float spotEffect = smoothstep(lightCosOuterAngle, lightCosInnerAngle, cosDir);
		float heightAttenuation = smoothstep(lightRange, 0.0f, dist);

		// Diffuse lighting
		float NdotL = max(0.0, dot(N, L));
		float3 diff = NdotL.xxx;

		// Specular lighting
		float3 R = reflect(-L, N);
		float NdotR = max(0.0, dot(R, V));
		float3 spec = (pow(NdotR, 16.0) * albedo.a * 2.5).xxx;

		fragcolor += float3((diff + spec) * spotEffect * heightAttenuation) * ubo.lights[i].color.rgb * albedo.rgb;
	}

	// Shadow calculations in a separate pass
	if (ubo.useShadows > 0)
	{
		fragcolor = shadow(fragcolor, fragPos);
	}

	return float4(fragcolor, 1);
}