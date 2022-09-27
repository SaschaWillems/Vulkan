// Copyright 2020 Google LLC

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

[shader("miss")]
void main(inout Payload p)
{
  // Update the hit value to the hit record SBT data associated with this 
  // miss record
  p.hitValue = float3(sbt.r, sbt.g, sbt.g);
}