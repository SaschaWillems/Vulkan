#version 450

#extension GL_ARB_separate_shader_objects : enable
#extension GL_ARB_shading_language_420pack : enable

layout (location = 0) in vec3 inPos;
layout (location = 1) in vec3 inColor;

layout (binding = 0) uniform UBO 
{
	mat4 projectionMatrix;
	mat4 modelMatrix;
	mat4 viewMatrix;
} ubo;

layout (location = 0) out vec3 outColor;

out gl_PerVertex 
{
    vec4 gl_Position;   
};


vec2 positions [4] = vec2[] (
	vec2(-1.0,-1.0),
	vec2(3.0,-1.0),
	vec2(-1.0,3.0),
	vec2(3.0,3.0)
);
//

// Show All Quad, will be a triangle.
//gl_Position = vec4(positions[gl_VertexIndex], 0.0f, 1.0f);
vec2 positions2 [4] = vec2[] (
	vec2(-0.33,-0.33),
	vec2(1.0,-0.33),
	vec2(-0.33,1.0),
	vec2(1.0,1.0)
);

vec2 positions3 [4] = vec2[] (
	vec2(-1.0,-1.0),
	vec2(1.0,-1.0),
	vec2(-1.0,1.0),
	vec2(1.0,1.0)
);

void main() 
{
	outColor = inColor;
        vec2 outUV = vec2((gl_VertexIndex << 1) & 2, gl_VertexIndex & 2);
	//gl_Position = ubo.projectionMatrix * ubo.viewMatrix * ubo.modelMatrix * vec4(outUV * 2.0f + -1.0f, 0.0f, 1.0f);
	//gl_Position = vec4(outUV * 2.0f + -1.0f, 0.0f, 1.0f);
	gl_Position = vec4(positions3[gl_VertexIndex], 0.0f, 1.0f);
}
