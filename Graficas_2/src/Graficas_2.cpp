#include "Graficas_2.h"

MainEngine::MainEngine(int width, int height) : width(width), height(height) {
	SetupWindow();
}

void MainEngine::SetupWindow() {
	if (!glfwInit()) {
		std::cerr << "Failed to initialize GLFW" << std::endl;
		exit(EXIT_FAILURE);
	}
	//Indica a GLFW que versión de OpenGL queremos usar
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 6);
	//Indica a GLFW que queremos usar el perfil core de OpenGL
	glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
	//Crear la ventana
	window = glfwCreateWindow(width, height, "Graficas 2", nullptr, nullptr);
	if (!window) {
		std::cerr << "Failed to create GLFW window" << std::endl;
		glfwTerminate();
		exit(EXIT_FAILURE);
	}
	//Hacer el contexto de la ventana actual
	glfwMakeContextCurrent(window);
	//Cargar las funciones de OpenGL usando GLAD
	if (!gladLoadGL(glfwGetProcAddress)) {
		std::cerr << "Failed to initialize GLAD" << std::endl;
		exit(EXIT_FAILURE);
	}
	//Configurar el viewport
	glViewport(0, 0, width, height);
	//Crear los programas de shaders
	flatShader = std::make_unique<ShaderProgram>("assets/shaders/standardShader.vert", "assets/shaders/flatShader.frag");
	lambertShader = std::make_unique<ShaderProgram>("assets/shaders/standardShader.vert", "assets/shaders/lambertShader.frag");
	phongShader = std::make_unique<ShaderProgram>("assets/shaders/standardShader.vert", "assets/shaders/phongShader.frag");
	blinnPhongShader = std::make_unique<ShaderProgram>("assets/shaders/standardShader.vert", "assets/shaders/blinnPhongShader.frag");
	lineShader = std::make_unique<ShaderProgram>("assets/shaders/standardShader.vert", "assets/shaders/lineShader.frag");
	pbrShader = std::make_unique<ShaderProgram>("assets/shaders/standardShader.vert", "assets/shaders/pbrShader.frag");
	actualShader = flatShader.get();
	pickingRay = std::make_unique<Ray>(glm::vec3(0.0f), glm::vec3(0.0f), rayColor);
	//Habilitar el depth test para objetos 3D
	glEnable(GL_DEPTH_TEST);
	//Habilitar el blending para la transparencia
	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	//Crear la cámara
	actualCamera = std::make_unique<Camera>(width, height, glm::vec3(0.0f, 0.0f, 5.0f), glm::vec3(0.0f, 0.0f, -1.0f), glm::vec3(0.0f, 1.0f, 0.0f));	//Crear la escena y cargar el modelo glTF/GLB
	actualScene = std::make_unique<Scene>();
	//Abrir Escena
	try {
		actualScene->LoadFromGLTF("assets/scenes/Camp.glb");
	} catch (const std::exception& e) {
		std::cout << "Aviso: " << e.what() << " (Se iniciará una escena vacía)" << std::endl;
	};
	// Inicializar ImGui
	IMGUI_CHECKVERSION();
	ImGui::CreateContext();
	ImGuiIO& io = ImGui::GetIO(); (void)io;
	ImGui::StyleColorsDark();
	ImGui_ImplGlfw_InitForOpenGL(window, true);
	ImGui_ImplOpenGL3_Init("#version 460 core");
	InitPreviewFBO();
}

void MainEngine::InitPreviewFBO() {
	glCreateFramebuffers(1, &previewFBO);
	glCreateTextures(GL_TEXTURE_2D, 1, &previewTexture);
	glTextureStorage2D(previewTexture, 1, GL_RGBA8, 512, 512);
	glTextureParameteri(previewTexture, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTextureParameteri(previewTexture, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glCreateRenderbuffers(1, &previewRBO);
	glNamedRenderbufferStorage(previewRBO, GL_DEPTH_COMPONENT24, 512, 512);
	glNamedFramebufferTexture(previewFBO, GL_COLOR_ATTACHMENT0, previewTexture, 0);
	glNamedFramebufferRenderbuffer(previewFBO, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, previewRBO);
}

void MainEngine::RenderPreview() {
	if (!previewNode) return;
	glBindFramebuffer(GL_FRAMEBUFFER, previewFBO);
	glViewport(0, 0, 512, 512);
	glClearColor(0.1f, 0.1f, 0.1f, 1.0f);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	//Configurar transformaciones de la matriz local del FBO
	glm::mat4 model = glm::mat4(1.0f);
	model = glm::translate(model, glm::vec3(0.0f, 0.0f, previewZoom));
	model = glm::rotate(model, glm::radians(previewPitch), glm::vec3(1.0f, 0.0f, 0.0f));
	model = glm::rotate(model, glm::radians(previewRotation), glm::vec3(0.0f, 1.0f, 0.0f));
	//Actualizamos el mundo del previewNode
	previewNode->localMatrix = model;
	previewNode->UpdateWorldMatrix();
	flatShader->Activate();
	Camera dummyCam(512, 512, glm::vec3(0.0f, 0.0f, 5.0f), glm::vec3(0.0f, 0.0f, -1.0f), glm::vec3(0.0f, 1.0f, 0.0f));
	dummyCam.updateMatrix(45.0f, 0.1f, 100.0f);
	dummyCam.matrix(*flatShader);
    flatShader->SetVec3("uLightPos", glm::vec3(2.0f, 2.0f, 5.0f));
	previewNode->Draw(*flatShader, dummyCam, false);
	glBindFramebuffer(GL_FRAMEBUFFER, 0);
	//Restaurar Viewport principal
	glViewport(0, 0, width, height); 
}

void MainEngine::MainLoop() {
	bool wasClicked = false;
	while (!glfwWindowShouldClose(window)) {
		//Delta Time
		float currentFrame = glfwGetTime();
		deltaTime = currentFrame - lastFrame;
		lastFrame = currentFrame;
		glfwPollEvents();
		Update();
		//Raycast Logic
		if (glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_RIGHT) == GLFW_PRESS && !ImGui::GetIO().WantCaptureMouse) {
			if (!wasClicked) {
				wasClicked = true;
				double mouseX, mouseY;
				glfwGetCursorPos(window, &mouseX, &mouseY);
				//Normalizar coordenadas
				float x = (2.0f * (float)mouseX) / width - 1.0f;
				float y = 1.0f - (2.0f * (float)mouseY) / height;
				glm::vec4 ray_clip = glm::vec4(x, y, -1.0f, 1.0f);
				//Calcular vector dirección global
				glm::vec4 ray_eye = glm::inverse(actualCamera->projection) * ray_clip;
				ray_eye = glm::vec4(ray_eye.x, ray_eye.y, -1.0f, 0.0f);
				glm::vec3 ray_wor = glm::normalize(glm::vec3(glm::inverse(actualCamera->view) * ray_eye));
				//Configurar nuestro Rayo y lanzarlo a la escena
				pickingRay->origin = actualCamera->position;
				pickingRay->direction = ray_wor;
				pickingRay->color = rayColor;
				pickingRay->precision = static_cast<RayPrecision>(rayPrecision);
				RayHit hit = actualScene->Raycast(*pickingRay);
				if (selectedNode) selectedNode->isSelected = false;
				if (hit.hasHit) {
					selectedNode = hit.localNode;
					if (selectedNode) {
						selectedNode->isSelected = true;
						if (rayPrecision == 2) {
							selectedNode->selectedTriangle = hit.selectedTriangleIndex;
						} else {
							selectedNode->selectedTriangle = -1;
						}
					}
				} else {
					selectedNode = nullptr;
				}
			}
		} else {
			wasClicked = false;
		}
		//Selección de Shaders
		if (lightModel == 0) actualShader = flatShader.get();
		else if (lightModel == 1) actualShader = lambertShader.get();
		else if (lightModel == 2) actualShader = phongShader.get();
		else if (lightModel == 3) actualShader = blinnPhongShader.get();
		else if (lightModel == 4) actualShader = pbrShader.get();
		actualShader->Activate();
		//Inputs de la cámara
		actualCamera->Inputs(window, deltaTime);
		actualCamera->updateMatrix(45.0f, 0.1f, 100.0f);
		actualCamera->matrix(*actualShader);
		actualShader->SetVec3("uLightPos", lightPos);
        //Seleccion
        actualShader->SetVec3("uHighlightColor", rayColor);
		//Actualizar matemáticas y dibujar
		actualScene->UpdateScene();
		actualScene->Draw(*actualShader, *actualCamera, false);
		if (pickingRay->hasHit) {
			pickingRay->Draw(*lineShader, *actualCamera);
		}
		if (showRevolutionUI) {
			RenderPreview();
		}
		DrawUI();
		glfwSwapBuffers(window);
	}
	Cleanup();
}

void MainEngine::Update() {
	//Configurar el color de fondo
	glClearColor(0.07f, 0.13f, 0.17f, 1.0f);
	//Limpiar el color y el depth buffer
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
}

void MainEngine::DrawUI() {
    ImGui_ImplOpenGL3_NewFrame();
    ImGui_ImplGlfw_NewFrame();
    ImGui::NewFrame();
    //VENTANA PRINCIPAL
    ImGui::Begin("Controles Graficas 2");
    ImGui::Text("Iluminacion y Materiales");
    ImGui::Text("Modelo de Iluminacion");
    ImGui::RadioButton("Flat", &lightModel, 0); ImGui::SameLine();
    ImGui::RadioButton("Lambert", &lightModel, 1); ImGui::SameLine();
    ImGui::RadioButton("Phong", &lightModel, 2); ImGui::SameLine();
    ImGui::RadioButton("Blinn-Phong", &lightModel, 3); ImGui::SameLine();
    ImGui::RadioButton("PBR", &lightModel, 4);
    ImGui::DragFloat3("Posicion Luz", glm::value_ptr(lightPos), 0.1f);
    ImGui::Separator();
    ImGui::Text("Rayo y Picking");
    ImGui::ColorEdit3("Color Rayo", glm::value_ptr(rayColor));
    ImGui::Combo("Precision Rayo", &rayPrecision, "Objeto (Global)\0Submallado (Local)\0Triangulo Exacto\0\0");
    ImGui::Checkbox("Mostrar UI de Revolucion", &showRevolutionUI);
    ImGui::Separator();
    ImGui::InputText("Ruta Guardado Escena", sceneSavePath, sizeof(sceneSavePath));
    if (ImGui::Button("Guardar Escena Completa")) {
        actualScene->SaveToGLTF(sceneSavePath);
    }
    //INSPECTOR DE NODOS
    if (selectedNode) {
        ImGui::Separator();
        ImGui::Text("Propiedades del Nodo Seleccionado");
        ImGui::TextColored(ImVec4(0.0f, 1.0f, 0.0f, 1.0f), "Nombre: %s", selectedNode->name.c_str());
        glm::vec3 pos(selectedNode->localMatrix[3]);
        if (ImGui::DragFloat3("Posicion", glm::value_ptr(pos), 0.1f)) {
            selectedNode->localMatrix[3][0] = pos.x;
            selectedNode->localMatrix[3][1] = pos.y;
            selectedNode->localMatrix[3][2] = pos.z;
        }
        if (selectedNode->material) {
            ImGui::SliderFloat("Metallic", &selectedNode->material->metallicFactor, 0.0f, 1.0f);
            ImGui::SliderFloat("Roughness", &selectedNode->material->roughnessFactor, 0.0f, 1.0f);
            ImGui::Text("Texturas");
            const char* texTypes[] = { "Base Color", "Normal Map" };
            for (size_t i = 0; i < 2; ++i) {
                bool hasTex = i < selectedNode->material->textures.size();
                GLuint texID = hasTex && selectedNode->material->textures[i] ? selectedNode->material->textures[i]->ID : 0;
                if (hasTex && texID != 0) {
                    ImGui::Text("%s: ID %u", texTypes[i], texID);
                } else {
                    ImGui::Text("%s: Vacio", texTypes[i]);
                }
                ImGui::SameLine();
                if (ImGui::Button(("Cambiar##" + std::to_string(i)).c_str())) {
                    ImGui::OpenPopup(("SeleccionarTextura" + std::to_string(i)).c_str());
                }
                if (ImGui::BeginPopup(("SeleccionarTextura" + std::to_string(i)).c_str())) {
                    for (size_t j = 0; j < actualScene->textureCache.size(); ++j) {
                        if (ImGui::Selectable(("Textura " + std::to_string(j) + " (ID " + std::to_string(actualScene->textureCache[j]->ID) + ")").c_str())) {
                            if (i >= selectedNode->material->textures.size()) {
                                selectedNode->material->textures.resize(i + 1, nullptr);
                            }
                            selectedNode->material->textures[i] = actualScene->textureCache[j];
                        }
                    }
                    ImGui::Separator();
                    ImGui::InputText("Ruta Textura", newTexturePath, sizeof(newTexturePath));
                    if (ImGui::Button("Cargar desde disco")) {
                        int imgWidth, imgHeight, imgChannels;
                        unsigned char* bytes = stbi_load(newTexturePath, &imgWidth, &imgHeight, &imgChannels, 0);
                        if (bytes) {
                            // Cargar a VRAM y guardar en Caché
                            texType texType = (i == 0) ? ALBEDO : NORMAL;
                            auto newTex = std::make_shared<Texture>(bytes, imgWidth, imgHeight, imgChannels, texType);
                            newTex->path = newTexturePath;
                            actualScene->textureCache.push_back(newTex);
                            stbi_image_free(bytes);
                        } else {
                            std::cerr << "Error cargando textura: " << newTexturePath << std::endl;
                        }
                    }
                    ImGui::EndPopup();
                }
            }
        }
    }
    ImGui::End();
    //SÓLIDOS DE REVOLUCIÓN
    if (showRevolutionUI) {
        ImGui::SetNextWindowSize(ImVec2(800, 600), ImGuiCond_FirstUseEver);
        ImGui::Begin("Generador Procedural - Solidos de Revolucion", &showRevolutionUI);
        ImGui::Columns(2, "RevColumns");
        ImGui::SetColumnWidth(0, 420.0f);
        //PANEL IZQUIERDO: PREVIEW
        ImGui::Text("Preview 3D (FBO)");
        ImVec2 pPos = ImGui::GetCursorScreenPos();
        //Renderizamos la textura generada por el Framebuffer
        ImGui::Image((void*)(intptr_t)previewTexture, ImVec2(400, 400), ImVec2(0, 1), ImVec2(1, 0));
        ImGui::SetCursorScreenPos(pPos);
        ImGui::InvisibleButton("preview_canvas", ImVec2(400, 400));
        if (ImGui::IsItemHovered()) {
            previewZoom += ImGui::GetIO().MouseWheel * 0.5f;
        }
        if (ImGui::IsItemActive() && ImGui::IsMouseDragging(0)) {
            previewRotation += ImGui::GetIO().MouseDelta.x;
            previewPitch += ImGui::GetIO().MouseDelta.y;
        }
        ImGui::Spacing();
        if (ImGui::Button("Generar Mallado")) {
            previewNode.reset();
            selectedNode = nullptr;
            std::vector<glm::vec2> profile = useBezier ? Solid::EvaluateBezier(controlPoints, bezierRes) : controlPoints;
            previewNode = Solid::GenerateRevolution(profile, revSegments, revAxis, solidColor, "Objeto_Procedural");
        }
        ImGui::SameLine();
        if (ImGui::Button("Añadir a Escena Global")) {
            if (previewNode) {
                actualScene->AddRootNode(std::move(previewNode));
            }
        }
        ImGui::Spacing();
        ImGui::InputText("Archivo Solido", solidSavePath, sizeof(solidSavePath));
        if (ImGui::Button("Exportar Sólido a GLB")) {
            if (previewNode) {
                Scene tempScene;
                tempScene.AddRootNode(std::move(previewNode));
                tempScene.SaveToGLTF(solidSavePath);
                previewNode = std::move(tempScene.rootNodes.front());
                tempScene.rootNodes.clear();
            }
        }
        ImGui::NextColumn();
        //PANEL DERECHO: CANVAS 2D
        ImGui::Text("Dibujar Perfil (Canvas 2D)");
        ImGui::SameLine();
        if (ImGui::Button("Limpiar Puntos")) {
            controlPoints.clear();
            previewNode.reset();
        }
        ImDrawList* draw_list = ImGui::GetWindowDrawList();
        ImVec2 canvas_pos = ImGui::GetCursorScreenPos();
        ImVec2 canvas_size = ImVec2(350, 350);
        //Fondo del canvas
        draw_list->AddRectFilled(canvas_pos, ImVec2(canvas_pos.x + canvas_size.x, canvas_pos.y + canvas_size.y), IM_COL32(50, 50, 50, 255));
        draw_list->AddRect(canvas_pos, ImVec2(canvas_pos.x + canvas_size.x, canvas_pos.y + canvas_size.y), IM_COL32(255, 255, 255, 255));
        ImGui::InvisibleButton("canvas", canvas_size);
        if (ImGui::IsItemHovered()) {
            if (ImGui::IsMouseClicked(ImGuiMouseButton_Left)) {
                //Añadir punto normalizado al clickar izquierdo
                controlPoints.push_back(glm::vec2((ImGui::GetMousePos().x - canvas_pos.x - canvas_size.x / 2) / (canvas_size.x / 2),
                    -(ImGui::GetMousePos().y - canvas_pos.y - canvas_size.y / 2) / (canvas_size.y / 2)));
            }
            else if (ImGui::IsMouseClicked(ImGuiMouseButton_Right) && !controlPoints.empty()) {
                //Borrar último punto al clickar derecho
                controlPoints.pop_back();
            }
        }
        //Dibujar Grid Central
        draw_list->AddLine(ImVec2(canvas_pos.x + canvas_size.x / 2, canvas_pos.y), ImVec2(canvas_pos.x + canvas_size.x / 2, canvas_pos.y + canvas_size.y), IM_COL32(100, 100, 100, 255));
        draw_list->AddLine(ImVec2(canvas_pos.x, canvas_pos.y + canvas_size.y / 2), ImVec2(canvas_pos.x + canvas_size.x, canvas_pos.y + canvas_size.y / 2), IM_COL32(100, 100, 100, 255));
        // Dibujar Líneas del Perfil
        std::vector<glm::vec2> profile = useBezier ? Solid::EvaluateBezier(controlPoints, bezierRes) : controlPoints;
        for (size_t i = 0; i < profile.size(); ++i) {
            ImVec2 p(canvas_pos.x + canvas_size.x / 2 + profile[i].x * canvas_size.x / 2,
                canvas_pos.y + canvas_size.y / 2 - profile[i].y * canvas_size.y / 2);
            if (i > 0) {
                ImVec2 p0(canvas_pos.x + canvas_size.x / 2 + profile[i - 1].x * canvas_size.x / 2,
                    canvas_pos.y + canvas_size.y / 2 - profile[i - 1].y * canvas_size.y / 2);
                draw_list->AddLine(p0, p, IM_COL32(255, 255, 0, 255), 2.0f);
            }
        }
        //Dibujar los Puntos de Control
        for (size_t i = 0; i < controlPoints.size(); ++i) {
            ImVec2 p(canvas_pos.x + canvas_size.x / 2 + controlPoints[i].x * canvas_size.x / 2,
                canvas_pos.y + canvas_size.y / 2 - controlPoints[i].y * canvas_size.y / 2);
            draw_list->AddCircleFilled(p, 4.0f, IM_COL32(255, 0, 0, 255));
        }
        //CONTROLES DE SOLIDOS
        ImGui::Spacing();
        ImGui::Checkbox("Interpolar con Bezier", &useBezier);
        if (useBezier) ImGui::SliderInt("Resolucion Bezier", &bezierRes, 5, 50);
        ImGui::SliderInt("Cantidad de Segmentos", &revSegments, 3, 100);
        ImGui::Combo("Eje de Rotacion", &revAxis, "Eje X\0Eje Y\0\0");
        ImGui::ColorEdit3("Color Base del Solido", glm::value_ptr(solidColor));
        ImGui::End();
    }
    ImGui::Render();
    ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
}

void MainEngine::Cleanup() {
	ImGui_ImplOpenGL3_Shutdown();
	ImGui_ImplGlfw_Shutdown();
	ImGui::DestroyContext();
	glfwDestroyWindow(window);
	glfwTerminate();
}

int main() {
	MainEngine engine(1280, 720);
	engine.MainLoop();
	return 0;
}
