// Copyright 2020 Google LLC

[[vk::input_attachment_index(0)]][[vk::binding(0)]] SubpassInput inputColor;
[[vk::input_attachment_index(1)]][[vk::binding(1)]] SubpassInput inputDepth;

struct UBO  {
	float2 brightnessContrast;
	float2 range;
	int attachmentIndex;
};

cbuffer ubo : register(b2) { UBO ubo; }

float3 brightnessContrast(float3 color, float brightness, float contrast) {
	return (color - 0.5) * contrast + 0.5 + brightness;
}

float4 main() : SV_TARGET
{
	// Apply brightness and contrast filer to color input
	if (ubo.attachmentIndex == 0) {
		// Read color from previous color input attachment
		float3 color = inputColor.SubpassLoad().rgb;
		return float4(brightnessContrast(color, ubo.brightnessContrast[0], ubo.brightnessContrast[1]), 1);
	}

	// Visualize depth input range
	if (ubo.attachmentIndex == 1) {
		// Read depth from previous depth input attachment
		float depth = inputDepth.SubpassLoad().r;
		return float4((depth - ubo.range[0]) * 1.0 / (ubo.range[1] - ubo.range[0]).xxx, 1);
	}

	return 0.xxxx;
}