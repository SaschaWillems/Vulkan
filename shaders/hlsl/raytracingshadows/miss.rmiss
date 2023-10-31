// Copyright 2020 Google LLC

struct Payload
{
	[[vk::location(0)]] float3 hitValue;
	[[vk::location(1)]] bool shadowed;
};

[shader("miss")]
void main(inout Payload payload)
{
    payload.hitValue = float3(0.0, 0.0, 0.2);
}