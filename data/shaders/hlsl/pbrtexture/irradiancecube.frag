// Copyright 2020 Google LLC

TextureCube textureEnv : register(t0);
SamplerState samplerEnv : register(s0);

struct PushConsts {
[[vk::offset(64)]] float deltaPhi;
[[vk::offset(68)]] float deltaTheta;
};
[[vk::push_constant]]PushConsts consts;

#define PI 3.1415926535897932384626433832795

float4 main([[vk::location(0)]] float3 inPos : TEXCOORD0) : SV_TARGET
{
	float3 N = normalize(inPos);
	float3 up = float3(0.0, 1.0, 0.0);
	float3 right = normalize(cross(up, N));
	up = cross(N, right);

	const float TWO_PI = PI * 2.0;
	const float HALF_PI = PI * 0.5;

	float3 color = float3(0.0, 0.0, 0.0);
	uint sampleCount = 0u;
	for (float phi = 0.0; phi < TWO_PI; phi += consts.deltaPhi) {
		for (float theta = 0.0; theta < HALF_PI; theta += consts.deltaTheta) {
			float3 tempVec = cos(phi) * right + sin(phi) * up;
			float3 sampleVector = cos(theta) * N + sin(theta) * tempVec;
			color += textureEnv.Sample(samplerEnv, sampleVector).rgb * cos(theta) * sin(theta);
			sampleCount++;
		}
	}
	return float4(PI * color / float(sampleCount), 1.0);
}
