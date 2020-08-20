#version 450

layout (location = 0) in vec3 inPos;

layout (set = 0, binding = 0) uniform RenderPassUBO
{
    mat4 projection;
    mat4 view;
} renderPassUBO;

layout (set = 0, binding = 1) uniform ObjectUBO
{
    mat4 model;
    vec4 color;
} objectUBO;

void main()
{
    mat4 PVM = renderPassUBO.projection * renderPassUBO.view * objectUBO.model;
    gl_Position = PVM * vec4(inPos, 1.0);
}
