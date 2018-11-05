#version 450

#extension GL_ARB_separate_shader_objects : enable
#extension GL_ARB_shading_language_420pack : enable

layout (location = 0) in vec3 inPos;
layout (location = 1) in vec2 inUV;
layout (location = 2) in vec3 inNormal;

layout (binding = 0) uniform UBO 
{
	mat4 projection;
	mat4 model;
	vec4 viewPos;
	float lodBias;
} ubo;

layout (location = 0) out vec2 outUV;
layout (location = 1) out float outLodBias;
layout (location = 2) out vec3 outNormal;
layout (location = 3) out vec3 outViewVec;
layout (location = 4) out vec3 outLightVec;

out gl_PerVertex 
{
    vec4 gl_Position;   
};


/*
		std::vector<Vertex> vertices =
		{
			{ {  1.0f,  1.0f, 0.0f }, { 1.0f, 1.0f },{ 0.0f, 0.0f, 1.0f } },
			{ { -1.0f,  1.0f, 0.0f }, { 0.0f, 1.0f },{ 0.0f, 0.0f, 1.0f } },
			{ { -1.0f, -1.0f, 0.0f }, { 0.0f, 0.0f },{ 0.0f, 0.0f, 1.0f } },
			{ {  1.0f, -1.0f, 0.0f }, { 1.0f, 0.0f },{ 0.0f, 0.0f, 1.0f } }
		};
*/
//*************************1, works
// below works full screen
//
vec2 positions [4] = vec2[] (
	vec2(1.0,1.0),
	vec2(-1.0,1.0),
	vec2(-1.0,-1.0),
	vec2(1.0,-1.0)
);
//gl_Position = vec4(positions[gl_VertexIndex], 0.0f, 1.0f);
//outFragColor = vec4(diffuse * color.rgb + specular, 1.0);	


//*************************2, works
vec2 positions2 [4] = vec2[] (
	vec2(3.0,3.0),
	vec2(-3.0,3.0),
	vec2(-3.0,-3.0),
	vec2(3.0,-3.0)
);
//gl_Position = vec4(positions2[gl_VertexIndex], 0.0f, 3.0f);
//outFragColor = vec4(diffuse * color.rgb + specular, 1.0);	




//*************************4, works
//gl_Position = vec4(outUV * 2.0f + -1.0f, 0.0f, 1.0f);
//outFragColor = texture(samplerColor, vec2(inUV.s, 1.0 - inUV.t));


//Problems:
/*
Bad:
gl_Position = vec4(outUV * 2.0f + -1.0f, 0.0f, 1.0f);
gl_Position.x = gl_Position.x/3.0f;
gl_Position.y = -gl_Position.y/3.0f;
gl_Position.y = -gl_Position.y;

*/


//*************************3, works
/*
xy: -1, -1;in uv:  0, 0; out uv:  0, 1
xy: 3, -1;in uv:  2, 0; out uv:  2, 1
xy: -1, 3;in uv:  0, 2; out uv:  0, -1
*/

vec2 positions1 [3] = vec2[] (
	vec2(-1.0,-1.0),
	vec2(3.0,-1.0),
	vec2(-1.0,3.0)
	//vec2(3.0,3.0)
);

vec2 uv1 [3] = vec2[] (
	vec2(0,0),
	vec2(2,0),
	vec2(0,2)
	//vec2(3.0,3.0)
);

/*
vec2 uv1 [3] = vec2[] (
	vec2(0,1),
	vec2(2,1),
	vec2(0,-1)
	//vec2(3.0,3.0)
);
*/

//outUV = vec2(uv1[gl_VertexIndex]);
//gl_Position = vec4(positions1[gl_VertexIndex], 0.0f, 1.0f);
//outFragColor = texture(samplerColor, vec2(inUV.s, 1.0 - inUV.t));


void main() 
{
	outUV = inUV;
	outLodBias = ubo.lodBias;

	vec3 worldPos = vec3(ubo.model * vec4(inPos, 1.0));

	//gl_Position = ubo.projection * ubo.model * vec4(inPos.xyz, 1.0);
	//gl_Position = ubo.projectionMatrix * ubo.viewMatrix * ubo.modelMatrix * vec4(outUV * 2.0f + -1.0f, 0.0f, 1.0f);

        //outUV = vec2((gl_VertexIndex << 1) & 2, gl_VertexIndex & 2);
	//gl_Position = vec4(outUV * 2.0f + -1.0f, 0.0f, 1.0f);

	outUV = vec2(uv1[gl_VertexIndex]);
	gl_Position = vec4(positions1[gl_VertexIndex], 0.0f, 1.0f);
	//gl_Position.y = - gl_Position.y;


	//gl_Position.x = gl_Position.x/3.0f;
	//gl_Position.y = gl_Position.y/3.0f;



    	vec4 pos = ubo.model * vec4(inPos, 1.0);
	outNormal = mat3(inverse(transpose(ubo.model))) * inNormal;
	vec3 lightPos = vec3(0.0);
	vec3 lPos = mat3(ubo.model) * lightPos.xyz;
    outLightVec = lPos - pos.xyz;
    outViewVec = ubo.viewPos.xyz - pos.xyz;		
}
