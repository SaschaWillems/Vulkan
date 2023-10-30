/* Copyright (c) 2023, Sascha Willems
 *
 * SPDX-License-Identifier: MIT
 *
 */

layout(push_constant) uniform BufferReferences {
	uint64_t vertices;
	uint64_t indices;
	uint64_t bufferAddress;
} bufferReferences;

layout(buffer_reference, scalar) buffer Vertices {vec4 v[]; };
layout(buffer_reference, scalar) buffer Indices {uint i[]; };
layout(buffer_reference, scalar) buffer Data {vec4 f[]; };