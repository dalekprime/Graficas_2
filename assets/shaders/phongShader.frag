#version 460 core

in vec3 normal;
in vec3 FragPos;
in vec2 TexCoords;
in vec3 Color;
in vec3 tangent;
in vec3 biTangent;

uniform vec3 uLightPos;
uniform vec3 uCamPos;

uniform sampler2D u_albedoMap;
uniform sampler2D u_normalMap;
uniform int uHasAlbedoMap;
uniform int uHasNormalMap;

uniform int isSelected;
uniform int uSelectedTriangle;
uniform vec3 uHighlightColor;

out vec4 FragColor;

void main() {
    // TBN Real
    vec3 N = normalize(normal);
    vec3 T = normalize(tangent);
    vec3 B = normalize(biTangent);
    mat3 TBN = mat3(T, B, N);
    
    vec3 norm = N;
    if (uHasNormalMap == 1) {
        vec3 tangentNormal = texture(u_normalMap, TexCoords).xyz * 2.0 - 1.0;
        norm = normalize(TBN * tangentNormal);
    }

    // Difuso
    vec3 lightDir = normalize(uLightPos - FragPos);
    float diff = max(dot(norm, lightDir), 0.0);

    // Especular (Phong Clásico)
    vec3 viewDir = normalize(uCamPos - FragPos);
    vec3 reflectDir = reflect(-lightDir, norm);
    float spec = pow(max(dot(viewDir, reflectDir), 0.0), 32.0); // 32 es el "shininess"

    vec4 texColor = vec4(1.0);
    if (uHasAlbedoMap == 1) {
        texColor = texture(u_albedoMap, TexCoords);
    }
    vec3 albedo = texColor.rgb * Color;
    
    vec3 ambient = 0.15 * albedo;
    vec3 diffuse = diff * albedo;
    vec3 specular = spec * vec3(0.5); // Intensidad del brillo (blanco)
    
    vec4 finalColor = vec4(ambient + diffuse + specular, texColor.a);

    // Sistema de Selección
    if (isSelected == 1) {
        if (uSelectedTriangle < 0 || gl_PrimitiveID == uSelectedTriangle) {
            finalColor = mix(finalColor, vec4(uHighlightColor, 1.0), 0.5);
        }
    }
    FragColor = finalColor;
}