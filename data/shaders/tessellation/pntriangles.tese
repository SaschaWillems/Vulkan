#version 450

#extension GL_ARB_separate_shader_objects : enable
#extension GL_ARB_shading_language_420pack : enable

// PN patch data
struct PnPatch
{
 float b210;
 float b120;
 float b021;
 float b012;
 float b102;
 float b201;
 float b111;
 float n110;
 float n011;
 float n101;
};

layout (binding = 1) uniform UBO 
{
	mat4 projection;
	mat4 model;
	float tessAlpha;
} ubo;

layout(triangles, fractional_odd_spacing, ccw) in;

layout(location = 0) in vec3 iNormal[];
layout(location = 3) in vec2 iTexCoord[];
layout(location = 6) in PnPatch iPnPatch[];

layout(location = 0) out vec3 oNormal;
layout(location = 1) out vec2 oTexCoord;

#define uvw gl_TessCoord

void main()
{
 vec3 uvwSquared = uvw * uvw;
 vec3 uvwCubed   = uvwSquared * uvw;

 // extract control points
 vec3 b210 = vec3(iPnPatch[0].b210, iPnPatch[1].b210, iPnPatch[2].b210);
 vec3 b120 = vec3(iPnPatch[0].b120, iPnPatch[1].b120, iPnPatch[2].b120);
 vec3 b021 = vec3(iPnPatch[0].b021, iPnPatch[1].b021, iPnPatch[2].b021);
 vec3 b012 = vec3(iPnPatch[0].b012, iPnPatch[1].b012, iPnPatch[2].b012);
 vec3 b102 = vec3(iPnPatch[0].b102, iPnPatch[1].b102, iPnPatch[2].b102);
 vec3 b201 = vec3(iPnPatch[0].b201, iPnPatch[1].b201, iPnPatch[2].b201);
 vec3 b111 = vec3(iPnPatch[0].b111, iPnPatch[1].b111, iPnPatch[2].b111);

 // extract control normals
 vec3 n110 = normalize(vec3(iPnPatch[0].n110,
                            iPnPatch[1].n110,
                            iPnPatch[2].n110));
 vec3 n011 = normalize(vec3(iPnPatch[0].n011,
                            iPnPatch[1].n011,
                            iPnPatch[2].n011));
 vec3 n101 = normalize(vec3(iPnPatch[0].n101,
                            iPnPatch[1].n101,
                            iPnPatch[2].n101));

 // compute texcoords
 oTexCoord  = gl_TessCoord[2]*iTexCoord[0]
            + gl_TessCoord[0]*iTexCoord[1]
            + gl_TessCoord[1]*iTexCoord[2];

 // normal
 vec3 barNormal = gl_TessCoord[2]*iNormal[0]
                + gl_TessCoord[0]*iNormal[1]
                + gl_TessCoord[1]*iNormal[2];
 vec3 pnNormal  = iNormal[0]*uvwSquared[2]
                + iNormal[1]*uvwSquared[0]
                + iNormal[2]*uvwSquared[1]
                + n110*uvw[2]*uvw[0]
                + n011*uvw[0]*uvw[1]
                + n101*uvw[2]*uvw[1];
 oNormal = ubo.tessAlpha*pnNormal + (1.0-ubo.tessAlpha)*barNormal;

 // compute interpolated pos
 vec3 barPos = gl_TessCoord[2]*gl_in[0].gl_Position.xyz
             + gl_TessCoord[0]*gl_in[1].gl_Position.xyz
             + gl_TessCoord[1]*gl_in[2].gl_Position.xyz;

 // save some computations
 uvwSquared *= 3.0;

 // compute PN position
 vec3 pnPos  = gl_in[0].gl_Position.xyz*uvwCubed[2]
             + gl_in[1].gl_Position.xyz*uvwCubed[0]
             + gl_in[2].gl_Position.xyz*uvwCubed[1]
             + b210*uvwSquared[2]*uvw[0]
             + b120*uvwSquared[0]*uvw[2]
             + b201*uvwSquared[2]*uvw[1]
             + b021*uvwSquared[0]*uvw[1]
             + b102*uvwSquared[1]*uvw[2]
             + b012*uvwSquared[1]*uvw[0]
             + b111*6.0*uvw[0]*uvw[1]*uvw[2];

 // final position and normal
 vec3 finalPos = (1.0-ubo.tessAlpha)*barPos + ubo.tessAlpha*pnPos;
 gl_Position   = ubo.projection * ubo.model * vec4(finalPos,1.0);
}

/*

struct PnPatch
{
	float barycentric[7];
	float normals[3];
};
 
layout (binding = 1) uniform UBO 
{
	mat4 projection;
	mat4 model;
	float tessAlpha;
} ubo; 
 
layout(triangles, equal_spacing, ccw) in;
 
layout(location = 0) in vec3 inNormal[];
layout(location = 1) in PnPatch inPatch[];
 
layout(location = 0) out vec3 outNormal;
 
void main()
{
	vec3 uvwSquared = gl_TessCoord * gl_TessCoord;
	vec3 uvwCubed   = uvwSquared * gl_TessCoord;

	// Get barycentric coordinates
	vec3 barycentric[7];
	for (int i = 0; i < barycentric.length(); ++i) 
	{
		barycentric[i] = vec3(inPatch[0].barycentric[i], inPatch[1].barycentric[i], inPatch[2].barycentric[i]);
	}
	
	// Normal control points
	vec3 normals[3];
	for (int i = 0; i < normals.length(); ++i) 
	{
		normals[i] = normalize(vec3(inPatch[0].normals[i], inPatch[1].normals[i], inPatch[2].normals[i]));
	}

	// Calculate normals
	vec3 barNormal = gl_TessCoord[2] * inNormal[0] + gl_TessCoord[0] * inNormal[1] + gl_TessCoord[1] * inNormal[2];
	vec3 pnNormal  = inNormal[0] * uvwSquared[2]
				   + inNormal[1] * uvwSquared[0]
				   + inNormal[2] * uvwSquared[1]
				   + normals[0] * gl_TessCoord[2] * gl_TessCoord[0]
				   + normals[1] * gl_TessCoord[0] * gl_TessCoord[1]
				   + normals[2] * gl_TessCoord[2] * gl_TessCoord[1];
	outNormal = ubo.tessAlpha * pnNormal + (1.0-ubo.tessAlpha) * barNormal;

	// Interpolate position
	vec3 barPos = gl_TessCoord[2] * gl_in[0].gl_Position.xyz
				+ gl_TessCoord[0] * gl_in[1].gl_Position.xyz
				+ gl_TessCoord[1] * gl_in[2].gl_Position.xyz;

	uvwSquared *= 3.0;

	// PN positions
	vec3 pnPos  = gl_in[0].gl_Position.xyz * uvwCubed[2]
				+ gl_in[1].gl_Position.xyz * uvwCubed[0]
				+ gl_in[2].gl_Position.xyz * uvwCubed[1]
				+ barycentric[0] * uvwSquared[2] * gl_TessCoord[0]
				+ barycentric[1] * uvwSquared[0] * gl_TessCoord[2]
				+ barycentric[5] * uvwSquared[2] * gl_TessCoord[1]
				+ barycentric[2] * uvwSquared[0] * gl_TessCoord[1]
				+ barycentric[4] * uvwSquared[1] * gl_TessCoord[2]
				+ barycentric[3] * uvwSquared[1] * gl_TessCoord[0]
				+ barycentric[6] * 6.0 * gl_TessCoord[0] * gl_TessCoord[1] * gl_TessCoord[2];

	vec3 finalPos = (1.0 - ubo.tessAlpha) * barPos + ubo.tessAlpha * pnPos;
	gl_Position   = ubo.projection * ubo.model * vec4(finalPos, 1.0);
}
*/