#ifndef NODE_H
#define NODE_H

#include <glm/glm.hpp>
#include <vector>
#include <memory>
#include <algorithm>
#include <glm/gtc/type_ptr.hpp>
#include "../BuffersManagement/VAO.h"
#include "../BuffersManagement/VBO.h"
#include "../BuffersManagement/EBO.h"
#include "../CameraManagement/Camera.h"
#include "../CameraManagement/Ray.h"
#include "texture.h"
#include <limits>

//Gestionara el Material y las Texturas
class Material {
public:
	std::shared_ptr<Texture> albedoMap;
	std::shared_ptr<Texture> normalMap;
	std::shared_ptr<Texture> pbrMap;
	float metallicFactor = 0.0f;
	float roughnessFactor = 1.0f;
	float aoFactor = 1.0f;
	glm::vec4 baseColorFactor = glm::vec4(1.0f);
	void Bind(ShaderProgram& shader);
};

//El mallado crudo
class Mesh {
public:
	std::unique_ptr<VAO> vao;
	std::unique_ptr<VBO> vbo;
	std::unique_ptr<EBO> ebo;
	std::vector<Vertex> vertices;
	std::vector<GLuint> indices;
	glm::vec3 aabbMinLocal = glm::vec3(0.0f);
	glm::vec3 aabbMaxLocal = glm::vec3(0.0f);

	Mesh(const std::vector<Vertex>& v, const std::vector<GLuint>& i);
	void Draw();
	void UpdateLocalAABB();
	bool Raycast(const Ray& localRay, RayHit& hit);
};

//Jerarquia
class Node {
public:
	std::string name = "";
	Node(const std::string& nodeName);
	//Elemento Visuales
	std::shared_ptr<Mesh> mesh;
	std::shared_ptr<Material> material;
	//Jerarquia de Escena
	Node* parent = nullptr;
	std::vector<std::unique_ptr<Node>> children;
	//Geometria
	glm::mat4 localMatrix = glm::mat4(1.0f);
	glm::mat4 worldMatrix = glm::mat4(1.0f);
	glm::vec3 aabbMinGlobal = glm::vec3(0.0f);
	glm::vec3 aabbMaxGlobal = glm::vec3(0.0f);
	//Estado
	bool isVisible = true;
	bool castShadows = true;
	bool isSelected = false;
	int selectedTriangle = -1;
	int uvMappingMode = 0; // 0 = Original, 1 = Cilíndrico, 2 = Esférico
	// Estado de Selección
	void AddChild(std::unique_ptr<Node> child);
	void Draw(ShaderProgram& shader, Camera& camera, bool isShadowPass = false, bool parentSelected = false);
	void UpdateWorldMatrix();
	void UpdateWorldAABB();
	bool Raycast(const Ray& worldRay, RayHit& outHit);
};

#endif
