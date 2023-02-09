#version 450

layout (binding = 1) uniform samplerCube shadowCubeMap;

layout (location = 0) in vec2 inUV;

layout (location = 0) out vec4 outFragColor;

void main() 
{
	outFragColor.rgb = vec3(0.05);
	
	vec3 samplePos = vec3(0.0f);
	
	// Crude statement to visualize different cube map faces based on UV coordinates
	int x = int(floor(inUV.x / 0.25f));
	int y = int(floor(inUV.y / (1.0 / 3.0))); 
	if (y == 1) {
		vec2 uv = vec2(inUV.x * 4.0f, (inUV.y - 1.0/3.0) * 3.0);
		uv = 2.0 * vec2(uv.x - float(x) * 1.0, uv.y) - 1.0;
		switch (x) {
			case 0:	// NEGATIVE_X
				samplePos = vec3(-1.0f, uv.y, uv.x);
				break;
			case 1: // POSITIVE_Z				
				samplePos = vec3(uv.x, uv.y, 1.0f);
				break;
			case 2: // POSITIVE_X
				samplePos = vec3(1.0, uv.y, -uv.x);
				break;				
			case 3: // NEGATIVE_Z
				samplePos = vec3(-uv.x, uv.y, -1.0f);
				break;
		}
	} else {
		if (x == 1) { 
			vec2 uv = vec2((inUV.x - 0.25) * 4.0, (inUV.y - float(y) / 3.0) * 3.0);
			uv = 2.0 * uv - 1.0;
			switch (y) {
				case 0: // NEGATIVE_Y
					samplePos = vec3(uv.x, -1.0f, uv.y);
					break;
				case 2: // POSITIVE_Y
					samplePos = vec3(uv.x, 1.0f, -uv.y);
					break;
			}
		}
	}

	if ((samplePos.x != 0.0f) && (samplePos.y != 0.0f)) {
		float dist = length(texture(shadowCubeMap, samplePos).xyz) * 0.005;
		outFragColor = vec4(vec3(dist), 1.0);
	}
}