#version 460 core
in vec3 normal;
in vec3 FragPos;
in vec2 TexCoords;
in vec3 Color;
in vec3 tangent;   // Recibimos la tangente real de tu C++
in vec3 biTangent; // Recibimos la bitangente real de tu C++

// === Uniformes Sincronizados con tu clase Material ===
uniform vec3 uLightPos;
uniform vec3 uCamPos;

uniform sampler2D u_albedoMap;
uniform sampler2D u_normalMap;
uniform int uHasAlbedoMap;
uniform int uHasNormalMap;
uniform float uMetallic;
uniform float uRoughness;

uniform int isSelected; // Tu C++ envia "isSelected", no "uIsSelected"
uniform int uSelectedTriangle;
uniform vec3 uHighlightColor;

out vec4 FragColor;

const float PI = 3.14159265359;

float DistributionGGX(vec3 N, vec3 H, float roughness) {
    float a = roughness*roughness;
    float a2 = a*a;
    float NdotH = max(dot(N, H), 0.0);
    float NdotH2 = NdotH*NdotH;
    float nom   = a2;
    float denom = (NdotH2 * (a2 - 1.0) + 1.0);
    denom = PI * denom * denom;
    return nom / denom;
}

float GeometrySchlickGGX(float NdotV, float roughness) {
    float r = (roughness + 1.0);
    float k = (r*r) / 8.0;
    float nom   = NdotV;
    float denom = NdotV * (1.0 - k) + k;
    return nom / denom;
}

float GeometrySmith(vec3 N, vec3 V, vec3 L, float roughness) {
    float NdotV = max(dot(N, V), 0.0);
    float NdotL = max(dot(N, L), 0.0);
    float ggx2 = GeometrySchlickGGX(NdotV, roughness);
    float ggx1 = GeometrySchlickGGX(NdotL, roughness);
    return ggx1 * ggx2;
}

vec3 fresnelSchlick(float cosTheta, vec3 F0) {
    return F0 + (1.0 - F0) * pow(clamp(1.0 - cosTheta, 0.0, 1.0), 5.0);
}

void main() {
    // 1. Leer texturas con los nombres correctos
    vec3 albedo = Color;
    if (uHasAlbedoMap == 1) {
        albedo = texture(u_albedoMap, TexCoords).rgb * Color;
    }
    
    // 2. Construir la matriz TBN usando TUS cálculos de C++ (Físicamente correcto)
    vec3 N = normalize(normal);
    if (uHasNormalMap == 1) {
        vec3 T = normalize(tangent);
        vec3 B = normalize(biTangent);
        mat3 TBN = mat3(T, B, N);
        
        // 3. Extraer el relieve y aplicarlo al TBN
        vec3 tangentNormal = texture(u_normalMap, TexCoords).xyz * 2.0 - 1.0;
        N = normalize(TBN * tangentNormal);
    }

    // Cálculos de PBR
    vec3 V = normalize(uCamPos - FragPos);
    vec3 L = normalize(uLightPos - FragPos);
    vec3 H = normalize(V + L);
    
    vec3 F0 = vec3(0.04); 
    F0 = mix(F0, albedo, uMetallic);

    float distance = length(uLightPos - FragPos);
    float attenuation = 1.0 / (distance * distance);
    vec3 radiance = vec3(300.0) * attenuation; // Intensidad de luz dura

    float NDF = DistributionGGX(N, H, uRoughness);   
    float G   = GeometrySmith(N, V, L, uRoughness);
    vec3 F    = fresnelSchlick(max(dot(H, V), 0.0), F0);

    vec3 nominator    = NDF * G * F;
    float denominator = 4.0 * max(dot(N, V), 0.0) * max(dot(N, L), 0.0) + 0.0001;
    vec3 specular = nominator / denominator;

    vec3 kS = F;
    vec3 kD = vec3(1.0) - kS;
    kD *= 1.0 - uMetallic;

    float NdotL = max(dot(N, L), 0.0);        
    vec3 Lo = (kD * albedo / PI + specular) * radiance * NdotL;

    vec3 ambient = vec3(0.03) * albedo;
    vec3 color = ambient + Lo;
    
    // Tone mapping y corrección Gamma
    color = color / (color + vec3(1.0));
    color = pow(color, vec3(1.0/2.2));
    
    vec4 finalColor = vec4(color, 1.0);
    
    // 4. Lógica de Raycast / Selección corregida
    if (isSelected == 1) {
        if (uSelectedTriangle < 0 || gl_PrimitiveID == uSelectedTriangle) {
            finalColor = mix(finalColor, vec4(uHighlightColor, 1.0), 0.5);
        }
    }
    FragColor = finalColor;
}