/* Copyright (c) 2025, Sascha Willems
 *
 * SPDX-License-Identifier: MIT
 *
 */

struct VSOutput
{
    float4 Pos : SV_POSITION;
};

[[vk::input_attachment_index(0)]] [[vk::binding(0)]] SubpassInput inputColor;
[[vk::input_attachment_index(1)]] [[vk::binding(1)]] SubpassInput inputDepth;

struct UBO  {
	float2 brightnessContrast;
	float2 range;
	int attachmentIndex;
};
ConstantBuffer<UBO> ubo;

float3 brightnessContrast(float3 color, float brightness, float contrast) {
	return (color - 0.5) * contrast + 0.5 + brightness;
}

[shader("vertex")]
VSOutput vertexMain(uint VertexIndex: SV_VertexID)
{
    VSOutput output;
    output.Pos = float4(float2((VertexIndex << 1) & 2, VertexIndex & 2) * 2.0f - 1.0f, 0.0f, 1.0f);
    return output;
}

[shader("fragment")]
float4 fragmentMain()
{
    // Apply brightness and contrast filer to color input
    if (ubo.attachmentIndex == 0) {
        // Read color from previous color input attachment
        float3 color = inputColor.SubpassLoad().rgb;
        return float4(brightnessContrast(color, ubo.brightnessContrast[0], ubo.brightnessContrast[1]), 1.0);
    }

    // Visualize depth input range
    if (ubo.attachmentIndex == 1) {
        // Read depth from previous depth input attachment
        float depth = inputDepth.SubpassLoad().r;
        return float4((depth - ubo.range[0]) * 1.0 / (ubo.range[1] - ubo.range[0]).xxx, 1.0);
    }

    return float4(1.0);
}