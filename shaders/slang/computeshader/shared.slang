/* Copyright (c) 2025, Sascha Willems
 *
 * SPDX-License-Identifier: MIT
 *
 */

module shared;

public Texture2D inputImage;
public RWTexture2D<float4> resultImage;

public float conv(in float kernel[9], in float data[9], in float denom, in float offset)
{
   float res = 0.0;
   for (int i=0; i<9; ++i)
   {
      res += kernel[i] * data[i];
   }
   return saturate(res/denom + offset);
}