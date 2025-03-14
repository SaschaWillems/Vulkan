/* Copyright (c) 2025, Sascha Willems
 *
 * SPDX-License-Identifier: MIT
 *
 */

[shader("fragment")]
float4 main([[vk::location(0)]] float3 Color)
{
  return float4(Color, 1.0);
}