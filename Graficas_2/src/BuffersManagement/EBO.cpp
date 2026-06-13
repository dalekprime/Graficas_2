#include "EBO.h"

EBO::EBO(const std::vector<GLuint>& indices) {
	glCreateBuffers(1, &ID);
	glNamedBufferStorage(ID, indices.size() * sizeof(GLuint), indices.data(), GL_DYNAMIC_STORAGE_BIT);
}

EBO::~EBO() {
	Delete();
}

void EBO::Bind() {
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ID);
}

void EBO::Unbind() {
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
}

void EBO::Delete() {
	if (ID != 0) {
		glDeleteBuffers(1, &ID);
		ID = 0;
	}
}