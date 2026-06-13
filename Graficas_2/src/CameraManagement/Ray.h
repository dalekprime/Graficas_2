#pragma once
#ifndef RAY_H
#define RAY_H

#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp>
#include "../Shader.h"
#include "Camera.h"
#include "../BuffersManagement/VAO.h"
#include "../BuffersManagement/VBO.h"
#include <vector>

enum class RayPrecision {
    GLOBAL,
    LOCAL,
    TRIANGLE
};

struct RayHit {
    bool hasHit = false;
    float distance = 0.0f;
    glm::vec3 worldPoint;
    class Node* rootNode = nullptr;
    class Node* localNode = nullptr;
    glm::vec3 triV0, triV1, triV2;
    int selectedTriangleIndex = -1;
};

class Ray {
public:
    VAO* vao = nullptr;
    VBO* vbo = nullptr;
    glm::vec3 origin;
    glm::vec3 direction;
    glm::vec3 color;
    glm::vec3 hitPoint = glm::vec3(0.0f);
    bool hasHit = false;
    bool initialized = false;
    RayPrecision precision = RayPrecision::LOCAL;
    Ray(glm::vec3 origin, glm::vec3 direction, glm::vec3 color);
    ~Ray();
    // Prevenir copias superficiales (Rule of Three)
    Ray(const Ray&) = delete;
    Ray& operator=(const Ray&) = delete;
    void Draw(ShaderProgram& shader, Camera& camera);
    void UpdateLine();
};

#endif
