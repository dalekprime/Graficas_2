#pragma once

#include <iostream>
#include <vector>
#include <string>
#include <glad/gl.h>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <stb_image.h>
#include <tiny_obj_loader.h>
#include <imgui.h>
#include <imgui_impl_glfw.h>
#include <imgui_impl_opengl3.h>
#include <tiny_gltf.h>
#include "Shader.h"

class mainEngine {
	int width, height;
	GLFWwindow* window;
	public:
		mainEngine(int width, int height) {
			this->width = width;
			this->height = height;
			this->window = nullptr;
			initWindow();
		}
		~mainEngine() {
			// Cleanup ImGui
			ImGui_ImplOpenGL3_Shutdown();
			ImGui_ImplGlfw_Shutdown();
			ImGui::DestroyContext();
			// Cleanup Window
			if (window) {
				glfwDestroyWindow(window);
			}
			glfwTerminate();
		}
		void initWindow() {
			if (!glfwInit()) {
				std::cout << "Failed to initialize GLFW" << std::endl;
				return;
			}
			glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
			glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 6);
			glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
			window = glfwCreateWindow(width, height, "Graficas 2 - Engine", NULL, NULL);
			if (window == NULL) {
				std::cout << "Failed to create GLFW window" << std::endl;
				glfwTerminate();
				return;
			}
			glfwMakeContextCurrent(window);
			if (!gladLoadGL((GLADloadfunc)glfwGetProcAddress)) {
				std::cout << "Failed to initialize GLAD" << std::endl;
				return;
			}
			glViewport(0, 0, width, height);
			// Initialize ImGui
			IMGUI_CHECKVERSION();
			ImGui::CreateContext();
			ImGui::StyleColorsDark();
			// Setup Platform/Renderer backends
			ImGui_ImplGlfw_InitForOpenGL(window, true);
			ImGui_ImplOpenGL3_Init("#version 460");
		}
		void mainLoop() {
			while (!glfwWindowShouldClose(window)) {
				glfwPollEvents();
				glViewport(0, 0, width, height);
				glClearColor(0.08f, 0.09f, 0.12f, 1.0f);
				glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
				update();
				drawUI();
				glfwSwapBuffers(window);
			}
		}
		void update() {
			// Update
		}
		void drawUI() {
			// Interface
		}
};