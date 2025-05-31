/* Copyright (c) 2025, Sascha Willems
 *
 * SPDX-License-Identifier: MIT
 *
 */

import types;

[shader("vertex")]
VSOutput vertexMain(uint VertexIndex : SV_VertexID)
{
	VSOutput output;
	output.UV = float2((VertexIndex << 1) & 2, VertexIndex & 2);
	output.Pos = float4(output.UV * 2.0f - 1.0f, 0.0f, 1.0f);
	return output;
}
