#version 460 core

in vec3 FragPos;
in vec3 Color;
in vec2 TexCoords;

// Ignoramos in vec3 normal; porque queremos geometría facetada

uniform vec3 uLightPos;
uniform sampler2D u_albedoMap;
uniform int uHasAlbedoMap;

uniform int isSelected;
uniform int uSelectedTriangle;
uniform vec3 uHighlightColor;

out vec4 FragColor;

void main() {
    // Truco matemático: Derivar la posición de la pantalla para obtener una normal plana y dura (Facetada)
    vec3 dX = dFdx(FragPos);
    vec3 dY = dFdy(FragPos);
    vec3 norm = normalize(cross(dX, dY));

    vec3 lightDir = normalize(uLightPos - FragPos);
    float diff = max(dot(norm, lightDir), 0.0);

    float ambient = 0.15;
    vec4 texColor = vec4(1.0);
    if (uHasAlbedoMap == 1) {
        texColor = texture(u_albedoMap, TexCoords);
    }
    vec3 baseColor = texColor.rgb * (diff + ambient) * Color;

    vec4 finalColor = vec4(baseColor, texColor.a);

    // Sistema de Selección
    if (isSelected == 1) {
        if (uSelectedTriangle < 0 || gl_PrimitiveID == uSelectedTriangle) {
            finalColor = mix(finalColor, vec4(uHighlightColor, 1.0), 0.5);
        }
    }
    FragColor = finalColor;
}