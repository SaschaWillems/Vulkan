#version 450

layout (binding = 1) uniform sampler2D sColorMap;
layout (binding = 2) uniform sampler2D sNormalHeightMap;

layout (binding = 3) uniform UBO 
{
	float heightScale;
	float parallaxBias;
	float numLayers;
	int mappingMode;
} ubo;

layout (location = 0) in vec2 inUV;
layout (location = 1) in vec3 inTangentLightPos;
layout (location = 2) in vec3 inTangentViewPos;
layout (location = 3) in vec3 inTangentFragPos;

layout (location = 0) out vec4 outColor;

vec2 parallax_uv(vec2 uv, vec3 view_dir, int type)
{
	if (type == 2) {
		// Parallax mapping
		float depth = 1.0 - textureLod(sNormalHeightMap, uv, 0.0).a;
		vec2 p = view_dir.xy * (depth * (ubo.heightScale * 0.5) + ubo.parallaxBias) / view_dir.z;
		return uv - p;  
	} else {
		float layer_depth = 1.0 / ubo.numLayers;
		float cur_layer_depth = 0.0;
		vec2 delta_uv = view_dir.xy * ubo.heightScale / (view_dir.z * ubo.numLayers);
		vec2 cur_uv = uv;

		float depth_from_tex = 1.0 - textureLod(sNormalHeightMap, cur_uv, 0.0).a;

		for (int i = 0; i < 32; i++) {
			cur_layer_depth += layer_depth;
			cur_uv -= delta_uv;
			depth_from_tex = 1.0 - textureLod(sNormalHeightMap, cur_uv, 0.0).a;
			if (depth_from_tex < cur_layer_depth) {
				break;
			}
		}

		if (type == 3) {
			// Steep parallax mapping
			return cur_uv;
		} else {
			// Parallax occlusion mapping
			vec2 prev_uv = cur_uv + delta_uv;
			float next = depth_from_tex - cur_layer_depth;
			float prev = 1.0 - textureLod(sNormalHeightMap, prev_uv, 0.0).a - cur_layer_depth + layer_depth;
			float weight = next / (next - prev);
			return mix(cur_uv, prev_uv, weight);
		}
	}
}

vec2 parallaxMapping(vec2 uv, vec3 viewDir) 
{
	float height = 1.0 - textureLod(sNormalHeightMap, uv, 0.0).a;
	vec2 p = viewDir.xy * (height * (ubo.heightScale * 0.5) + ubo.parallaxBias) / viewDir.z;
	return uv - p;  
}

vec2 steepParallaxMapping(vec2 uv, vec3 viewDir) 
{
	float layerDepth = 1.0 / ubo.numLayers;
	float currLayerDepth = 0.0;
	vec2 deltaUV = viewDir.xy * ubo.heightScale / (viewDir.z * ubo.numLayers);
	vec2 currUV = uv;
	float height = 1.0 - textureLod(sNormalHeightMap, currUV, 0.0).a;
	for (int i = 0; i < ubo.numLayers; i++) {
		currLayerDepth += layerDepth;
		currUV -= deltaUV;
		height = 1.0 - textureLod(sNormalHeightMap, currUV, 0.0).a;
		if (height < currLayerDepth) {
			break;
		}
	}
	return currUV;
}

vec2 parallaxOcclusionMapping(vec2 uv, vec3 viewDir) 
{
	float layerDepth = 1.0 / ubo.numLayers;
	float currLayerDepth = 0.0;
	vec2 deltaUV = viewDir.xy * ubo.heightScale / (viewDir.z * ubo.numLayers);
	vec2 currUV = uv;
	float height = 1.0 - textureLod(sNormalHeightMap, currUV, 0.0).a;
	for (int i = 0; i < ubo.numLayers; i++) {
		currLayerDepth += layerDepth;
		currUV -= deltaUV;
		height = 1.0 - textureLod(sNormalHeightMap, currUV, 0.0).a;
		if (height < currLayerDepth) {
			break;
		}
	}
	vec2 prevUV = currUV + deltaUV;
	float nextDepth = height - currLayerDepth;
	float prevDepth = 1.0 - textureLod(sNormalHeightMap, prevUV, 0.0).a - currLayerDepth + layerDepth;
	return mix(currUV, prevUV, nextDepth / (nextDepth - prevDepth));
}

void main(void) 
{
	vec3 V = normalize(inTangentViewPos - inTangentFragPos);
	vec2 uv = inUV;

	if (ubo.mappingMode == 0) {
		// Color only
		outColor = texture(sColorMap, inUV);
	} else {
		switch(ubo.mappingMode) {
			case 2:
				uv = parallaxMapping(inUV, V);
				break;
			case 3:
				uv = steepParallaxMapping(inUV, V);
				break;
			case 4:
				uv = parallaxOcclusionMapping(inUV, V);
				break;
		}

		// Discard fragments at texture border
		if (uv.x < 0.0 || uv.x > 1.0 || uv.y < 0.0 || uv.y > 1.0) {
			discard;
		}

		vec3 N = normalize(textureLod(sNormalHeightMap, uv, 0.0).rgb * 2.0 - 1.0);
		vec3 L = normalize(inTangentLightPos - inTangentFragPos);
		vec3 R = reflect(-L, N);
		vec3 H = normalize(L + V);  
   
		vec3 color = texture(sColorMap, uv).rgb;
		vec3 ambient = 0.2 * color;
		vec3 diffuse = max(dot(L, N), 0.0) * color;
		vec3 specular = vec3(0.15) * pow(max(dot(N, H), 0.0), 32.0);

		outColor = vec4(ambient + diffuse + specular, 1.0f);
	}	
}
