#version 460 core

in vec3 normal;
in vec3 FragPos;
in vec3 LocalPos;
in vec2 TexCoords;
in vec3 Color;
in vec3 tangent;
in vec3 biTangent;

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

uniform vec3 viewPos;

uniform sampler2D u_albedoMap;
uniform sampler2D u_normalMap;
uniform int uHasAlbedoMap;
uniform int uHasNormalMap;
uniform vec4 uBaseColorFactor;

uniform int isSelected;
uniform int uSelectedTriangle;
uniform vec3 uHighlightColor;
uniform int uUVMappingMode;

const float PI = 3.14159265359;

out vec4 FragColor;

void main() {
    vec2 normalTexCoords = TexCoords;
    if (uHasNormalMap == 1 && uUVMappingMode > 0) {
        vec3 p = normalize(LocalPos);
        float u = (atan(p.z, p.x) / (2.0 * PI)) + 0.5;
        float v;
        if (uUVMappingMode == 1) { // Cilindrico
            v = p.y * 0.5 + 0.5; 
        } else { // Esferico
            v = (asin(p.y) / PI) + 0.5;
        }
        normalTexCoords = vec2(u, v);
    }

    vec3 N = normalize(normal);
    vec3 norm = N;
    if (uHasNormalMap == 1) {
        mat3 TBN;
        if (uUVMappingMode > 0) {
            vec3 Q1 = dFdx(FragPos);
            vec3 Q2 = dFdy(FragPos);
            vec2 st1 = dFdx(normalTexCoords);
            vec2 st2 = dFdy(normalTexCoords);
            
            vec3 T = normalize(Q1 * st2.t - Q2 * st1.t);
            vec3 B = -normalize(cross(N, T));
            TBN = mat3(T, B, N);
        } else {
            vec3 T = normalize(tangent);
            vec3 B = normalize(biTangent);
            TBN = mat3(T, B, N);
        }
        
        vec3 tangentNormal = texture(u_normalMap, normalTexCoords).xyz * 2.0 - 1.0;
        norm = normalize(TBN * tangentNormal);
    }

    vec3 totalDiffuse = vec3(0.0);
    vec3 totalSpecular = vec3(0.0);
    vec3 viewDir = normalize(viewPos - FragPos);

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
                float epsilon = max(uLights[i].cutOff - uLights[i].outerCutOff, 0.0001);
                float intensitySpot = clamp((theta - uLights[i].outerCutOff) / epsilon, 0.0, 1.0);
                attenuation *= intensitySpot;
            }
        }

        // Diffuse
        float diff = max(dot(norm, lightDir), 0.0);
        // Specular
        vec3 reflectDir = reflect(-lightDir, norm);
        float spec = pow(max(dot(viewDir, reflectDir), 0.0), 32.0);

        vec3 radiance = uLights[i].color * uLights[i].intensity * attenuation;

        totalDiffuse += diff * radiance;
        totalSpecular += spec * radiance * 0.5;
    }


    vec3 albedo;
    if (uHasAlbedoMap == 1) {
        vec4 texColor = texture(u_albedoMap, TexCoords);
        albedo = texColor.rgb * Color;
    } else {
        albedo = Color * uBaseColorFactor.rgb;
    }
    
    vec3 ambient = 0.15 * albedo;
    vec3 diffuse = totalDiffuse * albedo;
    vec3 specular = totalSpecular;
    
    vec4 finalColor = vec4(ambient + diffuse + specular, 1.0);

    // Sistema de Selección
    if (isSelected == 1) {
        if (uSelectedTriangle < 0 || gl_PrimitiveID == uSelectedTriangle) {
            finalColor = mix(finalColor, vec4(uHighlightColor, 1.0), 0.5);
        }
    }
    FragColor = finalColor;
}