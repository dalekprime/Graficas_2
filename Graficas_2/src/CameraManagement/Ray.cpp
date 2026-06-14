#include "Ray.h"


Ray::Ray(glm::vec3 origin, glm::vec3 direction, glm::vec3 color) :
    origin(origin), direction(direction), color(color),
    vao(nullptr), vbo(nullptr), hasHit(false), initialized(false) {}

Ray::~Ray() {
    if (vao) { delete vao; vao = nullptr; }
    if (vbo) { delete vbo; vbo = nullptr; }
}

void Ray::UpdateLine() {
    if (!hasHit) return;
    std::vector<Vertex> vertices(2);
    vertices[0].position = origin;
    vertices[0].color = color;
    vertices[0].normal = glm::vec3(0.0f);
    vertices[0].texCoords = glm::vec2(0.0f);
    vertices[1].position = hitPoint;
    vertices[1].color = color;
    vertices[1].normal = glm::vec3(0.0f);
    vertices[1].texCoords = glm::vec2(0.0f);
    if (!initialized) {
        vao = new VAO();
        vao->Bind();
        vbo = new VBO(vertices);
        vao->LinkAttrib(*vbo, 0, 3, GL_FLOAT, sizeof(Vertex), 0);
        vao->LinkAttrib(*vbo, 2, 3, GL_FLOAT, sizeof(Vertex), 2 * sizeof(glm::vec3));
        vao->Unbind();
        vbo->Unbind();
        initialized = true;
    } else {
        glNamedBufferSubData(vbo->ID, 0, vertices.size() * sizeof(Vertex), vertices.data());
    }
}

void Ray::Draw(ShaderProgram& shader, Camera& camera) {
    if (!hasHit || !initialized) return;
    shader.Activate();
    //Enviar las matrices de la cámara
    camera.matrix(shader);
    //Enviar la matriz del modelo local
    glm::mat4 model = glm::mat4(1.0f);
    glUniformMatrix4fv(glGetUniformLocation(shader.ID, "modelMatrix"), 1, GL_FALSE, glm::value_ptr(model));
    shader.SetInt("uUseOverride", 0);
    vao->Bind();
    glDrawArrays(GL_LINES, 0, 2);
    vao->Unbind();
}
