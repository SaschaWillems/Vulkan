/* Copyright (c) 2025, Sascha Willems
 *
 * SPDX-License-Identifier: MIT
 *
 */

RWStructuredBuffer<uint> values;

[[SpecializationConstant]] const uint BUFFER_ELEMENTS = 32;

uint fibonacci(uint n) {
	if(n <= 1){
		return n;
	}
	uint curr = 1;
	uint prev = 1;
	for(uint i = 2; i < n; ++i) {
		uint temp = curr;
		curr += prev;
		prev = temp;
	}
	return curr;
}

[numthreads(1, 1, 1)]
[shader("compute")]
void computeMain(uint3 GlobalInvocationID : SV_DispatchThreadID)
{
	uint index = GlobalInvocationID.x;
	if (index >= BUFFER_ELEMENTS)
		return;
	values[index] = fibonacci(values[index]);
}

