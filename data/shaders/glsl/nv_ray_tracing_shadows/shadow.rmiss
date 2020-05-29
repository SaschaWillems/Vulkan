#version 460
#extension GL_NV_ray_tracing : require

layout(location = 2) rayPayloadInNV bool shadowed;

void main()
{
	shadowed = false;
}