#pragma once
#ifndef VAO_H
#define VAO_H

#include <glad/gl.h>
#include "VBO.h"
#include "EBO.h"

class VAO {
public:
	GLuint ID = 0;
	VAO();
	~VAO();
	void LinkAttrib(VBO& VBO, GLuint layout, GLint numComponents,
		GLenum type, GLsizei stride, GLuint offset);
	void LinkEBO(EBO& EBO);
	void Bind();
	void Unbind();
	void Delete();
};

#endif
