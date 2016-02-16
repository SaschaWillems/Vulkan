#version 450

#extension GL_ARB_separate_shader_objects : enable
#extension GL_ARB_shading_language_420pack : enable

layout (binding = 1) uniform sampler2D samplerposition;
layout (binding = 2) uniform sampler2D samplerNormal;
layout (binding = 3) uniform sampler2D samplerAlbedo;

layout (location = 0) in vec2 inUV;

layout (location = 0) out vec4 outFragcolor;

struct Light {
    vec4 position;
    vec4 color;
	float radius;
	float quadraticFalloff;
	float linearFalloff;
	float _pad;
};

layout (binding = 4) uniform UBO 
{
	Light lights[6];
	vec4 viewPos;
} ubo;


void main() 
{
    // Get G-Buffer values
    vec3 fragPos = texture(samplerposition, inUV).rgb;
    vec3 normal = texture(samplerNormal, inUV).rgb;
    vec4 albedo = texture(samplerAlbedo, inUV);
    
	#define lightCount 5
	#define ambient 0.05
	#define specularStrength 0.15
	
	// Ambient part
    vec3 fragcolor  = albedo.rgb * ambient;
	
    vec3 viewVec = normalize(ubo.viewPos.xyz - fragPos);
	
    for(int i = 0; i < lightCount; ++i)
    {
        // Distance from light to fragment position
        float dist = length(ubo.lights[i].position.xyz - fragPos);
		
        if(dist < ubo.lights[i].radius)
        {
			// Get vector from current light source to fragment position
            vec3 lightVec = normalize(ubo.lights[i].position.xyz - fragPos);
            // Diffuse part
            vec3 diffuse = max(dot(normal, lightVec), 0.0) * albedo.rgb * ubo.lights[i].color.rgb;
            // Specular part (specular texture part stored in albedo alpha channel)
            vec3 halfVec = normalize(lightVec + viewVec);  
            vec3 specular = ubo.lights[i].color.rgb * pow(max(dot(normal, halfVec), 0.0), 16.0) * albedo.a * specularStrength;
            // Attenuation with linearFalloff and quadraticFalloff falloff
            float attenuation = 1.0 / (1.0 + ubo.lights[i].linearFalloff * dist + ubo.lights[i].quadraticFalloff * dist * dist);
            fragcolor += (diffuse + specular) * attenuation;
        }
		
    }    	
   
  outFragcolor = vec4(fragcolor, 1.0);	
}