#include "VBO.h"

VBO::VBO(const std::vector<Vertex>& vertices) {
	glCreateBuffers(1, &ID);
	glNamedBufferStorage(ID, vertices.size() * sizeof(Vertex), vertices.data(), GL_DYNAMIC_STORAGE_BIT);
}

void VBO::Bind() {
	glBindBuffer(GL_ARRAY_BUFFER, ID);
}

void VBO::Unbind() {
	glBindBuffer(GL_ARRAY_BUFFER, 0);
}

void VBO::Delete() {
	glDeleteBuffers(1, &ID);
}