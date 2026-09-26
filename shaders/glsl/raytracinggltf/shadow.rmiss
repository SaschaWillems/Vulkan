#version 460
#extension GL_EXT_ray_tracing : require
#extension GL_GOOGLE_include_directive : require

#include "payload.glsl"

layout(location = 0) rayPayloadInEXT RayPayload payloadIn;

void main()
{
	payloadIn.shadowed = false;
}