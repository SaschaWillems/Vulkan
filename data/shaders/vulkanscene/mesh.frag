#version 450

layout (binding = 1) uniform sampler2D tex;

layout (location = 0) in vec2 inUV;
layout (location = 1) in vec3 inNormal;
layout (location = 2) in vec3 inColor;
layout (location = 3) in vec3 inEyePos;
layout (location = 4) in vec3 inLightVec;

layout (location = 0) out vec4 outFragColor;

float specpart(vec3 L, vec3 N, vec3 H)
{
	if (dot(N, L) > 0.0)
	{
		return pow(clamp(dot(H, N), 0.0, 1.0), 64.0);
	}
	return 0.0;
}

void main() 
{
	vec3 Eye = normalize(-inEyePos);
	vec3 Reflected = normalize(reflect(-inLightVec, inNormal)); 

	vec3 halfVec = normalize(inLightVec + inEyePos);
	float diff = clamp(dot(inLightVec, inNormal), 0.0, 1.0);
	float spec = specpart(inLightVec, inNormal, halfVec);
	float intensity = 0.1 + diff + spec;
 
	vec4 IAmbient = vec4(0.2, 0.2, 0.2, 1.0);
	vec4 IDiffuse = vec4(0.5, 0.5, 0.5, 0.5) * max(dot(inNormal, inLightVec), 0.0);
	float shininess = 0.75;
	vec4 ISpecular = vec4(0.5, 0.5, 0.5, 1.0) * pow(max(dot(Reflected, Eye), 0.0), 2.0) * shininess; 

	outFragColor = vec4((IAmbient + IDiffuse) * vec4(inColor, 1.0) + ISpecular);
 
	// Some manual saturation
	if (intensity > 0.95)
		outFragColor *= 2.25;
	if (intensity < 0.15)
		outFragColor = vec4(0.1);
}