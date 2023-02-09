// Copyright 2021 Sascha Willems

#version 450

#extension GL_EXT_nonuniform_qualifier : require

layout (set = 0, binding = 1) uniform sampler2D textures[];

layout (location = 0) in vec2 inUV;
layout (location = 1) flat in int inTexIndex;

layout (location = 0) out vec4 outFragColor;

void main() 
{
	outFragColor = texture(textures[nonuniformEXT(inTexIndex)], inUV);
}