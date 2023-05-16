/* Copyright (c) 2023, Sascha Willems
 *
 * SPDX-License-Identifier: MIT
 *
 */

#version 460

#extension GL_EXT_ray_tracing : require
#extension GL_GOOGLE_include_directive : require
#extension GL_EXT_nonuniform_qualifier : require
#extension GL_EXT_buffer_reference2 : require
#extension GL_EXT_scalar_block_layout : require
#extension GL_EXT_shader_explicit_arithmetic_types_int64 : require

layout(location = 0) rayPayloadInEXT vec3 hitValue;
layout(location = 2) rayPayloadEXT bool shadowed;
hitAttributeEXT vec2 attribs;

layout(binding = 0, set = 0) uniform accelerationStructureEXT topLevelAS;
layout(binding = 3, set = 0) uniform sampler2D image;

struct GeometryNode {
	uint64_t vertexBufferDeviceAddress;
	uint64_t indexBufferDeviceAddress;
};
layout(binding = 4, set = 0) buffer GeometryNodes { GeometryNode nodes[]; } geometryNodes;

#include "bufferreferences.glsl"
#include "geometrytypes.glsl"

void main()
{
	vec3 color = vec3(1.0);
	if (gl_GeometryIndexEXT == 0) {
		color = vec3(1.0, 0.0, 0.0);
	}
	if (gl_GeometryIndexEXT == 1) {
		color = vec3(0.0, 1.0, 0.0);
	}
	if (gl_GeometryIndexEXT == 2) {
		color = vec3(0.0, 0.0, 1.0);
	}
	if (gl_GeometryIndexEXT == 3) {
		color = vec3(1.0, 1.0, 0.0);
	}
	if (gl_GeometryIndexEXT == 4) {
		color = vec3(0.0, 1.0, 1.0);
	}
	if (gl_GeometryIndexEXT == 5) {
		color = vec3(1.0, 0.0, 1.0);
	}

	Triangle tri = unpackTriangle(gl_PrimitiveID, 112);
	hitValue = vec3(tri.normal);

//	hitValue = color;
	// Fetch the color for this ray hit from the texture at the current uv coordinates
//	vec4 color = texture(image, tri.uv);
	//hitValue = color.rgb;
	// Shadow casting
	float tmin = 0.001;
	float tmax = 10000.0;
	float epsilon = 0.01;
	vec3 origin = gl_WorldRayOriginEXT + gl_WorldRayDirectionEXT * gl_HitTEXT;// + tri.normal * epsilon;
	shadowed = true;  
	vec3 lightVector = vec3(-5.0, -2.5, -5.0);
	// Trace shadow ray and offset indices to match shadow hit/miss shader group indices
	traceRayEXT(topLevelAS, gl_RayFlagsTerminateOnFirstHitEXT | gl_RayFlagsOpaqueEXT | gl_RayFlagsSkipClosestHitShaderEXT, 0xFF, 0, 0, 1, origin, tmin, lightVector, tmax, 2);
	if (shadowed) {
//		hitValue *= 0.3;
	}
}
