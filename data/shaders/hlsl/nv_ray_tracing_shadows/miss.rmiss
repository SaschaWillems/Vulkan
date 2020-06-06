// Copyright 2020 Google LLC

[shader("miss")]
void main([[vk::location(0)]] in float3 hitValue)
{
    hitValue = float3(0.0, 0.0, 0.2);
}