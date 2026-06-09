#ifndef  CAMERA_H
#define  CAMERA_H

#include <string>
#include <glad/gl.h>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtx/rotate_vector.hpp>
#include <glm/gtx/vector_angle.hpp>
#include "../shader.h"

class Camera {
public:
	glm::vec3 position;
	glm::vec3 orientation = glm::vec3(0.0f, 0.0f, -1.0f);
	glm::vec3 up = glm::vec3(0.0f, 1.0f, 0.0f);
	glm::mat4 cameraMatrix = glm::mat4(1.0f);
	glm::mat4 view = glm::mat4(1.0f);
	glm::mat4 projection = glm::mat4(1.0f);
	int width;
	int height;
	float sensitivity = 100.0f;
	float speed = 5.0f;
	bool firstClick = true;
	float fov = 45.0f;
	float znear = 0.1f;
	float zfar = 100.0f;
	std::string name = "";
	Camera(int width, int height, glm::vec3 position, glm::vec3 orientation, glm::vec3 up);
	void updateMatrix(float FOVdeg, float nearPlane, float farPlane);
	void matrix(ShaderProgram& shader);
	void Inputs(GLFWwindow* window, float deltaTime);
};

#endif

