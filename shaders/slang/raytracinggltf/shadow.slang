/* Copyright (c) 2025, Sascha Willems
 *
 * SPDX-License-Identifier: MIT
 *
 */

struct Payload
{
    float3 hitValue;
    uint payloadSeed;
    bool shadowed;
};

[shader("miss")]
void missMain(inout Payload payload)
{
    payload.shadowed = false;
}