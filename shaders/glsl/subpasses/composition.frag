#version 450

layout (input_attachment_index = 0, binding = 0) uniform subpassInput samplerposition;
layout (input_attachment_index = 1, binding = 1) uniform subpassInput samplerNormal;
layout (input_attachment_index = 2, binding = 2) uniform subpassInput samplerAlbedo;

layout (location = 0) in vec2 inUV;

layout (location = 0) out vec4 outColor;

layout (constant_id = 0) const int NUM_LIGHTS = 64;

struct Light {
	vec4 position;
	vec3 color;
	float radius;
};

layout (binding = 3) uniform UBO 
{
	Light lights[NUM_LIGHTS];
} ubo;


void main() 
{
	// Read G-Buffer values from previous sub pass
	vec3 fragPos = subpassLoad(samplerposition).rgb;
	vec3 normal = subpassLoad(samplerNormal).rgb;
	vec4 albedo = subpassLoad(samplerAlbedo);
	
	#define ambient 0.05
	
	// Ambient part
	vec3 fragcolor  = albedo.rgb * ambient;
	
	for(int i = 0; i < NUM_LIGHTS; ++i)
	{
		vec3 L = ubo.lights[i].position.xyz - fragPos;
		float dist = length(L);

		L = normalize(L);
		float atten = ubo.lights[i].radius / (pow(dist, 3.0) + 1.0);

		vec3 N = normalize(normal);
		float NdotL = max(0.0, dot(N, L));
		vec3 diff = ubo.lights[i].color * albedo.rgb * NdotL * atten;

		fragcolor += diff;
	}    	
   
	outColor = vec4(fragcolor, 1.0);
}