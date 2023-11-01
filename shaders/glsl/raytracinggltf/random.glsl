/* Copyright (c) 2023, Sascha Willems
 *
 * SPDX-License-Identifier: MIT
 *
 */

// Tiny Encryption Algorithm
// By Fahad Zafar, Marc Olano and Aaron Curtis, see https://www.highperformancegraphics.org/previous/www_2010/media/GPUAlgorithms/HPG2010_GPUAlgorithms_Zafar.pdf
uint tea(uint val0, uint val1)
{
    uint sum = 0;
    uint v0 = val0;
    uint v1 = val1;
    for (uint n = 0; n < 16; n++)
    {
        sum += 0x9E3779B9;
        v0 += ((v1 << 4) + 0xA341316C) ^ (v1 + sum) ^ ((v1 >> 5) + 0xC8013EA4);
        v1 += ((v0 << 4) + 0xAD90777D) ^ (v0 + sum) ^ ((v0 >> 5) + 0x7E95761E);
    }
    return v0;
}

// Linear congruential generator based on the previous RNG state
// See https://en.wikipedia.org/wiki/Linear_congruential_generator
uint lcg(inout uint previous)
{
    const uint multiplier = 1664525u;
    const uint increment = 1013904223u;
    previous   = (multiplier * previous + increment);
    return previous & 0x00FFFFFF;
}

// Generate a random float in [0, 1) given the previous RNG state
float rnd(inout uint previous)
{
    return (float(lcg(previous)) / float(0x01000000));
}