#version 460 core

in vec3 FragPos;
in vec3 Color;
in vec2 TexCoords;

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

uniform sampler2D u_albedoMap;
uniform int uHasAlbedoMap = 0;
uniform vec4 uBaseColorFactor = vec4(1.0);

uniform int isSelected = 0;
uniform int uSelectedTriangle = -1;
uniform vec3 uHighlightColor = vec3(1.0, 1.0, 0.0);

uniform int shadowType;
uniform float bias;
uniform int pcfSize;
uniform float pcssSize;
uniform sampler2D uShadowMap;
uniform mat4 uLightSpaceMatrix;
uniform int uShadowViewMode;

out vec4 FragColor;

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

void main() {
    float globalShadow = 0.0;
    vec4 fragPosLightSpace = uLightSpaceMatrix * vec4(FragPos, 1.0);
    vec3 dX = dFdx(FragPos);
    vec3 dY = dFdy(FragPos);
    vec3 norm = normalize(cross(dX, dY));
    vec3 totalDiffuse = vec3(0.0);
    for(int i = 0; i < uNumLights; i++) {
        vec3 lightDir;
        float attenuation = 1.0;
        if (uLights[i].type == 0) {
            lightDir = normalize(-uLights[i].direction);
        } else {
            float distance = length(uLights[i].position - FragPos);
            lightDir = normalize(uLights[i].position - FragPos);
            attenuation = 1.0 / max(distance * distance, 1.0);
            if (uLights[i].type == 2) {
                float theta = dot(lightDir, normalize(-uLights[i].direction));
                float epsilon = uLights[i].cutOff - uLights[i].outerCutOff;
                float intensitySpot = clamp((theta - uLights[i].outerCutOff) / epsilon, 0.0, 1.0);
                attenuation *= intensitySpot;
            }
        }
        float diff = max(dot(norm, lightDir), 0.0);
        vec3 radiance = uLights[i].color * uLights[i].intensity * attenuation;
        float currentShadow = 0.0;
        if (i == 0 && (uLights[i].type == 0 || uLights[i].type == 2)) {
            globalShadow = ShadowCalculation(fragPosLightSpace, norm, lightDir);
            if (shadowType == 1) {
                currentShadow = globalShadow;
            }
        }
        totalDiffuse += diff * radiance * (1.0 - currentShadow);
    }
    vec3 baseColor;
    if (uHasAlbedoMap == 1) {
        vec4 texColor = texture(u_albedoMap, TexCoords);
        baseColor = texColor.rgb * (totalDiffuse + vec3(0.15)) * Color;
    } else {
        baseColor = (totalDiffuse + vec3(0.15)) * Color * uBaseColorFactor.rgb;
    }
    vec4 finalColor = vec4(baseColor, 1.0);


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
