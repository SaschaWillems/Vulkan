/* Copyright (c) 2021, Sascha Willems
 *
 * SPDX-License-Identifier: MIT
 *
 */

#version 450
 
layout (location = 0) in VertexInput {
  vec4 color;
} vertexInput;

layout(location = 0) out vec4 FragColor;
 

void main()
{
  FragColor = vertexInput.color;
}