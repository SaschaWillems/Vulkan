#version 450
layout(location=0) in vec3 inPosition;
layout(location=1) in vec3 inNormal;
layout(location=2) in vec2 inTexCoord;
layout(location=3) in vec4 inColor;
layout(location=4) in vec2 inMrTexCoord;
layout(push_constant) uniform Draw { mat4 model; mat4 viewProjection; } draw;
layout(location=0) out vec3 worldPosition;
layout(location=1) out vec3 worldNormal;
layout(location=2) out vec2 texCoord;
layout(location=3) out vec4 color;
layout(location=4) flat out float handedness;
layout(location=5) out vec2 mrTexCoord;
void main() {
    vec4 world = draw.model * vec4(inPosition, 1.0);
    worldPosition = world.xyz;
    worldNormal = transpose(inverse(mat3(draw.model))) * inNormal;
    texCoord = inTexCoord;
    mrTexCoord = inMrTexCoord;
    color = inColor;
    handedness = sign(determinant(mat3(draw.model)));
    gl_Position = draw.viewProjection * world;
}
