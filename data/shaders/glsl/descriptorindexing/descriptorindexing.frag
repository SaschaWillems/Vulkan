// Copyright 2021 Sascha Willems

#version 450

#extension GL_EXT_nonuniform_qualifier : require

layout (set = 0, binding = 1) uniform sampler2D textures[];

layout (location = 0) in vec2 inUV;
layout (location = 1) flat in int inTexIndex;

layout (location = 0) out vec4 outFragColor;

vec4 getTextureValue(vec2 uv, int textureId)
{
	return texture(textures[nonuniformEXT(textureId)], uv);
}

void main() 
{
	outFragColor = getTextureValue(inUV, inTexIndex);
}