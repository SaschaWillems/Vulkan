// Copyright 2020 Google LLC

struct Attribute
{
  float2 attribs;
};

struct Payload
{
[[vk::location(0)]] float3 hitValue;
};

[shader("closesthit")]
void main(inout Payload p, in float3 attribs)
{
  const float3 barycentricCoords = float3(1.0f - attribs.x - attribs.y, attribs.x, attribs.y);
  p.hitValue = barycentricCoords;
}
