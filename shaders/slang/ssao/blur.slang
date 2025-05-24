/* Copyright (c) 2025, Sascha Willems
 *
 * SPDX-License-Identifier: MIT
 *
 */

import types;

Sampler2D samplerSSAO;

[shader("fragment")]
float4 fragmentMain(VSOutput input)
{
	const int blurRange = 2;
	int n = 0;
    int2 texDim;
    samplerSSAO.GetDimensions(texDim.x, texDim.y);
	float2 texelSize = 1.0 / (float2)texDim;
	float result = 0.0;
	for (int x = -blurRange; x <= blurRange; x++)
	{
		for (int y = -blurRange; y <= blurRange; y++)
		{
            float2 offset = float2(float(x), float(y)) * texelSize;
            result += samplerSSAO.Sample(input.UV + offset).r;
			n++;
		}
	}
	return result / (float(n));
}