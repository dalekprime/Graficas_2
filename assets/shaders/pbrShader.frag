#version 460 core
in vec3 normal;
in vec3 FragPos;
in vec3 LocalPos;
in vec2 TexCoords;
in vec3 Color;
in vec3 tangent;
in vec3 biTangent;
in mat3 TBN;

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

uniform vec3 viewPos;

uniform sampler2D u_albedoMap;
uniform sampler2D u_normalMap;
uniform sampler2D u_pbrMap;
uniform int uHasAlbedoMap;
uniform int uHasNormalMap;
uniform int uHasPbrMap;
uniform float uMetallic;
uniform float uRoughness;
uniform float uAoFactor;
uniform vec4 uBaseColorFactor;

uniform int isSelected;
uniform int uSelectedTriangle;
uniform vec3 uHighlightColor;
uniform int uUVMappingMode;

uniform int shadowType;
uniform float bias;
uniform int pcfSize;
uniform float pcssSize;
uniform sampler2D uShadowMap;
uniform mat4 uLightSpaceMatrix;
uniform int uShadowViewMode;

out vec4 FragColor;

const float PI = 3.14159265359;

float ShadowCalculation(vec4 fragPosLightSpace, vec3 normal, vec3 lightDir) {
    int pcfLimit = (pcfSize-1)/2;
    vec3 proj = fragPosLightSpace.xyz / fragPosLightSpace.w;
    proj = proj * 0.5 + 0.5;
    if(proj.z > 1.0) return 0.0;
    float currentDepth = proj.z;
    if(pcfSize > 0) {
        //PCF
        float shadowPCF = 0.0;
        vec2 texel = 1.0 / textureSize(uShadowMap, 0);
        for(int x = -pcfLimit; x <= pcfLimit; x++) {
            for(int y = -pcfLimit; y <= pcfLimit; y++) {
                float d = texture(uShadowMap, proj.xy + vec2(x, y) * texel).r; 
                shadowPCF += currentDepth - bias > d ? 1.0 : 0.0;        
            }    
        }
        return shadowPCF / float(pcfSize * pcfSize);
    } else if(pcfSize == 0) {
        float closestDepth = texture(uShadowMap, proj.xy).r;
        return (currentDepth - bias > closestDepth) ? 1.0 : 0.0;
    } else {
        //PCSS
        float shadowPCSS = 0.0;
        vec2 texel = 1.0 / textureSize(uShadowMap, 0);
        float blockerDepth = 0.0;
        int blockers = 0;
        int searchWidth = 5;
        float searchRadius = 4.0;
        for(int x = -searchWidth; x <= searchWidth; x++) {
            for(int y = -searchWidth; y <= searchWidth; y++) {
                float d = texture(uShadowMap, proj.xy + vec2(x, y) * texel * searchRadius).r;
                if(d < currentDepth - bias) {
                    blockerDepth += d;
                    blockers++;
                }
            }
        }
        if(blockers == 0) return 0.0;
        float avgBlockerDepth = blockerDepth / float(blockers);
        float distanceToBlocker = max(currentDepth - avgBlockerDepth, 0.0);
        float penumbra = distanceToBlocker * pcssSize;
        float filterRadius = clamp(penumbra, 1.0, 20.0);
        int samples = 0;
        for(int x = -2; x <= 2; x++) {
            for(int y = -2; y <= 2; y++) {
                float d = texture(uShadowMap, proj.xy + vec2(x, y) * texel * filterRadius).r; 
                shadowPCSS += (currentDepth - bias > d) ? 1.0 : 0.0;
                samples++;
            }    
        }
        return shadowPCSS / float(samples);
    }
}

float D_GGX(vec3 N, vec3 H, float roughness) {
    float a = roughness * roughness;
    float a2 = a * a;
    float NdotH = max(dot(N, H), 0.0);
    float NdotH2 = NdotH*NdotH;
    float denom = (NdotH2 * (a2 - 1.0) + 1.0);
    denom = PI * denom * denom;
    return a2 / max(denom, 0.0000001);
}

float G_Smith(vec3 N, vec3 V, vec3 L, float roughness) {
    float NdotV = max(dot(N, V), 0.0);
    float NdotL = max(dot(N, L), 0.0);
    float k = (roughness + 1.0) * (roughness + 1.0) / 8.0;
    float Gv = NdotV / (NdotV * (1.0-k) + k);
    float Gl = NdotL / (NdotL * (1.0-k) + k);
    return Gv * Gl;
}

vec3 F_Schlick(vec3 H, vec3 V, vec3 albedo, float metalic) {
    float HdotV = max(dot(H, V), 0.0);
    vec3 F0 = mix(vec3(0.04), albedo, metalic);
    return F0 + (1.0 - F0) * pow(1.0 - HdotV, 5.0);
}

void main() {
    //Albedo
    vec3 albedo;
    if (uHasAlbedoMap == 1) {
        vec3 texColor = texture(u_albedoMap, TexCoords).rgb;
        albedo = pow(texColor, vec3(2.2)) * pow(Color, vec3(2.2));
    } else {
        vec3 baseColorLinear = pow(Color * uBaseColorFactor.rgb, vec3(2.2));
        albedo = baseColorLinear;
    }
    //Normales con UVs
    vec2 normalTexCoords = TexCoords;
    if (uHasNormalMap == 1 && uUVMappingMode > 0) {
        vec3 p = normalize(LocalPos);
        float u = (atan(p.z, p.x) / (2.0 * PI)) + 0.5;
        float v;
        if (uUVMappingMode == 1) { 
            // Cilindrico
            v = p.y * 0.5 + 0.5; 
        } else { 
            // Esferico
            v = (asin(p.y) / PI) + 0.5;
        }
        normalTexCoords = vec2(u, v);
    }
    vec3 N = normalize(normal);
    if (uHasNormalMap == 1) {
        vec3 nTS = texture(u_normalMap, normalTexCoords).rgb * 2.0 - 1.0;
        N = normalize(TBN * nTS);
    }
    //PBR
    float currentMetallic = uMetallic;
    float currentRoughness = uRoughness;
    float currentAO = uAoFactor;
    if (uHasPbrMap == 1) {
        vec4 pbrSample = texture(u_pbrMap, TexCoords);
        currentAO *= pbrSample.r;
        currentRoughness *= pbrSample.g;
        currentMetallic *= pbrSample.b;
    }
    currentRoughness = max(currentRoughness, 0.04);
    vec3 V = normalize(viewPos - FragPos);
    vec3 Lo = vec3(0.0);
    float globalShadow = 0.0;
    vec4 fragPosLightSpace = uLightSpaceMatrix * vec4(FragPos, 1.0);
    //Revisar todas las luces
    for(int i = 0; i < uNumLights; i++) {
        vec3 L;
        float attenuation = 1.0;
        if (uLights[i].type == 0) { 
            // Directional
            L = normalize(-uLights[i].direction);
        } else { 
            //Point/Spot
            vec3 lightVec = uLights[i].position - FragPos;
            float distance = length(lightVec);
            L = normalize(lightVec);
            attenuation = 1.0 / (distance * distance + 0.0001);
            if (uLights[i].type == 2) { 
                //Spot
                float theta = dot(L, normalize(-uLights[i].direction));
                float epsilon = max(uLights[i].cutOff - uLights[i].outerCutOff, 0.0001);
                float intensitySpot = clamp((theta - uLights[i].outerCutOff) / epsilon, 0.0, 1.0);
                attenuation *= intensitySpot;
            }
        }
        vec3 H = normalize(V + L);
        vec3 radiance = uLights[i].color * uLights[i].intensity * attenuation;
        float D = D_GGX(N, H, currentRoughness);   
        float G = G_Smith(N, V, L, currentRoughness);
        vec3 F = F_Schlick(H, V, albedo, currentMetallic);
        vec3 specular = (D * F * G) / (4.0 * max(dot(N, V), 0.0) * max(dot(N, L), 0.0) + 0.0001);
        vec3 kD = (vec3(1.0) - F) * (1.0 - currentMetallic);
        float shadow = 0.0;
        if (i == 0 && (uLights[i].type == 0 || uLights[i].type == 2)) {
            shadow = ShadowCalculation(fragPosLightSpace, N, L);
            globalShadow = shadow;
        }
        float currentShadow = (shadowType == 1) ? shadow : 0.0;
        Lo += (kD * albedo / PI + specular) * radiance * max(dot(N, L), 0.0) * (1.0 - currentShadow);
    }
    //Combinar
    vec3 ambient = vec3(0.03) * albedo * currentAO * (1.0 - currentMetallic);
    vec3 color = ambient + Lo;
    //Correcion Gamma
    color = color / (color + 1.0);
    color = pow(color, vec3(1.0 / 2.2));
    vec4 finalColor = vec4(color, 1.0);
    //RayCast
    if (isSelected == 1) {
        if (uSelectedTriangle < 0 || gl_PrimitiveID == uSelectedTriangle) {
            finalColor = mix(finalColor, vec4(uHighlightColor, 1.0), 0.5);
        }
    }
    FragColor = finalColor;
    if (shadowType == 1) {
        if (uShadowViewMode == 1) {
            FragColor = vec4(vec3(1.0 - globalShadow), 1.0);
        } else if (uShadowViewMode == 2) {
            vec3 proj = fragPosLightSpace.xyz / fragPosLightSpace.w;
            proj = proj * 0.5 + 0.5;
            float depthVal = texture(uShadowMap, proj.xy).r;
            FragColor = vec4(vec3(depthVal), 1.0);
        }
    }
}