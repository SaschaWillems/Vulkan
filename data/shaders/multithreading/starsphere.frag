#version 450

#extension GL_ARB_separate_shader_objects : enable
#extension GL_ARB_shading_language_420pack : enable

layout (location = 0) in vec3 inUVW;

layout (location = 0) out vec4 outFragColor;

#define HASHSCALE3 vec3(443.897, 441.423, 437.195)
#define STARFREQUENCY 0.01

// Hash function by Dave Hoskins (https://www.shadertoy.com/view/4djSRW)
float hash33(vec3 p3)
{
	p3 = fract(p3 * HASHSCALE3);
    p3 += dot(p3, p3.yxz+vec3(19.19));
    return fract((p3.x + p3.y)*p3.z + (p3.x+p3.z)*p3.y + (p3.y+p3.z)*p3.x);
}

vec3 starField(vec3 pos)
{
	vec3 color = vec3(0.0);
    float threshhold = (1.0 - STARFREQUENCY);
    float rnd = hash33(pos);
    if (rnd >= threshhold)
    {
        float starCol = pow((rnd - threshhold) / (1.0 - threshhold), 16.0);
		color += vec3(starCol);
    }	
	return color;
}

void main() 
{
	// Fake atmosphere at the bottom
	vec3 atmosphere = clamp(vec3(0.1, 0.15, 0.4) * (inUVW.t - 5.0), 0.0, 1.0);

	vec3 color = starField(inUVW) + atmosphere;
	
	outFragColor = vec4(color, 1.0);
}