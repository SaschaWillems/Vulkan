#extension GL_EXT_ray_query : require
struct RayVertex { vec4 position; vec4 normal; vec4 uv; vec4 color; };
struct RayDraw { mat4 model; uvec4 info; };
struct RayMaterial { vec4 base; vec4 factors; vec4 flags; uvec4 baseImage; uvec4 mrImage; };
layout(set=1,binding=0) uniform accelerationStructureEXT sceneTlas;
layout(std430,set=1,binding=1) readonly buffer Vertices { RayVertex rayVertices[]; };
layout(std430,set=1,binding=2) readonly buffer Indices { uint rayIndices[]; };
layout(std430,set=1,binding=3) readonly buffer Draws { RayDraw rayDraws[]; };
layout(std430,set=1,binding=4) readonly buffer Materials { RayMaterial rayMaterials[]; };
layout(std140,set=1,binding=5) uniform RayFrame {
    mat4 inverseViewProjection; mat4 root; vec4 eye; vec4 lightDirection; vec4 radiance; uvec4 state;
} rtFrame;
layout(std430,set=1,binding=6) readonly buffer Texels { uint rayTexels[]; };
vec3 rtNormalize(vec3 value, vec3 fallback) {
    float length2 = dot(value,value);
    return length2 > 1e-12 ? value * inversesqrt(length2) : fallback;
}
int wrapTexel(int value, int size, uint mode) {
    if (mode == 1u) return clamp(value,0,size-1);
    if (mode == 2u) { int x = ((value % (2*size)) + 2*size) % (2*size); return x < size ? x : 2*size-1-x; }
    return ((value % size) + size) % size;
}
vec4 fetchTexel(uvec4 image, ivec2 xy, bool srgb) {
    xy.x = wrapTexel(xy.x,int(image.y),image.w & 3u);
    xy.y = wrapTexel(xy.y,int(image.z),(image.w >> 2u) & 3u);
    vec4 value = unpackUnorm4x8(rayTexels[image.x + uint(xy.y)*image.y + uint(xy.x)]);
    if (srgb) value.rgb = mix(value.rgb/12.92,pow((value.rgb+0.055)/1.055,vec3(2.4)),greaterThan(value.rgb,vec3(0.04045)));
    return value;
}
vec4 sampleImage(uvec4 image, vec2 uv, bool srgb) {
    // Explicit LOD 0 and manual bilinear filtering avoid descriptor-indexing requirements.
    vec2 p = uv*vec2(image.yz)-0.5;
    if ((image.w & 16u) != 0u) return fetchTexel(image,ivec2(floor(p+0.5)),srgb);
    ivec2 q = ivec2(floor(p)); vec2 f = fract(p);
    return mix(mix(fetchTexel(image,q,srgb),fetchTexel(image,q+ivec2(1,0),srgb),f.x),
               mix(fetchTexel(image,q+ivec2(0,1),srgb),fetchTexel(image,q+ivec2(1,1),srgb),f.x),f.y);
}
struct Surface { vec3 position; vec3 normal; vec3 base; float alpha; float metallic; float roughness; };
Surface surfaceAt(uint instance, uint primitive, vec2 bary, vec3 position, vec3 direction) {
    RayDraw draw = rayDraws[instance]; RayMaterial material = rayMaterials[draw.info.y];
    uint index = draw.info.x + primitive*3u;
    RayVertex a = rayVertices[rayIndices[index]], b = rayVertices[rayIndices[index+1u]], c = rayVertices[rayIndices[index+2u]];
    vec3 w = vec3(1.0-bary.x-bary.y,bary);
    vec4 uv = a.uv*w.x+b.uv*w.y+c.uv*w.z;
    vec4 base = material.base*(a.color*w.x+b.color*w.y+c.color*w.z)*sampleImage(material.baseImage,uv.xy,true);
    vec4 mr = sampleImage(material.mrImage,uv.zw,false);
    mat4 model = rtFrame.root*draw.model;
    vec3 geometric = rtNormalize(transpose(inverse(mat3(model)))*cross(b.position.xyz-a.position.xyz,c.position.xyz-a.position.xyz),vec3(0,1,0));
    vec3 normal = rtNormalize(transpose(inverse(mat3(model)))*(a.normal.xyz*w.x+b.normal.xyz*w.y+c.normal.xyz*w.z),geometric);
    if (dot(normal,direction)>0.0) normal = -normal;
    return Surface(position,normal,base.rgb,base.a,clamp(material.factors.x*mr.b,0.0,1.0),clamp(material.factors.y*mr.g,0.045,1.0));
}
bool traceScene(vec3 origin, vec3 direction, float maximum, out Surface surface) {
    rayQueryEXT query;
    rayQueryInitializeEXT(query,sceneTlas,gl_RayFlagsNoneEXT,0xff,origin,0.0002,direction,maximum);
    while (rayQueryProceedEXT(query)) {
        if (rayQueryGetIntersectionTypeEXT(query,false) != gl_RayQueryCandidateIntersectionTriangleEXT) continue;
        uint instance = rayQueryGetIntersectionInstanceCustomIndexEXT(query,false);
        RayMaterial material = rayMaterials[rayDraws[instance].info.y];
        // TLAS facing flags account for mirrored instance matrices; double-sided materials accept both.
        if (material.flags.x < 0.5 && !rayQueryGetIntersectionFrontFaceEXT(query,false)) continue;
        uint primitive = rayQueryGetIntersectionPrimitiveIndexEXT(query,false);
        vec2 bary = rayQueryGetIntersectionBarycentricsEXT(query,false);
        Surface candidate = surfaceAt(instance,primitive,bary,origin,direction);
        if (material.factors.w > 0.5 && material.factors.w < 1.5 && candidate.alpha < material.factors.z) continue;
        // BLEND uses a coverage cutoff for ray visibility; raster retains true alpha blending.
        if (material.factors.w > 1.5 && candidate.alpha < 0.5) continue;
        rayQueryConfirmIntersectionEXT(query);
    }
    if (rayQueryGetIntersectionTypeEXT(query,true) == gl_RayQueryCommittedIntersectionNoneEXT) return false;
    float distance = rayQueryGetIntersectionTEXT(query,true);
    surface = surfaceAt(rayQueryGetIntersectionInstanceCustomIndexEXT(query,true),
        rayQueryGetIntersectionPrimitiveIndexEXT(query,true),rayQueryGetIntersectionBarycentricsEXT(query,true),
        origin+distance*direction,direction);
    return true;
}
float rayVisibility(vec3 position, vec3 normal, vec3 light) {
    Surface blocker;
    return traceScene(position+normal*0.001,light,100.0,blocker) ? 0.0 : 1.0;
}
vec3 rayDirect(Surface surface, vec3 view) {
    const float PI = 3.14159265359;
    vec3 l = normalize(rtFrame.lightDirection.xyz), n = surface.normal;
    vec3 h = rtNormalize(view+l,n);
    float nl = max(dot(n,l),0.0), nv = max(dot(n,view),0.0001), nh = max(dot(n,h),0.0);
    vec3 f0 = mix(vec3(0.04),surface.base,surface.metallic);
    vec3 f = f0+(1.0-f0)*pow(1.0-max(dot(h,view),0.0),5.0);
    float a2 = pow(surface.roughness,4.0), d = nh*nh*(a2-1.0)+1.0;
    float distribution = a2/max(PI*d*d,0.0001), k = pow(surface.roughness+1.0,2.0)/8.0;
    float geometry = nv/(nv*(1.0-k)+k)*nl/max(nl*(1.0-k)+k,0.0001);
    vec3 specular = distribution*geometry*f/max(4.0*nv*nl,0.0001);
    return ((1.0-f)*(1.0-surface.metallic)*surface.base/PI+specular)*rtFrame.radiance.rgb*nl*
           rayVisibility(surface.position,n,l);
}
vec3 reflectedRadiance(vec3 origin, vec3 direction) {
    Surface hit;
    if (!traceScene(origin,direction,100.0,hit)) return vec3(1.0);
    return rayDirect(hit,-direction)+0.12*hit.base;
}
