#include "Node.h"

void Material::Bind(ShaderProgram& shader) {
	shader.SetFloat("uMetallic", metallicFactor);
	shader.SetFloat("uRoughness", roughnessFactor);
	int hasAlbedo = 0;
	int hasNormal = 0;
	for (int i = 0; i < textures.size(); i++) {
		if (textures[i] != nullptr) {
			glBindTextureUnit(i, textures[i]->ID);
			switch (textures[i]->type) {
			case ALBEDO: shader.SetInt("u_albedoMap", i); hasAlbedo = 1; break;
			case NORMAL: shader.SetInt("u_normalMap", i); hasNormal = 1; break;
			case ROUGHNESS: shader.SetInt("u_roughnessMap", i); break;
			case METALLIC: shader.SetInt("u_metallicMap", i); break;
			}
		}
	}
	shader.SetInt("uHasAlbedoMap", hasAlbedo);
	shader.SetInt("uHasNormalMap", hasNormal);
}

Mesh::Mesh(const std::vector<Vertex>& v, const std::vector<GLuint>& i):
	vertices(v), indices(i) {
	UpdateLocalAABB();
	vao = std::make_unique<VAO>();
	vbo = std::make_unique<VBO>(vertices);
	ebo = std::make_unique<EBO>(indices);
	vao->LinkEBO(*ebo);
	//Posiciones 0
	vao->LinkAttrib(*vbo, 0, 3, GL_FLOAT, sizeof(Vertex), 0);
	//Normales 1
	vao->LinkAttrib(*vbo, 1, 3, GL_FLOAT, sizeof(Vertex), offsetof(Vertex, normal));
	//Color 2
	vao->LinkAttrib(*vbo, 2, 3, GL_FLOAT, sizeof(Vertex), offsetof(Vertex, color));
	//Uvs 3
	vao->LinkAttrib(*vbo, 3, 2, GL_FLOAT, sizeof(Vertex), offsetof(Vertex, texCoords));
	//Tangentes 4
	vao->LinkAttrib(*vbo, 4, 3, GL_FLOAT, sizeof(Vertex), offsetof(Vertex, tangent));
	//Bitangentes 5
	vao->LinkAttrib(*vbo, 5, 3, GL_FLOAT, sizeof(Vertex), offsetof(Vertex, biTangent));
}

void Mesh::Draw() {
	vao->Bind();
	glDrawElements(GL_TRIANGLES, static_cast<GLsizei>(indices.size()), GL_UNSIGNED_INT, 0);
	vao->Unbind();
}

void Mesh::UpdateLocalAABB() {
	if (vertices.empty()) {
		aabbMinLocal = glm::vec3(0.0f);
		aabbMaxLocal = glm::vec3(0.0f);
		return;
	}
	//Inicializar la caja con el primer vértice
	aabbMinLocal = vertices[0].position;
	aabbMaxLocal = vertices[0].position;
	//Expandir la caja evaluando cada vértice
	for (const auto& vertex : vertices) {
		aabbMinLocal.x = std::min(aabbMinLocal.x, vertex.position.x);
		aabbMinLocal.y = std::min(aabbMinLocal.y, vertex.position.y);
		aabbMinLocal.z = std::min(aabbMinLocal.z, vertex.position.z);
		aabbMaxLocal.x = std::max(aabbMaxLocal.x, vertex.position.x);
		aabbMaxLocal.y = std::max(aabbMaxLocal.y, vertex.position.y);
		aabbMaxLocal.z = std::max(aabbMaxLocal.z, vertex.position.z);
	}
}

Node::Node(const std::string& nodeName) : name(nodeName) {}

void Node::AddChild(std::unique_ptr<Node> child) {
	child->parent = this;
	children.push_back(std::move(child));
}

void Node::Draw(ShaderProgram& shader, Camera& camera, bool isShadowPass) {
	if (!isVisible) return;
	if (isShadowPass && !castShadows) return;
	//Si este nodo en particular tiene geometría, se dibujara
	if (mesh) {
		//Enviar la matriz de mundo al Vertex Shader
		shader.SetMatrix4("modelMatrix", worldMatrix);
		//Si NO es el pase de sombras, calculamos el PBR y el color
		if (!isShadowPass && material) {
			material->Bind(shader);
			// Uniforme opcional para pintar de otro color si está seleccionado
			shader.SetInt("isSelected", isSelected ? 1 : 0);
			shader.SetInt("uSelectedTriangle", selectedTriangle);
		}
		//Dibujar
		mesh->Draw();
	}
	//Dibujar Hijos
	for (auto& child : children) {
		if (child) {
			child->Draw(shader, camera, isShadowPass);
		}
	}
}

void Node::UpdateWorldMatrix() {
	//Calcular la matriz global
	if (parent != nullptr) {
		worldMatrix = parent->worldMatrix * localMatrix;
	} else {
		worldMatrix = localMatrix;
	}
	//Actualizar el BB
	UpdateWorldAABB();
	//Propagar a los hijos en cascada
	for (auto& child : children) {
		if (child) {
			child->UpdateWorldMatrix();
		}
	}
}

void Node::UpdateWorldAABB() {
	// Si este nodo es solo un grupo vacío sin geometría, su caja es cero
	if (!mesh) {
		aabbMinGlobal = glm::vec3(0.0f);
		aabbMaxGlobal = glm::vec3(0.0f);
		return;
	}
	//Extraer los límites del espacio local de la malla
	glm::vec3 lMin = mesh->aabbMinLocal;
	glm::vec3 lMax = mesh->aabbMaxLocal;
	//Definir los 8 vértices de la caja local
	glm::vec3 corners[8] = {
		glm::vec3(lMin.x, lMin.y, lMin.z),
		glm::vec3(lMax.x, lMin.y, lMin.z),
		glm::vec3(lMin.x, lMax.y, lMin.z),
		glm::vec3(lMax.x, lMax.y, lMin.z),
		glm::vec3(lMin.x, lMin.y, lMax.z),
		glm::vec3(lMax.x, lMin.y, lMax.z),
		glm::vec3(lMin.x, lMax.y, lMax.z),
		glm::vec3(lMax.x, lMax.y, lMax.z)
	};
	//Transformar la primera esquina para inicializar los límites globales
	glm::vec3 transformed = glm::vec3(worldMatrix * glm::vec4(corners[0], 1.0f));
	aabbMinGlobal = transformed;
	aabbMaxGlobal = transformed;
	//Transformar los 7 vértices restantes y expandir la caja dinámicamente
	for (int i = 1; i < 8; i++) {
		transformed = glm::vec3(worldMatrix * glm::vec4(corners[i], 1.0f));
		aabbMinGlobal.x = std::min(aabbMinGlobal.x, transformed.x);
		aabbMinGlobal.y = std::min(aabbMinGlobal.y, transformed.y);
		aabbMinGlobal.z = std::min(aabbMinGlobal.z, transformed.z);
		aabbMaxGlobal.x = std::max(aabbMaxGlobal.x, transformed.x);
		aabbMaxGlobal.y = std::max(aabbMaxGlobal.y, transformed.y);
		aabbMaxGlobal.z = std::max(aabbMaxGlobal.z, transformed.z);
	}
}

bool Mesh::Raycast(const Ray& localRay, RayHit& hit) {
	//Algoritmo de Slab
	glm::vec3 invDir = 1.0f / localRay.direction;
	glm::vec3 t0 = (aabbMinLocal - localRay.origin) * invDir;
	glm::vec3 t1 = (aabbMaxLocal - localRay.origin) * invDir;
	glm::vec3 tmin = glm::min(t0, t1);
	glm::vec3 tmax = glm::max(t0, t1);
	float tNear = glm::max(glm::max(tmin.x, tmin.y), tmin.z);
	float tFar = glm::min(glm::min(tmax.x, tmax.y), tmax.z);
	// Si el rayo pasa de largo
	if (tNear > tFar || tFar < 0.0f) {
		return false;
	}
	bool hitAnything = false;
	float closestDist = std::numeric_limits<float>::infinity();
	const float EPSILON = 0.0000001f;
	//Para recordar qué triángulo gana
	glm::vec3 bestV0, bestV1, bestV2;
	int bestTriangleIndex = -1;
	for (size_t i = 0; i < indices.size(); i += 3) {
		glm::vec3 v0 = vertices[indices[i]].position;
		glm::vec3 v1 = vertices[indices[i + 1]].position;
		glm::vec3 v2 = vertices[indices[i + 2]].position;
		glm::vec3 edge1 = v1 - v0;
		glm::vec3 edge2 = v2 - v0;
		glm::vec3 h = glm::cross(localRay.direction, edge2);
		float a = glm::dot(edge1, h);
		if (a > -EPSILON && a < EPSILON) continue;
		float f = 1.0f / a;
		glm::vec3 s = localRay.origin - v0;
		float u = f * glm::dot(s, h);
		if (u < 0.0f || u > 1.0f) continue;
		glm::vec3 q = glm::cross(s, edge1);
		float v = f * glm::dot(localRay.direction, q);
		if (v < 0.0f || u + v > 1.0f) continue;
		float t = f * glm::dot(edge2, q);
		if (t > EPSILON && t < closestDist) {
			closestDist = t;
			hitAnything = true;
			//Guardamos los vértices del triángulo ganador
			bestV0 = v0; bestV1 = v1; bestV2 = v2;
			bestTriangleIndex = i / 3;
		}
	}
	if (hitAnything) {
		hit.hasHit = true;
		hit.distance = closestDist;
		hit.triV0 = bestV0;
		hit.triV1 = bestV1;
		hit.triV2 = bestV2;
		hit.selectedTriangleIndex = bestTriangleIndex;
		return true;
	}
	return false;
}

bool Node::Raycast(const Ray& worldRay, RayHit& outHit) {
	//BVH
	glm::vec3 invDir = 1.0f / worldRay.direction;
	glm::vec3 t0 = (aabbMinGlobal - worldRay.origin) * invDir;
	glm::vec3 t1 = (aabbMaxGlobal - worldRay.origin) * invDir;
	glm::vec3 tmin = glm::min(t0, t1);
	glm::vec3 tmax = glm::max(t0, t1);
	float tNear = glm::max(glm::max(tmin.x, tmin.y), tmin.z);
	float tFar = glm::min(glm::min(tmax.x, tmax.y), tmax.z);
	// Si el rayo pasa de largo
	if (tNear > tFar || tFar < 0.0f) {
		return false;
	}
	bool hit = false;
	RayHit closestHit;
	closestHit.distance = std::numeric_limits<float>::infinity();
	glm::mat4 invMatrix = glm::inverse(worldMatrix);
	Ray localRay = worldRay;
	localRay.origin = glm::vec3(invMatrix * glm::vec4(worldRay.origin, 1.0f));
	localRay.direction = glm::normalize(glm::vec3(invMatrix * glm::vec4(worldRay.direction, 0.0f)));
	if (mesh) {
		RayHit meshHit;
		if (mesh->Raycast(localRay, meshHit)) {
			closestHit = meshHit;
			closestHit.localNode = this;
			hit = true;
		}
	}
	for (const auto& child : children) {
		RayHit childHit;
		if (child->Raycast(worldRay, childHit)) {
			if (childHit.distance < closestHit.distance) {
				closestHit = childHit;
				hit = true;
			}
		}
	}
	if (hit) {
		outHit = closestHit;
		return true;
	}
	return false;
};