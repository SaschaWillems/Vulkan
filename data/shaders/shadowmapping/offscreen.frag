#version 450

#extension GL_ARB_separate_shader_objects : enable
#extension GL_ARB_shading_language_420pack : enable

layout(location = 0) out vec4 color;
//layout(location = 0) out float fragmentdepth;

void main() 
{	
//	fragmentdepth = gl_FragCoord.z;
	color = vec4(1.0, 0.0, 0.0, 1.0);
}