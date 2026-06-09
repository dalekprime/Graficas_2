#include "VAO.h"

VAO::VAO() {
	glCreateVertexArrays(1, &ID);
}

void VAO::LinkAttrib(VBO& VBO, GLuint layout, GLint numComponents, GLenum type,
	GLsizei stride, GLuint offset) {
	//Enlaza el VBO en el BindingIndex de el VAO
	glVertexArrayVertexBuffer(ID, 0, VBO.ID, 0, stride);
	//Define el Formato de Atributo
	glVertexArrayAttribFormat(ID, layout, numComponents, type, GL_FALSE, offset);
	//Conecta el Atributo con el VBO en el BindingIndex indicado
	glVertexArrayAttribBinding(ID, layout, 0);
	//Enciende el Atributo y lo deja listo para usar
	glEnableVertexArrayAttrib(ID, layout);
}

void VAO::LinkEBO(EBO& EBO) {
	glVertexArrayElementBuffer(ID, EBO.ID);
}

void VAO::Bind() {
	glBindVertexArray(ID);
}

void VAO::Unbind() {
	glBindVertexArray(0);
}

void VAO::Delete() {
	glDeleteVertexArrays(1, &ID);
}