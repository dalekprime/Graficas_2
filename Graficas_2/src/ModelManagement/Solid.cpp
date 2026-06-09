#include "Solid.h"

std::vector<glm::vec2> Solid::EvaluateBezier(const std::vector<glm::vec2>& controlPoints, int resolution) {
    std::vector<glm::vec2> curve;
    if (controlPoints.size() < 2) return controlPoints;
    for (int i = 0; i <= resolution; i++) {
        float t = (float)i / (float)resolution;
        //Algoritmo de De Casteljau
        std::vector<glm::vec2> temp = controlPoints;
        int n = temp.size() - 1;
        for (int j = 1; j <= n; j++) {
            for (int k = 0; k <= n - j; k++) {
                temp[k] = (1.0f - t) * temp[k] + t * temp[k + 1];
            }
        }
        curve.push_back(temp[0]);
    }
    return curve;
}

std::unique_ptr<Node> Solid::GenerateRevolution(const std::vector<glm::vec2>& profile, 
                    int segments, int axis, glm::vec3 color, const std::string& nodeName) {
    if (profile.size() < 2 || segments < 3) return nullptr;
    std::vector<Vertex> vertices;
    std::vector<GLuint> indices;
    float angleStep = glm::two_pi<float>() / (float)segments;
    //Generar Vértices
    for (int s = 0; s <= segments; s++) {
        float angle = s * angleStep;
        float cosA = glm::cos(angle);
        float sinA = glm::sin(angle);
        for (size_t p = 0; p < profile.size(); p++) {
            Vertex v = {};
            v.color = color;
            v.texCoords = glm::vec2((float)s / segments, (float)p / (profile.size() - 1));
            if (axis == 1) { 
                // Eje Y
                v.position = glm::vec3(profile[p].x * cosA, profile[p].y, profile[p].x * -sinA);
            } else { 
                // Eje X
                v.position = glm::vec3(profile[p].x, profile[p].y * cosA, profile[p].y * -sinA);
            }
            vertices.push_back(v);
        }
    }
    //Generar Índices
    int pSize = profile.size();
    for (int s = 0; s < segments; s++) {
        for (int p = 0; p < pSize - 1; p++) {
            int current = s * pSize + p;
            int next = (s + 1) * pSize + p;
            indices.push_back(current);
            indices.push_back(next);
            indices.push_back(current + 1);
            indices.push_back(next);
            indices.push_back(next + 1);
            indices.push_back(current + 1);
        }
    }
    //Generar Normales, Tangentes y Bitangentes
    for (size_t i = 0; i < indices.size(); i += 3) {
        int i0 = indices[i];
        int i1 = indices[i + 1];
        int i2 = indices[i + 2];
        glm::vec3& v0 = vertices[i0].position;
        glm::vec3& v1 = vertices[i1].position;
        glm::vec3& v2 = vertices[i2].position;
        glm::vec2& uv0 = vertices[i0].texCoords;
        glm::vec2& uv1 = vertices[i1].texCoords;
        glm::vec2& uv2 = vertices[i2].texCoords;
        //Cálculos delta
        glm::vec3 deltaPos1 = v1 - v0;
        glm::vec3 deltaPos2 = v2 - v0;
        glm::vec2 deltaUV1 = uv1 - uv0;
        glm::vec2 deltaUV2 = uv2 - uv0;
        //Normal del triángulo
        glm::vec3 normal = glm::cross(deltaPos1, deltaPos2);
        //Tangentes y Bitangentes
        float r = 1.0f / (deltaUV1.x * deltaUV2.y - deltaUV1.y * deltaUV2.x);
        glm::vec3 tangent = (deltaPos1 * deltaUV2.y - deltaPos2 * deltaUV1.y) * r;
        glm::vec3 bitangent = (deltaPos2 * deltaUV1.x - deltaPos1 * deltaUV2.x) * r;
        //Vértices
        vertices[i0].normal += normal;
        vertices[i1].normal += normal;
        vertices[i2].normal += normal;
        vertices[i0].tangent += tangent;
        vertices[i1].tangent += tangent;
        vertices[i2].tangent += tangent;
        vertices[i0].biTangent += bitangent;
        vertices[i1].biTangent += bitangent;
        vertices[i2].biTangent += bitangent;
    }
    //Normalizar la acumulación para suavizar
    for (size_t i = 0; i < vertices.size(); ++i) {
        vertices[i].normal = glm::normalize(vertices[i].normal);
        vertices[i].tangent = glm::normalize(vertices[i].tangent);
        vertices[i].biTangent = glm::normalize(vertices[i].biTangent);
    }
    //Instanciar Nodo
    auto node = std::make_unique<Node>(nodeName);
    node->mesh = std::make_shared<Mesh>(vertices, indices);
    node->material = std::make_shared<Material>();
    return node;
}