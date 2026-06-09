#include "Camera.h"
#include <imgui.h>

Camera::Camera(int width, int height, glm::vec3 position, glm::vec3 orientation, glm::vec3 up) {
	//tamano de la ventana
	this->width = width;
	this->height = height;
	//Posicion de la camara
	this->position = position;
	//Orientaci�n de la camara
	this->orientation = orientation;
	//Vector up de la camara (usado para rotar la camara)
	this->up = up;
}

void Camera::matrix(ShaderProgram& shader) {
	shader.SetVec3("viewPos", position);
	shader.SetMatrix4("viewMatrix", view);
	shader.SetMatrix4("projectionMatrix", projection);
}

void Camera::updateMatrix(float FOVdeg, float nearPlane, float farPlane) {
	//Crear la matriz de vista y proyeccion
	view = glm::lookAt(position, position + orientation, up);
	projection = glm::perspective(glm::radians(FOVdeg), (float)width / height, nearPlane, farPlane);
	cameraMatrix = projection * view;
}

void Camera::Inputs(GLFWwindow* window, float deltaTime) {
	float realSpeed = speed * deltaTime;
	if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS) {
		position += realSpeed * orientation;
	}
	if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS) {
		position -= realSpeed * orientation;
	}
	if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS) {
		position -= glm::normalize(glm::cross(orientation, up)) * realSpeed;
	}
	if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS) {
		position += glm::normalize(glm::cross(orientation, up)) * realSpeed;
	}
	//Mouse inputs
	if (glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_LEFT) == GLFW_PRESS) {
		if (ImGui::GetIO().WantCaptureMouse) {
			glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
			firstClick = true;
			return;
		}
		glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
		if (firstClick) {
			glfwSetCursorPos(window, (width / 2), (height / 2));
			firstClick = false;
		}
		double mouseX, mouseY;
		glfwGetCursorPos(window, &mouseX, &mouseY);
		float rotX = sensitivity * (float)(mouseY - (height / 2)) / height;
		float rotY = sensitivity * (float)(mouseX - (width / 2)) / width;
		glm::vec3 newOrientation = glm::rotate(orientation, glm::radians(-rotX), glm::normalize(glm::cross(orientation, up)));
		if (!((glm::angle(newOrientation, up) <= glm::radians(5.0f)) || (glm::angle(newOrientation, -up) <= glm::radians(5.0f)))) {
			orientation = newOrientation;
		}
		orientation = glm::rotate(orientation, glm::radians(-rotY), up);
		glfwSetCursorPos(window, (width / 2), (height / 2));
	} else if (glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_LEFT) == GLFW_RELEASE) {
		glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
		firstClick = true;
	}
}