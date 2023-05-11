#version 450

layout (location = 0) in vec3 inColor;
layout (location = 1) in vec3 inNormal;
layout (location = 2) in vec3 inViewVec;
layout (location = 3) in vec3 inLightVec;

layout (location = 0) out vec4 outColor;

void main() 
{
	// Toon shading color attachment output
	float intensity = dot(normalize(inNormal), normalize(inLightVec));
	float shade = 1.0;
	shade = intensity < 0.5 ? 0.75 : shade;
	shade = intensity < 0.35 ? 0.6 : shade;
	shade = intensity < 0.25 ? 0.5 : shade;
	shade = intensity < 0.1 ? 0.25 : shade;

	outColor.rgb = inColor * 3.0 * shade;

	// Depth attachment does not need to be explicitly written
}