#version 460
#ifdef OFFSCREEN_RAY_QUERY
#extension GL_GOOGLE_include_directive : require
#include "ray_scene.glsl"
#endif
layout(location=0) in vec3 worldPosition;
layout(location=1) in vec3 worldNormal;
layout(location=2) in vec2 texCoord;
layout(location=3) in vec4 color;
layout(location=4) flat in float handedness;
layout(location=5) in vec2 mrTexCoord;
layout(set=0,binding=0) uniform Lighting { vec4 eye; vec4 lightDirection; vec4 radiance; } lighting;
layout(set=0,binding=1) uniform Material { vec4 baseColor; vec4 factors; vec4 flags; } material;
layout(set=0,binding=2) uniform sampler2D baseColorTexture;
layout(set=0,binding=3) uniform sampler2D metallicRoughnessTexture;
layout(location=0) out vec4 outColor;
const float PI = 3.14159265359;
float geometry(float nd, float k) { return nd / max(nd * (1.0 - k) + k, 0.0001); }
vec3 safeNormalize(vec3 value, vec3 fallback) {
    float squared = dot(value,value);
    return squared > 0.00000001 ? value * inversesqrt(squared) : fallback;
}
void main() {
    bool front = gl_FrontFacing == (handedness > 0.0);
    if (!front && material.flags.x < 0.5) discard;
    // Base-color textures are sRGB images; Vulkan converts samples to linear light.
    vec4 base = material.baseColor * color * texture(baseColorTexture, texCoord);
    if (material.factors.w > 0.5 && material.factors.w < 1.5 && base.a < material.factors.z) discard;
    vec4 mr = texture(metallicRoughnessTexture, mrTexCoord);
    float metallic = clamp(material.factors.x * mr.b, 0.0, 1.0);
    float roughness = clamp(material.factors.y * mr.g, 0.045, 1.0);
    vec3 geometric = safeNormalize(cross(dFdx(worldPosition),dFdy(worldPosition)),vec3(0,1,0));
    vec3 n = safeNormalize(worldNormal,geometric);
    if (!front) n = -n;
    vec3 v = safeNormalize(lighting.eye.xyz - worldPosition,n);
    vec3 l = normalize(lighting.lightDirection.xyz);
    vec3 h = safeNormalize(v + l,n);
    float nv = max(dot(n,v),0.0001), nl = max(dot(n,l),0.0), nh = max(dot(n,h),0.0);
    vec3 f0 = mix(vec3(0.04), base.rgb, metallic);
    vec3 fresnel = f0 + (1.0-f0) * pow(1.0-max(dot(h,v),0.0),5.0);
    float a2 = pow(roughness,4.0);
    float denominator = nh*nh*(a2-1.0)+1.0;
    float distribution = a2 / max(PI*denominator*denominator,0.0001);
    float k = (roughness+1.0)*(roughness+1.0)/8.0;
    vec3 specular = distribution * geometry(nv,k) * geometry(nl,k) * fresnel / max(4.0*nv*nl,0.0001);
    vec3 diffuse = (1.0-fresnel) * (1.0-metallic) * base.rgb / PI;
    vec3 linear = (diffuse+specular)*lighting.radiance.rgb*nl + 0.12*base.rgb;
#ifdef OFFSCREEN_RAY_QUERY
    if (rtFrame.state.x == 1u) {
        linear = (diffuse+specular)*lighting.radiance.rgb*nl*rayVisibility(worldPosition,n,l)+0.12*base.rgb;
        if (metallic > 0.01) {
            vec3 reflection = reflectedRadiance(worldPosition+n*0.001,reflect(-v,n));
            linear += metallic*fresnel*reflection*(1.0-0.5*roughness);
        }
    }
#endif
    vec3 mapped = linear/(linear+vec3(1.0));
    outColor = vec4(pow(mapped,vec3(1.0/2.2)), material.factors.w > 1.5 ? base.a : 1.0);
}
