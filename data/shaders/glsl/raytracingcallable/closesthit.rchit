#version 460
#extension GL_EXT_ray_tracing : require
#extension GL_EXT_nonuniform_qualifier : enable

layout(location = 0) rayPayloadInEXT vec3 hitValue;
layout(location = 0) callableDataEXT vec3 outColor;

layout(binding = 0, set = 0) uniform accelerationStructureEXT topLevelAS;

void main()
{
	// Execute the callable shader indexed by the current geometry being hit
	// For our sample this means that the first callable shader in the SBT is invoked for the first triangle, the second callable shader for the second triangle, etc.
	executeCallableEXT(gl_GeometryIndexEXT, 0);

	hitValue = outColor;
}
