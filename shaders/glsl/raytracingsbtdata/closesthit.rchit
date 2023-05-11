#version 460
#extension GL_EXT_ray_tracing : enable
#extension GL_EXT_nonuniform_qualifier : enable

layout(location = 0) rayPayloadInEXT vec3 hitValue;
hitAttributeEXT vec2 attribs;

layout(shaderRecordEXT, std430) buffer SBT {
  float r;
  float g;
  float b;
};

void main()
{
  // Update the hit value to the hit record SBT data associated with this 
  // geometry ID and ray ID
  hitValue = vec3(r, g, b);
}
