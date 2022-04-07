#version 460
#extension GL_EXT_ray_tracing : enable
#extension GL_EXT_ray_query : enable

layout (binding = 2, set = 0) uniform accelerationStructureEXT topLevelAS;

layout (location = 0) in vec3 inNormal;
layout (location = 1) in vec3 inColor;
layout (location = 2) in vec3 inViewVec;
layout (location = 3) in vec3 inLightVec;
layout (location = 4) in vec3 inWorldPos;

layout (location = 0) out vec4 outFragColor;

#define ambient 0.1

void main() 
{	
	vec3 N = normalize(inNormal);
	vec3 L = normalize(inLightVec);
	vec3 V = normalize(inViewVec);
	vec3 R = normalize(-reflect(L, N));
	vec3 diffuse = max(dot(N, L), ambient) * inColor;

	outFragColor = vec4(diffuse, 1.0);

	rayQueryEXT rayQuery;
	rayQueryInitializeEXT(rayQuery, topLevelAS, gl_RayFlagsTerminateOnFirstHitEXT | gl_RayFlagsSkipAABBEXT, 0xFF, inWorldPos, 0.01, L, 1000.0);

	// Traverse the acceleration structure and store information about the first intersection (if any)
	rayQueryProceedEXT(rayQuery);

	// If the intersection has hit a triangle, the fragment is shadowed
	if (rayQueryGetIntersectionTypeEXT(rayQuery, true) == gl_RayQueryCommittedIntersectionTriangleEXT ) {
		outFragColor *= 0.1;
	}
}
