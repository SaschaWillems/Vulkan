// Copyright 2020 Google LLC

struct Attribute
{
  float2 attribs;
};

struct Payload
{
[[vk::location(0)]] float3 hitValue;
};

struct SBT {
  float r;
  float g;
  float b;
};
[[vk::shader_record_ext]]
ConstantBuffer<SBT> sbt;

[shader("closesthit")]
void main(inout Payload p, in float2 attribs)
{
  // Update the hit value to the hit record SBT data associated with this 
  // geometry ID and ray ID
  p.hitValue = float3(sbt.r, sbt.g, sbt.g);
}
