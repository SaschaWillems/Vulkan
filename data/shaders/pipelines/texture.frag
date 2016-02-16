#version 450

layout (binding = 1) uniform sampler2D colorMap;

layout (location = 1) in vec2 inUV;
layout (location = 2) in vec3 inNormal;

layout (location = 0) out vec4 outFragColor;

void main() 
{
	outFragColor = texture(colorMap, inUV);
	outFragColor.rgb = inNormal;
	
	vec3 N = normalize(inNormal);
	vec3 L = normalize(vec3(2.0, 2.0, 2.0));
	
	vec3 color = texture(colorMap, inUV).rgb; 
	outFragColor.rgb = vec3(clamp(max(dot(N,L), 0.0), 0.15, 1.0)) * color; 	
}
  