#pragma once
#include <glm/glm.hpp>
#include <glad/gl.h>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include "../Shader.h"

enum LightType {
    DIRECTIONAL,
    POINT,
    SPOT
};

class Light {
public:
    LightType type;
    glm::vec3 position;
    glm::vec3 direction;
    glm::vec3 color;
    GLfloat intensity;
    GLfloat cutOff;
    GLfloat outerCutOff;
    Light(LightType type = DIRECTIONAL, glm::vec3 pos = glm::vec3(1.0f));
    ~Light();
    bool showGizmo;
    void DrawGizmo(ShaderProgram& shaderProgram);
private:
    GLuint vao, vbo;
    int numVertices;
    void setupGizmo();
};
