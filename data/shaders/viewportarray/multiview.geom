#version 450

#extension GL_ARB_viewport_array : enable

layout (triangles, invocations = 2) in;
layout (triangle_strip, max_vertices = 3) out;

layout (binding = 0) uniform UBO 
{
	mat4 projection[2];
	mat4 modelview[2];
	vec4 lightPos;
} ubo;

layout (location = 0) in vec3 inNormal[];
layout (location = 1) in vec3 inColor[];

layout (location = 0) out vec3 outNormal;
layout (location = 1) out vec3 outColor;
layout (location = 2) out vec3 outViewVec;
layout (location = 3) out vec3 outLightVec;

void main(void)
{	
	for(int i = 0; i < gl_in.length(); i++)
	{
		outNormal = mat3(ubo.modelview[gl_InvocationID]) * inNormal[i];
		outColor = inColor[i];

		vec4 pos = gl_in[i].gl_Position;
		vec4 worldPos = (ubo.modelview[gl_InvocationID] * pos);
		
		vec3 lPos = vec3(ubo.modelview[gl_InvocationID]  * ubo.lightPos);
		outLightVec = lPos - worldPos.xyz;
		outViewVec = -worldPos.xyz;	
	
		gl_Position = ubo.projection[gl_InvocationID] * worldPos;

		// Set the viewport index that the vertex will be emitted to
		gl_ViewportIndex = gl_InvocationID;
      gl_PrimitiveID = gl_PrimitiveIDIn;
		EmitVertex();
	}
	EndPrimitive();
}