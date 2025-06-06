/* Copyright (c) 2025, Sascha Willems
 *
 * SPDX-License-Identifier: MIT
 *
 */

// Shader is looseley based on the ray tracing coding session by Inigo Quilez (www.iquilezles.org)

#define EPSILON 0.0001
#define MAXLEN 1000.0
#define SHADOW 0.5
#define RAYBOUNCES 2
#define REFLECTIONS true
#define REFLECTIONSTRENGTH 0.4
#define REFLECTIONFALLOFF 0.5

#define SceneObjectTypeSphere 0
#define SceneObjectTypePlane 1

RWTexture2D<float4> resultImage;

struct Camera
{
	float3 pos;
	float3 lookat;
	float fov;
};

struct UBO
{
	float3 lightPos;
	float aspectRatio;
	float4 fogColor;
	Camera camera;
	float4x4 rotMat;
};
ConstantBuffer<UBO> ubo;

struct SceneObject
{
	float4 objectProperties;
	float3 diffuse;
	float specular;
	int id;
	int objectType;
};
StructuredBuffer<SceneObject> sceneObjects;

void reflectRay(inout float3 rayD, in float3 mormal)
{
	rayD = rayD + 2.0 * -dot(mormal, rayD) * mormal;
}

// Lighting =========================================================

float lightDiffuse(float3 normal, float3 lightDir)
{
	return clamp(dot(normal, lightDir), 0.1, 1.0);
}

float lightSpecular(float3 normal, float3 lightDir, float specularFactor)
{
	float3 viewVec = normalize(ubo.camera.pos);
	float3 halfVec = normalize(lightDir + viewVec);
	return pow(clamp(dot(normal, halfVec), 0.0, 1.0), specularFactor);
}

// Sphere ===========================================================

float sphereIntersect(in float3 rayO, in float3 rayD, in SceneObject sphere)
{
	float3 oc = rayO - sphere.objectProperties.xyz;
	float b = 2.0 * dot(oc, rayD);
	float c = dot(oc, oc) - sphere.objectProperties.w * sphere.objectProperties.w;
	float h = b*b - 4.0*c;
	if (h < 0.0)
	{
		return -1.0;
	}
	float t = (-b - sqrt(h)) / 2.0;

	return t;
}

float3 sphereNormal(in float3 pos, in SceneObject sphere)
{
	return (pos - sphere.objectProperties.xyz) / sphere.objectProperties.w;
}

// Plane ===========================================================

float planeIntersect(float3 rayO, float3 rayD, SceneObject plane)
{
	float d = dot(rayD, plane.objectProperties.xyz);

	if (d == 0.0)
		return 0.0;

	float t = -(plane.objectProperties.w + dot(rayO, plane.objectProperties.xyz)) / d;

	if (t < 0.0)
		return 0.0;

	return t;
}


int intersect(in float3 rayO, in float3 rayD, inout float resT)
{
	int id = -1;
	float t = MAXLEN;

	uint sceneObjectsLength;
	uint sceneObjectsStride;
	sceneObjects.GetDimensions(sceneObjectsLength, sceneObjectsStride);

	for (int i = 0; i < sceneObjectsLength; i++) {
		// Sphere
		if (sceneObjects[i].objectType == SceneObjectTypeSphere) {
			t = sphereIntersect(rayO, rayD, sceneObjects[i]);
		}
		// Plane
		if (sceneObjects[i].objectType == SceneObjectTypePlane) {
			t = planeIntersect(rayO, rayD, sceneObjects[i]);
		}
		if ((t > EPSILON) && (t < resT))
		{
			id = sceneObjects[i].id;
			resT = t;
		}
	}

	return id;
}

float calcShadow(in float3 rayO, in float3 rayD, in int objectId, inout float t)
{
	uint sceneObjectsLength;
	uint sceneObjectsStride;
	sceneObjects.GetDimensions(sceneObjectsLength, sceneObjectsStride);

	for (int i = 0; i < sceneObjectsLength; i++) {
		if (sceneObjects[i].id == objectId)
			continue;

		float tLoc = MAXLEN;

		// Sphere
		if (sceneObjects[i].objectType == SceneObjectTypeSphere)
		{
			tLoc = sphereIntersect(rayO, rayD, sceneObjects[i]);
		}
		// Plane
		if (sceneObjects[i].objectType == SceneObjectTypePlane)
		{
			tLoc = planeIntersect(rayO, rayD, sceneObjects[i]);
		}
		if ((tLoc > EPSILON) && (tLoc < t))
		{
			t = tLoc;
			return SHADOW;
		}
	}
	return 1.0;
}

float3 fog(in float t, in float3 color)
{
	return lerp(color, ubo.fogColor.rgb, clamp(sqrt(t*t)/20.0, 0.0, 1.0));
}

float3 renderScene(inout float3 rayO, inout float3 rayD, inout int id)
{
	float3 color = float3(0, 0, 0);
	float t = MAXLEN;

	// Get intersected object ID
	int objectID = intersect(rayO, rayD, t);

	if (objectID == -1)
	{
		return color;
	}

	float3 pos = rayO + t * rayD;
	float3 lightVec = normalize(ubo.lightPos - pos);
	float3 normal;

	uint sceneObjectsLength;
	uint sceneObjectsStride;
	sceneObjects.GetDimensions(sceneObjectsLength, sceneObjectsStride);

	for (int i = 0; i < sceneObjectsLength; i++) {
		if (objectID == sceneObjects[i].id)
		{
			// Sphere
			if (sceneObjects[i].objectType == SceneObjectTypeSphere) {
				normal = sphereNormal(pos, sceneObjects[i]);
			}
			// Plane
			if (sceneObjects[i].objectType == SceneObjectTypePlane) {
				normal = sceneObjects[i].objectProperties.xyz;
			}
			// Lighting
			float diffuse = lightDiffuse(normal, lightVec);
			float specular = lightSpecular(normal, lightVec, sceneObjects[i].specular);
			color = diffuse * sceneObjects[i].diffuse + specular;
		}
	}

	if (id == -1)
		return color;

	id = objectID;

	// Shadows
	t = length(ubo.lightPos - pos);
	color *= calcShadow(pos, lightVec, id, t);

	// Fog
	color = fog(t, color);

	// Reflect ray for next render pass
	reflectRay(rayD, normal);
	rayO = pos;

	return color;
}

[shader("compute")]
[numthreads(16, 16, 1)]
void computeMain(uint3 GlobalInvocationID : SV_DispatchThreadID)
{
	int2 dim;
	resultImage.GetDimensions(dim.x, dim.y);
	float2 uv = float2(GlobalInvocationID.xy) / dim;

	float3 rayO = ubo.camera.pos;
	float3 rayD = normalize(float3((-1.0 + 2.0 * uv) * float2(ubo.aspectRatio, 1.0), -1.0));

	// Basic color path
	int id = 0;
	float3 finalColor = renderScene(rayO, rayD, id);

	// Reflection
	if (REFLECTIONS)
	{
		float reflectionStrength = REFLECTIONSTRENGTH;
		for (int i = 0; i < RAYBOUNCES; i++)
		{
			float3 reflectionColor = renderScene(rayO, rayD, id);
			finalColor = (1.0 - reflectionStrength) * finalColor + reflectionStrength * lerp(reflectionColor, finalColor, 1.0 - reflectionStrength);
			reflectionStrength *= REFLECTIONFALLOFF;
		}
	}

	resultImage[int2(GlobalInvocationID.xy)] = float4(finalColor, 0.0);
}