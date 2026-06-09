#include "Shader.h"

std::string get_file_contents(const char* filename) {
	std::ifstream in(filename, std::ios::binary);
	if (in) {
		//Leer el contenido del archivo
		std::string contents;
		//Mover el puntero al final del archivo para obtener su tamaño
		in.seekg(0, std::ios::end);
		//Redimensionar la cadena para que pueda contener el contenido del archivo
		contents.resize(in.tellg());
		//Mover el puntero al inicio del archivo para leer su contenido
		in.seekg(0, std::ios::beg);
		//Leer el contenido del archivo en la cadena
		in.read(&contents[0], contents.size());
		in.close();
		return contents;
	}
	std::cerr << "Fallo al abrir el archivo del shader: " << filename << std::endl;
	return "";
}

ShaderProgram::ShaderProgram(const char* vertexFile, const char* fragmentFile) {
	//Leer el código fuente de los shaders
	std::string vertexCode = get_file_contents(vertexFile);
	std::string fragmentCode = get_file_contents(fragmentFile);
	const char* vertexShaderSource = vertexCode.c_str();
	const char* fragmentShaderSource = fragmentCode.c_str();
	//Crear el Vertex Shader
	GLuint vertexShader, fragmentShader;
	vertexShader = glCreateShader(GL_VERTEX_SHADER);
	glShaderSource(vertexShader, 1, &vertexShaderSource, NULL);
	glCompileShader(vertexShader);
	CheckCompileErrors(vertexShader, VERTEX);
	//Crear el Fragment Shader
	fragmentShader = glCreateShader(GL_FRAGMENT_SHADER);
	glShaderSource(fragmentShader, 1, &fragmentShaderSource, NULL);
	glCompileShader(fragmentShader);
	CheckCompileErrors(fragmentShader, FRAGMENT);
	//Crear el programa de shaders
	ID = glCreateProgram();
	//Adjuntar los shaders al programa de shaders
	glAttachShader(ID, vertexShader);
	glAttachShader(ID, fragmentShader);
	//Enlazar el programa de shaders
	glLinkProgram(ID);
	CheckCompileErrors(ID, PROGRAM);
	//Eliminar los shaders
	glDeleteShader(vertexShader);
	glDeleteShader(fragmentShader);
}

void ShaderProgram::CheckCompileErrors(GLuint shader, ShaderType type) {
	GLint status = GL_FALSE;
	GLchar message[1024];
	switch (type) {
	case VERTEX:
	case FRAGMENT:
		//Verificar el estado de compilación del shader
		glGetShaderiv(shader, GL_COMPILE_STATUS, &status);
		if (!status) {
			//Obtener el mensaje de error del shader
			glGetShaderInfoLog(shader, sizeof(message), NULL, message);
			std::string errorLog(message);
			std::cerr << "SHADER ERROR: " << errorLog;
			return;
		}
		break;
	case PROGRAM:
		//Verificar el estado de enlace del programa de shaders
		glGetProgramiv(shader, GL_LINK_STATUS, &status);
		if (!status) {
			//Obtener el mensaje de error del programa de shaders
			glGetProgramInfoLog(shader, sizeof(message), NULL, message);
			std::string errorLog(message);
			std::cerr << "PROGRAM ERROR: " << errorLog;
			return;
		}
		break;
	}
}

void ShaderProgram::Activate() {
	glUseProgram(ID);
}

void ShaderProgram::Deactivate() {
	glUseProgram(0);
}

void ShaderProgram::Delete() {
	glDeleteProgram(ID);
}

//Uniformes usando DSA
void ShaderProgram::SetInt(const char* name, const GLint num) {
	//Obtener la ubicación del uniforme en el programa de shaders
	GLuint loc = glGetUniformLocation(ID, name);
	//Establecer el valor del uniforme
	glProgramUniform1i(ID, loc, num);
}
void ShaderProgram::SetFloat(const char* name, const GLfloat num) {
	//Obtener la ubicación del uniforme en el programa de shaders
	GLuint loc = glGetUniformLocation(ID, name);
	//Establecer el valor del uniforme
	glProgramUniform1f(ID, loc, num);
}
void ShaderProgram::SetVec2(const char* name, const glm::vec2& vector) {
	//Obtener la ubicación del uniforme en el programa de shaders
	GLuint loc = glGetUniformLocation(ID, name);
	//Establecer el valor del uniforme
	glProgramUniform2fv(ID, loc, 1, glm::value_ptr(vector));
}
void ShaderProgram::SetVec3(const char* name, const glm::vec3& vector) {
	//Obtener la ubicación del uniforme en el programa de shaders
	GLuint loc = glGetUniformLocation(ID, name);
	//Establecer el valor del uniforme
	glProgramUniform3fv(ID, loc, 1, glm::value_ptr(vector));
}
void ShaderProgram::SetVec4(const char* name, const glm::vec4& vector) {
	//Obtener la ubicación del uniforme en el programa de shaders
	GLuint loc = glGetUniformLocation(ID, name);
	//Establecer el valor del uniforme
	glProgramUniform4fv(ID, loc, 1, glm::value_ptr(vector));
}
void ShaderProgram::SetMatrix4(const char* name, const glm::mat4& matrix) {
	//Obtener la ubicación del uniforme en el programa de shaders
	GLuint loc = glGetUniformLocation(ID, name);
	//Establecer el valor del uniforme
	glProgramUniformMatrix4fv(ID, loc, 1, GL_FALSE, glm::value_ptr(matrix));
}