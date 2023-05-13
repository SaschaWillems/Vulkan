#version 460
#extension GL_EXT_ray_tracing : require
#extension GL_EXT_nonuniform_qualifier : enable

layout(location = 0) rayPayloadInEXT vec3 hitValue;
layout(location = 2) rayPayloadEXT bool shadowed;
hitAttributeEXT vec2 attribs;

layout(binding = 0, set = 0) uniform accelerationStructureEXT topLevelAS;
layout(binding = 2, set = 0) uniform UBO 
{
	mat4 viewInverse;
	mat4 projInverse;
	vec4 lightPos;
} ubo;

struct Sphere {
	vec3 center;
	float radius;
	vec4 color;
};
layout(binding = 3, set = 0) buffer Spheres { Sphere s[]; } spheres;

void main()
{
	Sphere sphere = spheres.s[gl_PrimitiveID];

	vec3 worldPos = gl_WorldRayOriginEXT + gl_WorldRayDirectionEXT * gl_HitTEXT;
	vec3 worldNrm = normalize(worldPos - sphere.center);

	// Basic lighting
	vec3 lightVector = normalize(ubo.lightPos.xyz);
	float dot_product = max(dot(lightVector, worldNrm), 0.2);
	hitValue = sphere.color.rgb * dot_product;
}