#version 460 core

in vec3 FragPos;
in vec3 Color;
in vec2 TexCoords;

// Ignoramos in vec3 normal; porque queremos geometría facetada

struct Light {
    int type; // 0 = Dir, 1 = Point, 2 = Spot
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
uniform int uHasAlbedoMap;
uniform vec4 uBaseColorFactor;

uniform int isSelected;
uniform int uSelectedTriangle;
uniform vec3 uHighlightColor;

out vec4 FragColor;

void main() {
    // Truco matemático: Derivar la posición de la pantalla para obtener una normal plana y dura (Facetada)
    vec3 dX = dFdx(FragPos);
    vec3 dY = dFdy(FragPos);
    vec3 norm = normalize(cross(dX, dY));

    vec3 totalDiffuse = vec3(0.0);

    for(int i = 0; i < uNumLights; i++) {
        vec3 lightDir;
        float attenuation = 1.0;

        if (uLights[i].type == 0) { // Directional
            lightDir = normalize(-uLights[i].direction);
        } else { // Point or Spot
            float distance = length(uLights[i].position - FragPos);
            lightDir = normalize(uLights[i].position - FragPos);
            attenuation = 1.0 / max(distance * distance, 1.0);
            
            if (uLights[i].type == 2) { // Spot
                float theta = dot(lightDir, normalize(-uLights[i].direction));
                float epsilon = uLights[i].cutOff - uLights[i].outerCutOff;
                float intensitySpot = clamp((theta - uLights[i].outerCutOff) / epsilon, 0.0, 1.0);
                attenuation *= intensitySpot;
            }
        }

        float diff = max(dot(norm, lightDir), 0.0);
        vec3 radiance = uLights[i].color * uLights[i].intensity * attenuation;

        totalDiffuse += diff * radiance;
    }


    vec3 baseColor;
    if (uHasAlbedoMap == 1) {
        vec4 texColor = texture(u_albedoMap, TexCoords);
        baseColor = texColor.rgb * (totalDiffuse + vec3(0.15)) * Color;
    } else {
        baseColor = (totalDiffuse + vec3(0.15)) * Color * uBaseColorFactor.rgb;
    }

    vec4 finalColor = vec4(baseColor, 1.0);

    // Sistema de Selección
    if (isSelected == 1) {
        if (uSelectedTriangle < 0 || gl_PrimitiveID == uSelectedTriangle) {
            finalColor = mix(finalColor, vec4(uHighlightColor, 1.0), 0.5);
        }
    }
    FragColor = finalColor;
}