#pragma once
#ifndef GRAFICAS_2_H
#define GRAFICAS_2_H

#include <iostream>
#include <vector>
#include <string>
#include <memory>
#include <glad/gl.h>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <imgui.h>
#include <imgui_impl_glfw.h>
#include <imgui_impl_opengl3.h>
//Librerias Internas
#include "Shader.h"
#include "BuffersManagement/VAO.h"
#include "BuffersManagement/VBO.h"
#include "BuffersManagement/EBO.h"
#include "ModelManagement/texture.h"
#include "CameraManagement/Camera.h"
#include "ModelManagement/Node.h"
#include "ModelManagement/Scene.h"
#include "ModelManagement/Solid.h"
#include "CameraManagement/Ray.h"
#include "IluminationManagement/ShadowEngine.h"

//Clase principal del motor gráfico
class MainEngine {
	int width, height;
	GLFWwindow* window;
	std::unique_ptr<ShaderProgram> flatShader;
	std::unique_ptr<ShaderProgram> lambertShader;
	std::unique_ptr<ShaderProgram> phongShader;
	std::unique_ptr<ShaderProgram> blinnPhongShader;
	std::unique_ptr<ShaderProgram> pbrShader;
	std::unique_ptr<ShaderProgram> rtShader;
	std::unique_ptr<ShaderProgram> lineShader;
	std::unique_ptr<ShaderProgram> unlitShader;
	std::unique_ptr<ShaderProgram> shadowShader;
	ShaderProgram* actualShader;
	std::unique_ptr<ShadowEngine> shadowEngine;
	std::unique_ptr<Camera> actualCamera;
	std::unique_ptr<Scene> actualScene;
	std::unique_ptr<Ray> pickingRay;
	float deltaTime = 0.0f;
	float lastFrame = 0.0f;
	int lightModel = 4; //0 Flat, 1 Lambert, 2 Phong, 3 Blinn, 4 PBR
	int shadowModel = 1; //0 Planar, 1 Mapping, 2 Volumen
	int shadowViewMode = 0;
	//UI State
	int rayPrecision = 1; // 0: Global, 1: Local, 2: Triangle
	glm::vec3 rayColor = glm::vec3(1.0f, 0.0f, 0.0f);
	Node* selectedNode = nullptr;
	//Solid
	bool showRevolutionUI = false;
	char sceneSavePath[256] = "assets/models/scene.glb";
	char sceneLoadPath[256] = "assets/scenes/Camp.glb";
	int selectedCameraIndex = 0;
	char newTexturePath[256] = "assets/textures/newTex.png";
	//Revolution UI State
	std::vector<glm::vec2> controlPoints;
	std::unique_ptr<Node> previewNode;
	int revSegments = 32;
	int revAxis = 1; // 0=X, 1=Y
	bool useBezier = true;
	int bezierRes = 20;
	glm::vec3 solidColor = glm::vec3(0.8f, 0.8f, 0.8f);
	float previewRotation = 0.0f;
	float previewPitch = 0.0f;
	float previewZoom = 0.0f;
	char solidSavePath[256] = "assets/models/revolucion.glb";
	GLuint previewFBO = 0, previewTexture = 0, previewRBO = 0;
	void InitPreviewFBO();
	void RenderPreview();
	public:
		MainEngine(int width, int height);
		void SetupWindow();
		void MainLoop();
		void Update();
		void DrawUI();
		void Cleanup();
};

#endif