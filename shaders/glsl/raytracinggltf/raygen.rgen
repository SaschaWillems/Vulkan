/* Copyright (c) 2023, Sascha Willems
 *
 * SPDX-License-Identifier: MIT
 *
 */

#version 460
#extension GL_EXT_ray_tracing : enable
#extension GL_GOOGLE_include_directive : require

layout(binding = 0, set = 0) uniform accelerationStructureEXT topLevelAS;
layout(binding = 1, set = 0, rgba8) uniform image2D image;
layout(binding = 2, set = 0) uniform CameraProperties 
{
	mat4 viewInverse;
	mat4 projInverse;
	uint frame;
} cam;

layout(location = 0) rayPayloadEXT vec3 hitValue;
layout(location = 3) rayPayloadEXT uint payloadSeed;

#include "random.glsl"

void main() 
{
	uint seed = tea(gl_LaunchIDEXT.y * gl_LaunchSizeEXT.x + gl_LaunchIDEXT.x, cam.frame);
    
	float r1 = rnd(seed);
    float r2 = rnd(seed);

    // Subpixel jitter: send the ray through a different position inside the pixel
    // each time, to provide antialiasing.
    vec2 subpixel_jitter = cam.frame == 0 ? vec2(0.5f, 0.5f) : vec2(r1, r2);
    const vec2 pixelCenter = vec2(gl_LaunchIDEXT.xy) + subpixel_jitter;
    const vec2 inUV        = pixelCenter / vec2(gl_LaunchSizeEXT.xy);
    vec2       d           = inUV * 2.0 - 1.0;

//	const vec2 pixelCenter = vec2(gl_LaunchIDEXT.xy) + vec2(0.5);
//	const vec2 inUV = pixelCenter/vec2(gl_LaunchSizeEXT.xy);
//	vec2 d = inUV * 2.0 - 1.0;

	vec4 origin = cam.viewInverse * vec4(0,0,0,1);
	vec4 target = cam.projInverse * vec4(d.x, d.y, 1, 1) ;
	vec4 direction = cam.viewInverse*vec4(normalize(target.xyz), 0.0) ;

	float tmin = 0.001;
	float tmax = 10000.0;

    hitValue = vec3(0.0);
	vec3 hitValues = vec3(0);

	const int samples = 4;

	// Trace multiple rays for e.g. transparency
	 for(int smpl = 0; smpl < samples; smpl++) {
		payloadSeed = tea(gl_LaunchIDEXT.y * gl_LaunchSizeEXT.x + gl_LaunchIDEXT.x, cam.frame);
		traceRayEXT(topLevelAS, gl_RayFlagsNoneEXT, 0xff, 0, 0, 0, origin.xyz, tmin, direction.xyz, tmax, 0);
		hitValues += hitValue;
	}

//	imageStore(image, ivec2(gl_LaunchIDEXT.xy), vec4(hitValues / float(samples), 0.0));

	vec3 hitVal = hitValues / float(samples);

  if(cam.frame > 0)
  {
    float a         = 1.0f / float(cam.frame + 1);
    vec3  old_color = imageLoad(image, ivec2(gl_LaunchIDEXT.xy)).xyz;
    imageStore(image, ivec2(gl_LaunchIDEXT.xy), vec4(mix(old_color, hitVal, a), 1.f));
  }
  else
  {
    // First frame, replace the value in the buffer
    imageStore(image, ivec2(gl_LaunchIDEXT.xy), vec4(hitVal, 1.f));
  }
}
