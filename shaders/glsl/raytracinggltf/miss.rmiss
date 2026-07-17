/* Copyright (c) 2023, Sascha Willems
 *
 * SPDX-License-Identifier: MIT
 *
 */

#version 460
#extension GL_EXT_ray_tracing : enable
#extension GL_GOOGLE_include_directive : require

#include "payload.glsl"

layout(location = 0) rayPayloadInEXT RayPayload payloadIn;

void main()
{
    payloadIn.hitValue = vec3(1.0);
}