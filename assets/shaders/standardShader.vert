#version 460 core

layout (location = 0) in vec3 aPos;
layout (location = 1) in vec3 aNormal;
layout (location = 2) in vec3 aColor;
layout (location = 3) in vec2 aTex;
layout (location = 4) in vec3 aTangent;
layout (location = 5) in vec3 aBiTangent;

uniform mat4 modelMatrix;
uniform mat4 viewMatrix;
uniform mat4 projectionMatrix;

out vec3 FragPos;
out vec3 LocalPos;
out vec3 normal;
out vec3 Color;
out vec2 TexCoords;
out vec3 tangent;
out vec3 biTangent;
out mat3 TBN;

void main() {
    gl_Position = projectionMatrix * viewMatrix * modelMatrix * vec4(aPos, 1.0);
    mat3 normalMatrix = mat3(transpose(inverse(modelMatrix)));
    FragPos = vec3(modelMatrix * vec4(aPos, 1.0));
    LocalPos = aPos;
    normal = normalMatrix * aNormal;
    tangent = normalMatrix * aTangent;
    biTangent = normalMatrix * aBiTangent;
    Color = aColor;
    TexCoords = aTex;
    vec3 T = normalize(mat3(modelMatrix) * tangent);
    vec3 B = normalize(mat3(modelMatrix) * biTangent);
    vec3 N = normalize(mat3(modelMatrix) * normal);
    TBN = mat3(T, B, N); 
}
