#ifndef SCENE_H
#define SCENE_H

#include "tiny_gltf.h"
#include "stb_image_write.h"
#include <iostream>
#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <glm/geometric.hpp>
#include <vector>
#include <memory>
#include "Node.h"

class Scene {
public:
	std::vector<std::vector<std::shared_ptr<Mesh>>> meshCache;
	std::vector<std::shared_ptr<Material>> materialCache;
	std::vector<std::shared_ptr<Texture>> textureCache;
	std::vector<Camera> cameras;
	std::vector<std::unique_ptr<Node>> rootNodes;
	void LoadFromGLTF(const std::string& filename);
	void SaveToGLTF(const std::string& filename);
	void AddRootNode(std::unique_ptr<Node> node);
	void UpdateScene();
	void Draw(ShaderProgram& shader, Camera& camera, bool isShadowPass);
	RayHit Raycast(Ray& worldRay);
	//Funciones Auxiliares par GLFT
private:
	std::shared_ptr<Texture> LoadTexture(int textureIndex, texType type,tinygltf::Model& model, std::vector<std::shared_ptr<Texture>>& Map);
	bool GetAttributeData(const std::string& attrName, const unsigned char*& outData, int& outStride, tinygltf::Model& model, tinygltf::Primitive& primitive);
	std::unique_ptr<Node> ProcessNode(int nodeIndex, tinygltf::Model& model);
	int ExportNode(Node* node, tinygltf::Model& model, tinygltf::Buffer& mainBuffer);
	int ExportMesh(Mesh* mesh, Material* material, tinygltf::Model& model, tinygltf::Buffer& mainBuffer);
	int ExportMaterial(Material* material, tinygltf::Model& model, tinygltf::Buffer& mainBuffer);
	int ExportTexture(Texture* texture, tinygltf::Model& model, tinygltf::Buffer& mainBuffer);
};

#endif