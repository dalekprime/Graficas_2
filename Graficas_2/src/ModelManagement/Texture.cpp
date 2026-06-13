#include "Texture.h"

Texture::Texture(const char* image, texType texType, GLuint slot, GLenum format) {
	type = texType;
	path = image;
	int width, height, nrChannels;
	//Para glTF, NO debemos voltear la imagen, ya que OpenGL asigna el primer byte a (0,0) que para glTF es el top-left
	stbi_set_flip_vertically_on_load(false);
	unsigned char* data = stbi_load(image, &width, &height, &nrChannels, 0);
	if (data) {
		GLenum formatDetected = format;
		GLenum internalFormat = GL_RGB8;
		if (nrChannels == 1) {
			formatDetected = GL_RED;
			internalFormat = GL_R8;
		}
		else if (nrChannels == 3) {
			formatDetected = GL_RGB;
			internalFormat = (type == ALBEDO) ? GL_SRGB8 : GL_RGB8;
		}
		else if (nrChannels == 4) {
			formatDetected = GL_RGBA;
			internalFormat = (type == ALBEDO) ? GL_SRGB8_ALPHA8 : GL_RGBA8;
		}
		//Crea el espacio para la textura
		glCreateTextures(GL_TEXTURE_2D, 1, &ID);
		// Calcula el número de mipmaps posibles
		GLsizei mipLevels = (GLsizei)log2(std::max(width, height)) + 1;
		//Reserva el espacio de memoria para la textura
		glTextureStorage2D(ID, mipLevels, internalFormat, width, height);
		//Envia la textura a la GPU con alineación de 1 byte
		glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
		glTextureSubImage2D(ID, 0, 0, 0, width, height, formatDetected, GL_UNSIGNED_BYTE, data);
		glPixelStorei(GL_UNPACK_ALIGNMENT, 4);
		//Colocan los parametros de textura
		glTextureParameteri(ID, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_NEAREST);
		glTextureParameteri(ID, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
		glTextureParameteri(ID, GL_TEXTURE_WRAP_S, GL_REPEAT);
		glTextureParameteri(ID, GL_TEXTURE_WRAP_T, GL_REPEAT);
		//Genera los Mipmaps
		glGenerateTextureMipmap(ID);
		stbi_image_free(data);
	}
	else {
		std::cerr << "Failed to load texture: " << image << std::endl;
	}
}

Texture::Texture(const unsigned char* data, int width, int height, int nrChannels, texType texType) {
	type = texType;
	if (data) {
		GLenum formatDetected = GL_RGB;
		GLenum internalFormat = GL_RGB8;
		if (nrChannels == 1) {
			formatDetected = GL_RED;
			internalFormat = GL_R8;
		}
		else if (nrChannels == 3) {
			formatDetected = GL_RGB;
			internalFormat = (type == ALBEDO) ? GL_SRGB8 : GL_RGB8;
		}
		else if (nrChannels == 4) {
			formatDetected = GL_RGBA;
			internalFormat = (type == ALBEDO) ? GL_SRGB8_ALPHA8 : GL_RGBA8;
		}
		glCreateTextures(GL_TEXTURE_2D, 1, &ID);
		GLsizei mipLevels = (GLsizei)std::log2(std::max(width, height)) + 1;
		glTextureStorage2D(ID, mipLevels, internalFormat, width, height);
		glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
		glTextureSubImage2D(ID, 0, 0, 0, width, height, formatDetected, GL_UNSIGNED_BYTE, data);
		glPixelStorei(GL_UNPACK_ALIGNMENT, 4);
		glTextureParameteri(ID, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
		glTextureParameteri(ID, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
		glTextureParameteri(ID, GL_TEXTURE_WRAP_S, GL_REPEAT);
		glTextureParameteri(ID, GL_TEXTURE_WRAP_T, GL_REPEAT);
		glGenerateTextureMipmap(ID);
	}
	else {
		std::cerr << "Failed to load texture from memory" << std::endl;
	}
}

Texture::~Texture() {
	Delete();
}

void Texture::Bind(GLuint unit) {
	glBindTextureUnit(unit, ID);
}

void Texture::Unbind(GLuint unit) {
	glBindTextureUnit(unit, 0);
}

void Texture::Delete() {
	if (ID != 0) {
		glDeleteTextures(1, &ID);
		ID = 0;
	}
}
