#version 450

layout (binding = 1) uniform sampler2D samplerColor;

layout (location = 0) in vec2 inUV;
layout (location = 1) in vec3 inNormal;
layout (location = 2) in vec3 inViewVec;
layout (location = 3) in vec3 inLightVec;

layout (location = 0) out vec4 outFragColor;

void main () 
{
	vec3 color = texture(samplerColor, inUV).rgb;
	vec3 N = normalize(inNormal);
	vec3 L = normalize(inLightVec);
	vec3 V = normalize(inViewVec);
	vec3 R = reflect(-L, N);
	vec3 diffuse = max(dot(N, L), 0.15) * vec3(1.0);
	vec3 specular = pow(max(dot(R, V), 0.0), 8.0) * vec3(0.2);
	outFragColor = vec4(diffuse * color.rgb + specular, 1.0);	
}
