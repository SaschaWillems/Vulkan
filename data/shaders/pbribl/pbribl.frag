// Phyiscally based rendering using IBL
// Based on http://www.trentreed.net/blog/physically-based-shading-and-image-based-lighting/

#version 450

layout (location = 0) in vec3 inWorldPos;
layout (location = 1) in vec3 inNormal;
layout (location = 2) in vec2 inUV;

layout (binding = 0) uniform UBO {
	mat4 projection;
	mat4 model;
	mat4 view;
	vec3 camPos;
} ubo;

layout (binding = 1) uniform UBOShared {
	float exposure;
	float gamma;
} uboShared;

layout(push_constant) uniform PushConsts {
	layout(offset = 12) float roughness;
	layout(offset = 16) float metallic;
	layout(offset = 20) float specular;
	layout(offset = 24) float r;
	layout(offset = 28) float g;
	layout(offset = 32) float b;
} material;

layout (binding = 2) uniform samplerCube radianceMap;
layout (binding = 3) uniform samplerCube irradianceMap;

layout (location = 0) out vec4 outColor;

// From http://filmicgames.com/archives/75
vec3 Uncharted2Tonemap( vec3 x )
{
	float A = 0.15;
	float B = 0.50;
	float C = 0.10;
	float D = 0.20;
	float E = 0.02;
	float F = 0.30;
	return ((x*(A*x+C*B)+D*E)/(x*(A*x+B)+D*F))-E/F;
}

// Environment BRDF approximation from https://www.unrealengine.com/blog/physically-based-shading-on-mobile
vec3 EnvBRDFApprox(vec3 SpecularColor, float Roughness, float NoV) 
{
	vec4 c0 = vec4(-1, -0.0275, -0.572, 0.022);
	vec4 c1 = vec4(1, 0.0425, 1.04, -0.04);
	vec4 r = Roughness * c0 + c1;
	float a004 = min(r.x * r.x, exp2(-9.28 * NoV)) * r.x + r.y;
	vec2 AB = vec2(-1.04, 1.04) * a004 + r.zw;
	return SpecularColor * AB.x + AB.y;
}

void main() 
{
	vec3 N = normalize(inNormal);
	vec3 V = normalize(ubo.camPos - inWorldPos);
	vec3 R = reflect(-V, N);

	vec3 baseColor = vec3(material.r, material.g, material.b);
	
	// Diffuse and specular color from material color and metallic factor
	vec3 diffuseColor = baseColor - baseColor * material.metallic;
	vec3 specularColor = mix(vec3(material.specular), baseColor, material.metallic);

	// Cube map sampling
	ivec2 cubedim = textureSize(radianceMap, 0);
	int numMipLevels = int(log2(max(cubedim.s, cubedim.y)));
	float mipLevel = numMipLevels - 1.0 + log2(material.roughness);
	vec3 radianceSample = pow(textureLod(radianceMap, R, mipLevel).rgb, vec3(2.2f));
	vec3 irradianceSample = pow(texture(irradianceMap, N).rgb, vec3(2.2f));
	
	vec3 reflection = EnvBRDFApprox(specularColor, pow(material.roughness, 1.0f), clamp(dot(N, V), 0.0, 1.0));
	
	// Combine specular IBL and BRDF
	vec3 diffuse = diffuseColor * irradianceSample;
	vec3 specular = radianceSample * reflection;
	vec3 color = diffuse + specular;
	
	// Tone mapping
	color = Uncharted2Tonemap( color * uboShared.exposure );
	color = color * (1.0f / Uncharted2Tonemap(vec3(11.2f)));	
	// Gamma correction
	color = pow(color, vec3(1.0f / uboShared.gamma));
	
	outColor = vec4( color, 1.0 );
}