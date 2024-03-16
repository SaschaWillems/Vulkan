/* Copyright (c) 2024, Sascha Willems
 *
 * SPDX-License-Identifier: MIT
 *
 */

#version 460
#extension GL_EXT_ray_tracing : enable
#extension GL_EXT_nonuniform_qualifier : enable
// This extension is required for fetching position data in the closes hit shader
#extension GL_EXT_ray_tracing_position_fetch : require

layout(location = 0) rayPayloadInEXT vec3 hitValue;
hitAttributeEXT vec2 attribs;

layout(binding = 2, set = 0) uniform UBO 
{
	mat4 viewInverse;
	mat4 projInverse;
	vec4 lightPos;
} ubo;

void main()
{
	// We need the barycentric coordinates to calculate data for the current position
	const vec3 barycentricCoords = vec3(1.0 - attribs.x - attribs.y, attribs.x, attribs.y);

	// With VK_KHR_ray_tracing_position_fetch we can access the vertices for the hit triangle in the shader
	vec3 vertexPos0 = gl_HitTriangleVertexPositionsEXT[0];
	vec3 vertexPos1 = gl_HitTriangleVertexPositionsEXT[1];
	vec3 vertexPos2 = gl_HitTriangleVertexPositionsEXT[2];
	vec3 currentPos = vertexPos0 * barycentricCoords.x + vertexPos1 * barycentricCoords.y + vertexPos2 * barycentricCoords.z;

	// Calcualte the normal from above values
	vec3 normal = normalize(cross(vertexPos1 - vertexPos0, vertexPos2 - vertexPos0));
	normal = normalize(vec3(normal * gl_WorldToObjectEXT));

	// Visualize the normal
	hitValue = normal;

	// Basic lighting
	vec3 lightDir = normalize(ubo.lightPos.xyz - currentPos);
	float diffuse = max(dot(normal, lightDir), 0.0);

	hitValue = vec3(0.1 + diffuse);
}