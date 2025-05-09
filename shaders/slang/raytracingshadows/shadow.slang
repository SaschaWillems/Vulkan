/* Copyright (c) 2025, Sascha Willems
 *
 * SPDX-License-Identifier: MIT
 *
 */

import payload;

[shader("miss")]
void missMain(inout Payload payload)
{
    payload.shadowed = false;
}

