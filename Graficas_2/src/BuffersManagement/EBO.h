#pragma once
#ifndef EBO_H
#define EBO_H

#include <vector>
#include <glad/gl.h>

class EBO {
public:
	GLuint ID = 0;
	EBO(const std::vector<GLuint>& indices);
	~EBO();
	void Bind();
	void Unbind();
	void Delete();
};

#endif
