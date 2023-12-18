#version 450

layout (input_attachment_index = 0, binding = 0) uniform subpassInput inputPosition;
layout (input_attachment_index = 1, binding = 1) uniform subpassInput inputNormal;
layout (input_attachment_index = 2, binding = 2) uniform subpassInput inputAlbedo;

layout (location = 0) in vec2 inUV;

layout (location = 0) out vec4 outColor;

struct Light {
	vec4 position;
	vec3 color;
	float radius;
};

layout (std140, binding = 3) buffer LightsBuffer 
{
	Light lights[];
};

void main() 
{
	// Read G-Buffer values from previous sub pass
	vec3 fragPos = subpassLoad(inputPosition).rgb;
	vec3 normal = subpassLoad(inputNormal).rgb;
	vec4 albedo = subpassLoad(inputAlbedo);
	
	#define ambient 0.05
	
	// Ambient part
	vec3 fragcolor  = albedo.rgb * ambient;
	
	for(int i = 0; i < lights.length(); ++i)
	{
		vec3 L = lights[i].position.xyz - fragPos;
		float dist = length(L);

		L = normalize(L);
		float atten = lights[i].radius / (pow(dist, 3.0) + 1.0);

		vec3 N = normalize(normal);
		float NdotL = max(0.0, dot(N, L));
		vec3 diff = lights[i].color * albedo.rgb * NdotL * atten;

		fragcolor += diff;
	}    	
   
	outColor = vec4(fragcolor, 1.0);
}