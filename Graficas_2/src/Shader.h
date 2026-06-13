#pragma once
#ifndef SHADER_H
#define SHADER_H

#include <glad/gl.h>
#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <string>
#include <fstream>
#include <iostream>

//Enum para identificar el tipo de shader
enum ShaderType {
    VERTEX,
    FRAGMENT,
    PROGRAM
};
//Función para leer el contenido de un archivo y devolverlo como una cadena
std::string get_file_contents(const char* filename);
//Clase para manejar shaders
class ShaderProgram {
public:
    GLuint ID;
    ShaderProgram(const char* vertexFile, const char* fragmentFile);
    ~ShaderProgram();
    void CheckCompileErrors(GLuint shader, ShaderType type);
    void Activate();
    void Deactivate();
    void Delete();
	//Funciones para establecer uniformes
    void SetInt(const char* name, const GLint num);
    void SetFloat(const char* name, const GLfloat num);
    void SetVec2(const char* name, const glm::vec2& vector);
    void SetVec3(const char* name, const glm::vec3& vector);
    void SetVec4(const char* name, const glm::vec4& vector);
    void SetMatrix4(const char* name, const glm::mat4& matrix);
};

#endif