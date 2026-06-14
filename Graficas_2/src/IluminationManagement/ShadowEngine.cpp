#include "shadowEngine.h"

void ShadowEngine::InitShadowMapping() {
	//Z-Buffer
	glCreateTextures(GL_TEXTURE_2D, 1, &depthMap);
	glTextureStorage2D(depthMap, 1, GL_DEPTH_COMPONENT24, shadowRes, shadowRes);
	glTextureParameteri(depthMap, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTextureParameteri(depthMap, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTextureParameteri(depthMap, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
	glTextureParameteri(depthMap, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);
	glm::vec3 borderColor = glm::vec3(1.0f);
	glTextureParameterfv(depthMap, GL_TEXTURE_BORDER_COLOR, glm::value_ptr(borderColor));
	//FrameBuffer
	glCreateFramebuffers(1, &depthMapFBO);
	glNamedFramebufferTexture(depthMapFBO, GL_DEPTH_ATTACHMENT, depthMap, 0);
	glNamedFramebufferDrawBuffer(depthMapFBO, GL_NONE);
	glNamedFramebufferReadBuffer(depthMapFBO, GL_NONE);
};

void ShadowEngine::RenderPlanar(Scene& scene, Camera& camera, ShaderProgram& actualShader) {
	glDepthMask(GL_FALSE);
	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	glEnable(GL_STENCIL_TEST);
	planarShader->Activate();
	planarShader->SetMatrix4("viewMatrix", camera.view);
	planarShader->SetMatrix4("projectionMatrix", camera.projection);
	planarShader->SetVec4("uColor", glm::vec4(0.0f, 0.0f, 0.0f, 0.6f));
	//Recorrer todas las luces
	for (const auto& light : scene.lights) {
		//Ignoraremos la Luz de Punto
		if (light->type == POINT) continue;
		glm::vec3 L = glm::normalize(light->direction);
		if (L.y > -0.001f) continue;
		glm::mat4 shadowProj = glm::mat4(1.0f);
		shadowProj[1][0] = -L.x / L.y;
		shadowProj[1][1] = 0.0f;
		shadowProj[1][2] = -L.z / L.y;
		glm::mat4 moveDown = glm::translate(glm::mat4(1.0f), glm::vec3(0.0f, -groundHeight, 0.0f));
		glm::mat4 moveBackUp = glm::translate(glm::mat4(1.0f), glm::vec3(0.0f, groundHeight + 0.01f, 0.0f));
		glm::mat4 finalShadowMatrix = moveBackUp * shadowProj * moveDown;
		glClear(GL_STENCIL_BUFFER_BIT);
		glStencilFunc(GL_NOTEQUAL, 1, 0xFF);
		glStencilOp(GL_KEEP, GL_KEEP, GL_REPLACE);
		planarShader->SetMatrix4("viewMatrix", camera.view * finalShadowMatrix);
		scene.Draw(*planarShader, camera, true);
	}
	glDisable(GL_STENCIL_TEST);
	glDisable(GL_BLEND);
	glDepthMask(GL_TRUE);
}

void ShadowEngine::RenderMapping(Scene& scene, Camera& camera, ShaderProgram& actualShader) {
	if (scene.lights.empty() || !scene.lights[0]) return;
	//No haremos Luz Puntual
	if (scene.lights[0]->type == POINT) return;
	glm::mat4 lightSpaceMatrix = glm::mat4(1.0f);
	if (scene.lights[0]->type == DIRECTIONAL) {
		glm::vec3 lightDir = glm::normalize(scene.lights[0]->direction);
		glm::vec3 lightPos = -lightDir * 20.0f;
		//Matriz de Espacio de la Luz
		glm::vec3 up = glm::vec3(0.0f, 1.0f, 0.0f);
		if (glm::abs(glm::dot(lightDir, up)) > 0.999f) {
			up = glm::vec3(0.0f, 0.0f, 1.0f);
		}
		glm::mat4 lightView = glm::lookAt(lightPos, glm::vec3(0.0f), up);
		glm::mat4 lightProj = glm::ortho(-10.0f, 10.0f, -10.0f, 10.0f, 1.0f, 100.0f);
		lightSpaceMatrix = lightProj * lightView;
	} else if (scene.lights[0]->type == SPOT) {
		glm::vec3 spotPos = scene.lights[0]->position;
		glm::vec3 spotDir = glm::normalize(scene.lights[0]->direction);
		glm::vec3 spotTarget = spotPos + spotDir;
		float spotAngle = glm::acos(scene.lights[0]->outerCutOff);
		glm::vec3 up = glm::vec3(0.0f, 1.0f, 0.0f);
		if (glm::abs(glm::dot(spotDir, up)) > 0.999f) {
			up = glm::vec3(0.0f, 0.0f, 1.0f);
		}
		glm::mat4 lightView = glm::lookAt(spotPos, spotTarget, up);
		glm::mat4 lightProj = glm::perspective(spotAngle * 2.0f, 1.0f, 0.1f, 50.0f);
		lightSpaceMatrix = lightProj * lightView;
	}
	//Construir Shadow Map
	glViewport(0, 0, shadowRes, shadowRes);
	glBindFramebuffer(GL_FRAMEBUFFER, depthMapFBO);
	glClear(GL_DEPTH_BUFFER_BIT);
	depthShader->Activate();
	depthShader->SetMatrix4("lightSpaceMatrix", lightSpaceMatrix);
	glEnable(GL_CULL_FACE);
	glCullFace(GL_BACK);
	scene.Draw(*depthShader, camera, true);
	glCullFace(GL_BACK);
	glDisable(GL_CULL_FACE);
	glBindFramebuffer(GL_FRAMEBUFFER, 0);
	glViewport(0, 0, camera.width, camera.height);
	actualShader.Activate();
	actualShader.SetMatrix4("uLightSpaceMatrix", lightSpaceMatrix);
	glBindTextureUnit(3, depthMap);
	actualShader.SetInt("uShadowMap", 3);
	actualShader.SetFloat("bias", bias);
	actualShader.SetInt("pcfSize", pcfSize);
	actualShader.SetFloat("pcssSize", pcssSize);
}

void ShadowEngine::RenderVolumen(Scene& scene, Camera& camera, ShaderProgram& actualShader) {

}

ShadowEngine::ShadowEngine(ShadowType type) {
	actualType = type;
	depthShader = std::make_unique<ShaderProgram>("assets/shaders/depth.vert", "assets/shaders/depth.frag");
	planarShader = std::make_unique<ShaderProgram>("assets/shaders/planar.vert", "assets/shaders/planar.frag");
	// ambientShader = std::make_unique<ShaderProgram>("assets/shaders/ambient.vert", "assets/shaders/ambient.frag");
	InitShadowMapping();
}

ShadowEngine::~ShadowEngine() {
	if (depthMapFBO != 0) {
		glDeleteFramebuffers(1, &depthMapFBO);
	}
	if (depthMap != 0) {
		glDeleteTextures(1, &depthMap);
	}
}

void ShadowEngine::RenderSceneWithShadows(Scene& actualScene, Camera& actualCamera, ShaderProgram& actualShader) {
	switch (actualType) {
		case PLANAR:
			RenderPlanar(actualScene, actualCamera, actualShader);
		break;
		case MAPPING:
			RenderMapping(actualScene, actualCamera, actualShader);
		break;
		case VOLUMEN:
			RenderVolumen(actualScene, actualCamera, actualShader);
		break;
	}
}

void ShadowEngine::SetShadowType(ShadowType type) {
	actualType = type;
}
