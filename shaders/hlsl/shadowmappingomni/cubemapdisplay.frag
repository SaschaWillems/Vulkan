// Copyright 2020 Google LLC

TextureCube shadowCubeMapTexture : register(t1);
SamplerState shadowCubeMapSampler : register(s1);

float4 main([[vk::location(0)]] float2 inUV : TEXCOORD0) : SV_TARGET
{
	float4 outFragColor = float4(0, 0, 0, 0);
	outFragColor.rgb = float3(0.05, 0.05, 0.05);

	float3 samplePos = float3(0, 0, 0);

	// Crude statement to visualize different cube map faces based on UV coordinates
	int x = int(floor(inUV.x / 0.25f));
	int y = int(floor(inUV.y / (1.0 / 3.0)));
	if (y == 1) {
		float2 uv = float2(inUV.x * 4.0f, (inUV.y - 1.0/3.0) * 3.0);
		uv = 2.0 * float2(uv.x - float(x) * 1.0, uv.y) - 1.0;
		switch (x) {
			case 0:	// NEGATIVE_X
				samplePos = float3(-1.0f, uv.y, uv.x);
				break;
			case 1: // POSITIVE_Z
				samplePos = float3(uv.x, uv.y, 1.0f);
				break;
			case 2: // POSITIVE_X
				samplePos = float3(1.0, uv.y, -uv.x);
				break;
			case 3: // NEGATIVE_Z
				samplePos = float3(-uv.x, uv.y, -1.0f);
				break;
		}
	} else {
		if (x == 1) {
			float2 uv = float2((inUV.x - 0.25) * 4.0, (inUV.y - float(y) / 3.0) * 3.0);
			uv = 2.0 * uv - 1.0;
			switch (y) {
				case 0: // NEGATIVE_Y
					samplePos = float3(uv.x, -1.0f, uv.y);
					break;
				case 2: // POSITIVE_Y
					samplePos = float3(uv.x, 1.0f, -uv.y);
					break;
			}
		}
	}

	if ((samplePos.x != 0.0f) && (samplePos.y != 0.0f)) {
		float dist = length(shadowCubeMapTexture.Sample(shadowCubeMapSampler, samplePos).xyz) * 0.005;
		outFragColor = float4(dist.xxx, 1.0);
	}
	return outFragColor;
}