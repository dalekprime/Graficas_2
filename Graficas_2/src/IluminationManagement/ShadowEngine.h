#ifndef  SHADOW_H
#define SHADOW

#include <glad/gl.h>
#include <memory>
#include "../ModelManagement/Scene.h"
#include "../CameraManagement/Camera.h"
#include "../Shader.h"

enum ShadowType {
	PLANAR,
	MAPPING,
	VOLUMEN
};

class ShadowEngine {
private:
	ShadowType actualType = PLANAR;
	GLuint depthMapFBO = 0;
	GLuint depthMap = 0;
	int shadowRes = 2048;
	//Para llevar al espacio de Luz de Shadow Mapping
	glm::mat4 lightSpaceMatrix = glm::mat4(1.0f);
	//Shader para los modelos
	std::unique_ptr<ShaderProgram> planarShader; 
	std::unique_ptr<ShaderProgram> ambientShader;
	std::unique_ptr<ShaderProgram> depthShader;  
	std::unique_ptr<ShaderProgram> volumeShader;
	//Modelos
	void InitShadowMapping();
	void RenderPlanar(Scene& scene, Camera& camera, ShaderProgram& pbrShader);
	void RenderMapping(Scene& scene, Camera& camera, ShaderProgram& pbrShader);
	void RenderVolumen(Scene& scene, Camera& camera, ShaderProgram& pbrShader);
public:
	//Para manejar la altura de las sombras planares
	float groundHeight = -1.0f;
	ShadowEngine(ShadowType type);
	~ShadowEngine();
	void RenderSceneWithShadows(Scene& actualScene, Camera& actualCamera, ShaderProgram& actualShader);
	void SetShadowType(ShadowType type);
};

#endif
