#ifndef SCENE_H
#define SCENE_H

#include "tiny_gltf.h"
#include "stb_image_write.h"
#include <iostream>
#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <glm/geometric.hpp>
#include <glm/gtc/quaternion.hpp>
#include <vector>
#include <memory>
#include "Node.h"
#include "../IluminationManagement/Light.h"
#include <unordered_map>

struct GPUVertex {
    glm::vec4 position;
    glm::vec4 normal;
};

struct GPUMaterial {
    glm::vec4 albedo;
    glm::vec4 properties;
};

struct GPUNode {
    glm::vec4 aabbMin;
    glm::vec4 aabbMax;
};

class Scene {
public:
	std::vector<std::vector<std::shared_ptr<Mesh>>> meshCache;
	std::vector<std::shared_ptr<Material>> materialCache;
	std::vector<std::shared_ptr<Texture>> textureCache;
	std::vector<std::unique_ptr<Node>> rootNodes;
	std::vector<Camera> cameras;
	std::vector<std::shared_ptr<Light>> lights;
	void AddRootNode(std::unique_ptr<Node> node);
	void UpdateScene();
	void Draw(ShaderProgram& shader, Camera& camera, bool isShadowPass);
	void RenderRaytraced(ShaderProgram& shader, Camera& camera, const std::vector<std::shared_ptr<Light>>& lights);
	void DrawLightGizmos(ShaderProgram& shaderProgram);
	RayHit Raycast(Ray& worldRay);
	void SaveToGLTF(const std::string& filename, Camera* currentCamera = nullptr);
	void LoadFromGLTF(const std::string& filename);

    GLuint rtVertexSSBO = 0;
    GLuint rtIndexSSBO = 0;
    GLuint rtMaterialSSBO = 0;
    GLuint rtNodeSSBO = 0;
    GLuint fullscreenVAO = 0;
    GLuint fullscreenVBO = 0;
private:
	//Funciones Auxiliares par GLFT
	std::shared_ptr<Texture> LoadTexture(int textureIndex, texType type,tinygltf::Model& model, std::vector<std::shared_ptr<Texture>>& Map, const std::string& basePath);
	bool GetAttributeData(const std::string& attrName, const unsigned char*& outData, int& outStride, tinygltf::Model& model, tinygltf::Primitive& primitive);
	std::unique_ptr<Node> ProcessNode(int nodeIndex, tinygltf::Model& model);
	int ExportNode(Node* node, tinygltf::Model& model, tinygltf::Buffer& mainBuffer);
	int ExportMesh(Mesh* mesh, Material* material, tinygltf::Model& model, tinygltf::Buffer& mainBuffer);
	int ExportMaterial(Material* material, tinygltf::Model& model, tinygltf::Buffer& mainBuffer);
	int ExportTexture(Texture* texture, tinygltf::Model& model, tinygltf::Buffer& mainBuffer);
};

#endif