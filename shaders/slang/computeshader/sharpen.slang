/* Copyright (c) 2025, Sascha Willems
 *
 * SPDX-License-Identifier: MIT
 *
 */

import shared;

[shader("compute")]
[numthreads(16, 16, 1)]
void computeMain(uint3 GlobalInvocationID : SV_DispatchThreadID)
{
	float r[9];
	float g[9];
	float b[9];

	// Fetch neighbouring texels
	int n = -1;
	for (int i=-1; i<2; ++i)
	{
		for(int j=-1; j<2; ++j)
		{
			n++;
			float3 rgb = inputImage[uint2(GlobalInvocationID.x + i, GlobalInvocationID.y + j)].rgb;
			r[n] = rgb.r;
			g[n] = rgb.g;
			b[n] = rgb.b;
		}
	}

	float kernel[9];
	kernel[0] = -1.0; kernel[1] = -1.0; kernel[2] = -1.0;
	kernel[3] = -1.0; kernel[4] =  9.0; kernel[5] = -1.0;
	kernel[6] = -1.0; kernel[7] = -1.0; kernel[8] = -1.0;

	float4 res = float4(
		conv(kernel, r, 1.0, 0.0),
		conv(kernel, g, 1.0, 0.0),
		conv(kernel, b, 1.0, 0.0),
		1.0);

	resultImage[int2(GlobalInvocationID.xy)] = res;
}