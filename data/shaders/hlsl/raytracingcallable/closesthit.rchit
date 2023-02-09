// Copyright 2021 Sascha Willems

struct Payload
{
[[vk::location(0)]] float3 hitValue;
};

struct CallData
{
    float3 outColor;
};

[shader("closesthit")]
void main(inout Payload p, in float2 attribs)
{
	// Execute the callable shader indexed by the current geometry being hit
	// For our sample this means that the first callable shader in the SBT is invoked for the first triangle, the second callable shader for the second triangle, etc.
	CallData callData;
	CallShader(GeometryIndex(), callData);
	p.hitValue = callData.outColor;
}
