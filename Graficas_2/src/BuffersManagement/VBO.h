#pragma once
#ifndef VBO_H
#define VBO_H

#include <vector>
#include <glad/gl.h>
#include <glm/glm.hpp>

struct Vertex {
	glm::vec3 position;
	glm::vec3 normal;
	glm::vec3 color;
	glm::vec2 texCoords;
	glm::vec3 tangent;
	glm::vec3 biTangent;
};

class VBO {
public:
	GLuint ID = 0;
	VBO(const std::vector<Vertex>& vertices);
	~VBO();
	void Bind();
	void Unbind();
	void Delete();
};

#endif
