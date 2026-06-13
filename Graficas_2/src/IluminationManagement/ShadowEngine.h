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
	//Bias y PCF para Shadow Mapping
	float bias = 0.005;
	int pcfSize = 3;
	float pcssSize = 100.0f;
	ShadowEngine(ShadowType type);
	~ShadowEngine();
	void RenderSceneWithShadows(Scene& actualScene, Camera& actualCamera, ShaderProgram& actualShader);
	void SetShadowType(ShadowType type);
};

#endif
