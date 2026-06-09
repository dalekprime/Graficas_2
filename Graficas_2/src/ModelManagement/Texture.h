#ifndef TEXTURE_H
#define TEXTURE_H

#include <glad/gl.h>
#include <stb_image.h>
#include <cmath>
#include <iostream>
#include "../shader.h"

enum texType {
	ALBEDO,
	NORMAL,
	BUMP,
	PBR,
	ROUGHNESS,
	METALLIC,
	AO
};

class Texture {
public:
	GLuint ID = 0;
	texType type = ALBEDO;
	std::string path = "";
	//Textura desde Imagen
	Texture(const char* image, texType texType, GLuint slot, GLenum format);
	//Textura desde GLTF
	Texture(const unsigned char* data, int width, int height, int nrChannels, texType texType);
	void Bind(GLuint unit);
	void Unbind(GLuint unit);
	void Delete();
};

#endif 
