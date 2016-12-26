#version 450

#extension GL_ARB_separate_shader_objects : enable
#extension GL_ARB_shading_language_420pack : enable

layout (binding = 1) uniform sampler2D samplerColormap;
layout (binding = 2) uniform sampler2D samplerDiscard;

layout (location = 0) in vec3 inNormal;
layout (location = 1) in vec3 inColor;
layout (location = 2) in vec2 inUV;
layout (location = 3) in vec3 inViewVec;
layout (location = 4) in vec3 inLightVec;

layout (location = 0) out vec4 outFragColor;

// We use this constant to control the flow of the shader depending on the 
// lighting model selected at pipeline creation time
layout (constant_id = 0) const int LIGHTING_MODEL = 0;

void main() 
{
	switch (LIGHTING_MODEL) {
		case 0: // Phong			
		{
			vec3 ambient = inColor * vec3(0.25);
			vec3 N = normalize(inNormal);
			vec3 L = normalize(inLightVec);
			vec3 V = normalize(inViewVec);
			vec3 R = reflect(-L, N);
			vec3 diffuse = max(dot(N, L), 0.0) * inColor;
			vec3 specular = pow(max(dot(R, V), 0.0), 32.0) * vec3(0.75);
			outFragColor = vec4(ambient + diffuse * 1.75 + specular, 1.0);		
			break;
		}
		case 1: // Toon
		{

			vec3 N = normalize(inNormal);
			vec3 L = normalize(inLightVec);
			float intensity = dot(N,L);
			vec3 color;
			if (intensity > 0.98)
				color = inColor * 1.5;
			else if  (intensity > 0.9)
				color = inColor * 1.0;
			else if (intensity > 0.5)
				color = inColor * 0.6;
			else if (intensity > 0.25)
				color = inColor * 0.4;
			else
				color = inColor * 0.2;
			outFragColor.rgb = color;
			break;
		}
		case 2: // Textured
		{
			vec4 color = texture(samplerColormap, inUV).rrra;
			vec3 ambient = color.rgb * vec3(0.25) * inColor;
			vec3 N = normalize(inNormal);
			vec3 L = normalize(inLightVec);
			vec3 V = normalize(inViewVec);
			vec3 R = reflect(-L, N);
			vec3 diffuse = max(dot(N, L), 0.0) * color.rgb;
			float specular = pow(max(dot(R, V), 0.0), 32.0) * color.a;
			outFragColor = vec4(ambient + diffuse + vec3(specular), 1.0);		
			break;
		}
	}
}