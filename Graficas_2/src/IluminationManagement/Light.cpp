#include "Light.h"
#include <vector>

Light::Light(LightType type, glm::vec3 pos):
        type(type), position(pos), direction(0.0f, -1.0f, 0.0f),
      color(1.0f, 1.0f, 1.0f), intensity(1.0f),
      cutOff(glm::cos(glm::radians(30.0f))), outerCutOff(glm::cos(glm::radians(45.0f))),
      showGizmo(true), vao(0), vbo(0){}

Light::~Light() {
    if (vao != 0) {
        glDeleteVertexArrays(1, &vao);
        glDeleteBuffers(1, &vbo);
    }
}

void Light::setupGizmo() {
    if (vao != 0) {
        glDeleteVertexArrays(1, &vao);
        glDeleteBuffers(1, &vbo);
    }
    std::vector<GLfloat> vertices;
    if (type == POINT) {
        // Esfera solida
        int stacks = 16;
        int sectors = 16;
        float radius = 0.2f;
        glm::vec3 offset(0.0f);
        if (type == DIRECTIONAL) {
            offset = glm::vec3(0.0f, 0.0f, -0.4f);
        }

        for (int i = 0; i < stacks; ++i) {
            float phi1 = glm::pi<float>() * float(i) / float(stacks);
            float phi2 = glm::pi<float>() * float(i + 1) / float(stacks);

            for (int j = 0; j < sectors; ++j) {
                float theta1 = 2.0f * glm::pi<float>() * float(j) / float(sectors);
                float theta2 = 2.0f * glm::pi<float>() * float(j + 1) / float(sectors);

                glm::vec3 p1(radius * glm::cos(theta1) * glm::sin(phi1), radius * glm::cos(phi1), radius * glm::sin(theta1) * glm::sin(phi1));
                glm::vec3 p2(radius * glm::cos(theta2) * glm::sin(phi1), radius * glm::cos(phi1), radius * glm::sin(theta2) * glm::sin(phi1));
                glm::vec3 p3(radius * glm::cos(theta1) * glm::sin(phi2), radius * glm::cos(phi2), radius * glm::sin(theta1) * glm::sin(phi2));
                glm::vec3 p4(radius * glm::cos(theta2) * glm::sin(phi2), radius * glm::cos(phi2), radius * glm::sin(theta2) * glm::sin(phi2));

                // Triangulo 1
                vertices.push_back(p1.x + offset.x); vertices.push_back(p1.y + offset.y); vertices.push_back(p1.z + offset.z);
                vertices.push_back(p3.x + offset.x); vertices.push_back(p3.y + offset.y); vertices.push_back(p3.z + offset.z);
                vertices.push_back(p2.x + offset.x); vertices.push_back(p2.y + offset.y); vertices.push_back(p2.z + offset.z);

                // Triangulo 2
                vertices.push_back(p2.x + offset.x); vertices.push_back(p2.y + offset.y); vertices.push_back(p2.z + offset.z);
                vertices.push_back(p3.x + offset.x); vertices.push_back(p3.y + offset.y); vertices.push_back(p3.z + offset.z);
                vertices.push_back(p4.x + offset.x); vertices.push_back(p4.y + offset.y); vertices.push_back(p4.z + offset.z);
            }
        }
    } else if (type == SPOT || type == DIRECTIONAL) {
        // Cono solido
        int segments = 16;
        float height = (type == DIRECTIONAL) ? 1.0f : 0.5f;
        float angle = glm::acos((type == DIRECTIONAL) ? 0.9f : outerCutOff);
        float radius = height * glm::tan(angle);
        glm::vec3 tip(0.0f, 0.0f, 0.0f);
        glm::vec3 baseCenter(0.0f, 0.0f, height);

        for (int i = 0; i < segments; i++) {
            float theta1 = ((float)i / segments) * 2.0f * glm::pi<float>();
            float theta2 = ((float)(i + 1) / segments) * 2.0f * glm::pi<float>();
            glm::vec3 p1(radius * glm::cos(theta1), radius * glm::sin(theta1), height);
            glm::vec3 p2(radius * glm::cos(theta2), radius * glm::sin(theta2), height);

            // Triangulo desde la punta hasta la base
            vertices.push_back(tip.x); vertices.push_back(tip.y); vertices.push_back(tip.z);
            vertices.push_back(p1.x);  vertices.push_back(p1.y);  vertices.push_back(p1.z);
            vertices.push_back(p2.x);  vertices.push_back(p2.y);  vertices.push_back(p2.z);

            // Triangulo para la tapa de la base
            vertices.push_back(baseCenter.x); vertices.push_back(baseCenter.y); vertices.push_back(baseCenter.z);
            vertices.push_back(p2.x);         vertices.push_back(p2.y);         vertices.push_back(p2.z);
            vertices.push_back(p1.x);         vertices.push_back(p1.y);         vertices.push_back(p1.z);
        }
    }

    numVertices = vertices.size() / 3;
    glGenVertexArrays(1, &vao);
    glGenBuffers(1, &vbo);
    glBindVertexArray(vao);
    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(GLfloat), vertices.data(), GL_STATIC_DRAW);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(GLfloat), (void*)0);
    glEnableVertexAttribArray(0);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);
}

void Light::DrawGizmo(ShaderProgram& shaderProgram) {
    if (!showGizmo) return;
    setupGizmo();
    glm::mat4 model = glm::translate(glm::mat4(1.0f), position);
    if (type == DIRECTIONAL || type == SPOT) {
        glm::vec3 dir = glm::normalize(direction);
        glm::vec3 up = glm::vec3(0.0f, 1.0f, 0.0f);
        if (glm::abs(glm::dot(dir, up)) > 0.999f) {
            up = glm::vec3(0.0f, 0.0f, 1.0f);
        }
        glm::vec3 right = glm::normalize(glm::cross(up, dir));
        glm::vec3 realUp = glm::normalize(glm::cross(dir, right));
        glm::mat4 rot(1.0f);
        rot[0] = glm::vec4(right, 0.0f);
        rot[1] = glm::vec4(realUp, 0.0f);
        rot[2] = glm::vec4(dir, 0.0f);
        model *= rot;
    }
    shaderProgram.Activate();
    shaderProgram.SetMatrix4("modelMatrix", model);
    shaderProgram.SetInt("uUseOverride", 1);
    shaderProgram.SetVec3("uColorOverride", color);
    glBindVertexArray(vao);
    glDrawArrays(GL_TRIANGLES, 0, numVertices);
    glBindVertexArray(0);
}
