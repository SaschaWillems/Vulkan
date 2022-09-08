// Copyright 2020 Google LLC

struct Payload
{
[[vk::location(0)]] float3 hitValue;
};

[shader("miss")]
void main(inout Payload p)
{
    // for now, we do nothing in the miss program
}