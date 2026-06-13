#version 460 core

out vec4 FragColor;

uniform vec3 viewPos;
uniform mat4 uInvView;
uniform mat4 uInvProj;
uniform vec2 uResolution;

uniform int uMaxBounces; 
uniform vec3 uAmbientColor;
uniform vec3 uSceneMin;
uniform vec3 uSceneMax;

uniform vec3 uLightPos;
uniform vec3 uLightColor;
uniform float uLightIntensity;
uniform int uLightType;


struct Light {
    int type;
    vec3 position;
    vec3 direction;
    vec3 color;
    float intensity;
    float cutOff;
    float outerCutOff;
};

#define MAX_LIGHTS 8
uniform Light uLights[MAX_LIGHTS];
uniform int uNumLights;

struct GPUVertex {
    vec4 position;
    vec4 normal;
	vec4 extra;
};

struct GPUMaterial {
    vec4 albedo;
    vec4 properties;
	vec4 texIndices;
};

struct GPUNode {
    vec4 aabbMin;
    vec4 aabbMax;
};

layout(std430, binding = 0) readonly buffer VertexBuffer {
    GPUVertex vertices[];
};

layout(std430, binding = 1) readonly buffer IndexBuffer {
    uint indices[];
};

layout(std430, binding = 2) readonly buffer MaterialBuffer {
    GPUMaterial materials[];
};

layout(std430, binding = 3) readonly buffer NodeBuffer {
    GPUNode sceneNodes[];
};

uniform int uNumIndices;
uniform int uNumNodes;

struct Ray {
    vec3 origin;
    vec3 direction;
};

uniform sampler2D uTextures[16];

struct HitRecord {
    bool hit;
    float t;
    vec3 point;
    vec3 normal;
    vec3 geomNormal;
    bool frontFace;
    int materialID;
    vec2 uv;
};

#define MAX_DIST 99999.0
#define EPSILON 0.001

//Rayo Primario desde la Camara
Ray GenerateRay(vec2 fragCoord) {
    vec2 ndc = (fragCoord / uResolution) * 2.0 - 1.0;
    vec4 target = uInvProj * vec4(ndc.x, ndc.y, -1.0, 1.0);
    vec3 viewDir = target.xyz / target.w;
    vec3 rayDir = (uInvView * vec4(normalize(viewDir), 0.0)).xyz;
    return Ray(viewPos, normalize(rayDir));
}

bool RayAABBIntersect(Ray ray, vec3 boxMin, vec3 boxMax) {
    vec3 dir = ray.direction;
    if (abs(dir.x) < 1e-6) dir.x = 1e-6 * sign(dir.x + 1e-8);
    if (abs(dir.y) < 1e-6) dir.y = 1e-6 * sign(dir.y + 1e-8);
    if (abs(dir.z) < 1e-6) dir.z = 1e-6 * sign(dir.z + 1e-8);
    vec3 invDir = 1.0 / dir;
    vec3 t0 = (boxMin - ray.origin) * invDir;
    vec3 t1 = (boxMax - ray.origin) * invDir;
    vec3 tmin = min(t0, t1);
    vec3 tmax = max(t0, t1);
    float tNear = max(max(tmin.x, tmin.y), tmin.z);
    float tFar = min(min(tmax.x, tmax.y), tmax.z);
    return tFar >= tNear && tFar > 0.0;
}

//Möller-Trumbore
bool RayTriangleIntersect(Ray ray, vec3 v0, vec3 v1, vec3 v2, out float t, out vec2 uv) {
    vec3 edge1 = v1 - v0;
    vec3 edge2 = v2 - v0;
    vec3 h = cross(ray.direction, edge2);
    float a = dot(edge1, h);
    if (abs(a) < 1e-7) return false;
    float f = 1.0 / a;
    vec3 s = ray.origin - v0;
    float u = f * dot(s, h);
    if (u < 0.0 || u > 1.0) return false;
    vec3 q = cross(s, edge1);
    float v = f * dot(ray.direction, q);
    if (v < 0.0 || u + v > 1.0) return false;
    t = f * dot(edge2, q);
    uv = vec2(u, v);
    return t > EPSILON;
}

HitRecord TraverseScene(Ray ray) {
    HitRecord closestHit;
    closestHit.hit = false;
    closestHit.t = MAX_DIST;
    if (!RayAABBIntersect(ray, uSceneMin, uSceneMax)) {
        return closestHit;
    }
    uint bestI0, bestI1, bestI2;
    vec2 bestUV;
    for (int n = 0; n < uNumNodes; n++) {
        GPUNode node = sceneNodes[n];
        if (!RayAABBIntersect(ray, node.aabbMin.xyz, node.aabbMax.xyz)) continue;
        int startIdx = int(node.aabbMin.w);
        int numIdx = int(node.aabbMax.w);
        for (int i = startIdx; i < startIdx + numIdx; i += 3) {
            uint i0 = indices[i];
            uint i1 = indices[i+1];
            uint i2 = indices[i+2];
            vec3 v0 = vertices[i0].position.xyz;
            vec3 v1 = vertices[i1].position.xyz;
            vec3 v2 = vertices[i2].position.xyz;
            float t; vec2 uv;
            if (RayTriangleIntersect(ray, v0, v1, v2, t, uv)) {
                if (t < closestHit.t) {
                    closestHit.hit = true;
                    closestHit.t = t;
                    bestI0 = i0; bestI1 = i1; bestI2 = i2;
                    bestUV = uv;
                }
            }
        }
    }
    if (closestHit.hit) {
        vec3 v0 = vertices[bestI0].position.xyz;
        vec3 v1 = vertices[bestI1].position.xyz;
        vec3 v2 = vertices[bestI2].position.xyz;
        vec2 uv0 = vec2(vertices[bestI0].normal.w, vertices[bestI0].extra.x);
        vec2 uv1 = vec2(vertices[bestI1].normal.w, vertices[bestI1].extra.x);
        vec2 uv2 = vec2(vertices[bestI2].normal.w, vertices[bestI2].extra.x);
        closestHit.uv = uv0 * (1.0 - bestUV.x - bestUV.y) + uv1 * bestUV.x + uv2 * bestUV.y;
        closestHit.point = ray.origin + ray.direction * closestHit.t;
        vec3 N = cross(v1 - v0, v2 - v0);
        closestHit.geomNormal = (length(N) > 0.0001) ? normalize(N) : vec3(0.0, 1.0, 0.0);
        vec3 n0 = vertices[bestI0].normal.xyz;
        vec3 n1 = vertices[bestI1].normal.xyz;
        vec3 n2 = vertices[bestI2].normal.xyz;
        closestHit.normal = normalize(n0 * (1.0 - bestUV.x - bestUV.y) + n1 * bestUV.x + n2 * bestUV.y);
        closestHit.frontFace = dot(closestHit.geomNormal, ray.direction) < 0.0;
        if (!closestHit.frontFace) closestHit.geomNormal = -closestHit.geomNormal;
        if (dot(closestHit.normal, ray.direction) > 0.0) closestHit.normal = -closestHit.normal;
        closestHit.materialID = int(vertices[bestI0].position.w);
    }

    return closestHit;
}

const float PI = 3.14159265359;

float GeometrySchlickGGX(float NdotV, float roughness) {
    float r = (roughness + 1.0);
    float k = (r*r) / 8.0;
    return NdotV / (NdotV * (1.0 - k) + k);
}

vec3 ComputeLocalIllumination(Ray ray, HitRecord hit, GPUMaterial mat) {
    vec3 N = hit.normal;
    vec3 V = normalize(-ray.direction);
    vec3 albedo = mat.albedo.rgb;
    int albedoTexID = int(mat.texIndices.x);
    if (albedoTexID >= 0 && albedoTexID < 16) {
        albedo = texture(uTextures[albedoTexID], hit.uv).rgb;
    }
    float roughness = max(mat.properties.x, 0.05); 
    float metallic = mat.properties.y;
    vec3 ambient = albedo * uAmbientColor;
    vec3 totalLo = vec3(0.0);
    for(int i = 0; i < uNumLights; i++) {
        vec3 L;
        float attenuation = 1.0;
        float distToLight = MAX_DIST;
        if (uLights[i].type == 0) { 
            //DIRECCIONAL
            L = -uLights[i].direction;
        } else { 
            //POINT o SPOT
            vec3 lightVec = uLights[i].position - hit.point;
            distToLight = length(lightVec);
            L = normalize(lightVec);
            attenuation = 1.0 / (distToLight * distToLight + 0.0001);
            if (uLights[i].type == 2) { 
                //SPOT
                float theta = dot(L, -uLights[i].direction);
                float epsilon = max(uLights[i].cutOff - uLights[i].outerCutOff, 0.0001);
                float intensitySpot = clamp((theta - uLights[i].outerCutOff) / epsilon, 0.0, 1.0);
                attenuation *= intensitySpot;
            }
        }
        vec3 H = normalize(V + L);
        float NdotL = max(dot(N, L), 0.0);
        if (NdotL <= 0.0) continue;
        vec3 offsetOrigin = hit.point + hit.normal * EPSILON + L * EPSILON;
        Ray shadowRay = Ray(offsetOrigin, L);
        HitRecord shadowHit = TraverseScene(shadowRay);
        if (shadowHit.hit && shadowHit.t < distToLight && int(materials[shadowHit.materialID].properties.w) != 2) {
            continue;
        }

        vec3 F0 = mix(vec3(0.04), albedo, metallic);
        float cosTheta = max(dot(H, V), 0.0);
        vec3 F = F0 + (1.0 - F0) * pow(clamp(1.0 - cosTheta, 0.0, 1.0), 5.0);
        float a = roughness * roughness;
        float a2 = a * a;
        float NdotH = max(dot(N, H), 0.0);
        float denom = (NdotH * NdotH * (a2 - 1.0) + 1.0);
        float NDF = a2 / max(PI * denom * denom, 0.0000001);
        float NdotV = max(dot(N, V), 0.0);
        float G = GeometrySchlickGGX(NdotV, roughness) * GeometrySchlickGGX(NdotL, roughness);
        vec3 specular = (NDF * G * F) / max(4.0 * NdotV * NdotL, 0.0001);
        vec3 kD = (vec3(1.0) - F) * (1.0 - metallic);
        vec3 radiance = uLights[i].color * uLights[i].intensity * attenuation;
        totalLo += (kD * albedo / PI + specular) * radiance * NdotL;
    }
    return ambient + totalLo;
}

struct StackItem {
    Ray ray;
    vec3 throughput;
    int depth;
};
#define MAX_STACK 6

void main() {
    Ray primaryRay = GenerateRay(gl_FragCoord.xy);
    vec3 finalColor = vec3(0.0);
    StackItem stack[MAX_STACK]; 
    int stackPtr = 0;
    stack[stackPtr] = StackItem(primaryRay, vec3(1.0), 0);
    while (stackPtr >= 0 && stackPtr < MAX_STACK) {
        StackItem current = stack[stackPtr];
        stackPtr--;
        if (current.depth >= uMaxBounces) {
            finalColor += current.throughput * vec3(0.0);
            continue;
        }
        HitRecord hit = TraverseScene(current.ray);
        if (!hit.hit) {
            vec3 unitDir = normalize(current.ray.direction);
            float t_sky = 0.5 * (unitDir.y + 1.0);
            vec3 skyColor = mix(vec3(1.0, 1.0, 1.0), vec3(0.5, 0.7, 1.0), t_sky); 
            finalColor += current.throughput * skyColor;
            continue;
        }
        GPUMaterial mat = materials[hit.materialID];
        int matType = int(mat.properties.w);
        vec3 baseAlbedo = mat.albedo.rgb;
        int texID = int(mat.texIndices.x);
        if (texID >= 0 && texID < 16) {
            baseAlbedo = texture(uTextures[texID], hit.uv).rgb;
        }
        //PBR Base
        if (matType == 0) {
            vec3 localLight = ComputeLocalIllumination(current.ray, hit, mat);
            float metallic = mat.properties.y;
            float roughness = max(mat.properties.x, 0.05);
            if (metallic > 0.01 && stackPtr < MAX_STACK - 1) {
                vec3 V = normalize(-current.ray.direction);
                vec3 F0 = mix(vec3(0.04), baseAlbedo, metallic);
                float cosTheta = max(dot(hit.normal, V), 0.0);
                vec3 F = F0 + (max(vec3(1.0 - roughness), F0) - F0) * pow(1.0 - cosTheta, 5.0);
                float reflectionEnergy = 1.0 - roughness;
                if (reflectionEnergy > 0.05) {
                    vec3 refDir = reflect(current.ray.direction, hit.normal);
                    Ray refRay = Ray(hit.point + hit.geomNormal * EPSILON, refDir);
                    stackPtr++;
                    finalColor += current.throughput * localLight * (vec3(1.0) - F);
                    stack[stackPtr] = StackItem(refRay, current.throughput * F * reflectionEnergy, current.depth + 1);
                } else {
                    finalColor += current.throughput * localLight;
                }
            } else {
                finalColor += current.throughput * localLight;
            }
        }
        //Espejo
        else if (matType == 1) { 
            vec3 refDir = reflect(current.ray.direction, hit.normal);
            Ray refRay = Ray(hit.point + hit.geomNormal * EPSILON, refDir);
            if (stackPtr < MAX_STACK - 1) {
                stackPtr++;
                stack[stackPtr] = StackItem(refRay, current.throughput * baseAlbedo, current.depth + 1);
            }
        }
        //Cristal
        else if (matType == 2) {
            float ior = mat.properties.z;
            float eta = hit.frontFace ? (1.0 / ior) : ior;
            float cosTheta = dot(-current.ray.direction, hit.normal); 
            vec3 refrDir = refract(current.ray.direction, hit.normal, eta);
            float R;
            float sinT2 = eta * eta * (1.0 - cosTheta * cosTheta);
            if (sinT2 > 1.0 || length(refrDir) == 0.0) {
                R = 1.0; 
            } else {
                float cosX = (eta > 1.0) ? sqrt(1.0 - sinT2) : cosTheta;
                float r0 = (1.0 - ior) / (1.0 + ior);
                r0 = r0 * r0;
                R = r0 + (1.0 - r0) * pow(1.0 - cosX, 5.0);
            }
            if (R < 1.0 && stackPtr < MAX_STACK - 1) {
                vec3 d = normalize(refrDir);
                Ray refrRay = Ray(hit.point - hit.geomNormal * EPSILON, d);
                stackPtr++;
                stack[stackPtr] = StackItem(refrRay, current.throughput * (1.0 - R) * baseAlbedo, current.depth + 1);
            }
            if (R > 0.0 && stackPtr < MAX_STACK - 1) {
                vec3 refDir = reflect(current.ray.direction, hit.normal);
                Ray refRay = Ray(hit.point + hit.geomNormal * EPSILON, refDir);
                stackPtr++;
                stack[stackPtr] = StackItem(refRay, current.throughput * R, current.depth + 1);
            }
        }
    }
    finalColor = finalColor / (finalColor + vec3(1.0));
    finalColor = pow(finalColor, vec3(1.0 / 2.2));
    FragColor = vec4(finalColor, 1.0);
}