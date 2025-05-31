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
	float imageData[9];
	// Fetch neighbouring texels
	int n = -1;
	for (int i=-1; i<2; ++i)
	{
		for(int j=-1; j<2; ++j)
		{
			n++;
			float3 rgb = inputImage[uint2(GlobalInvocationID.x + i, GlobalInvocationID.y + j)].rgb;
			imageData[n] = (rgb.r + rgb.g + rgb.b) / 3.0;
		}
	}

	float kernel[9];
	kernel[0] = -1.0; kernel[1] =  0.0; kernel[2] =  0.0;
	kernel[3] = 0.0; kernel[4] = -1.0; kernel[5] =  0.0;
	kernel[6] = 0.0; kernel[7] =  0.0; kernel[8] = 2.0;

	float4 res = float4(conv(kernel, imageData, 1.0, 0.50).xxx, 1.0);

	resultImage[int2(GlobalInvocationID.xy)] = res;
}