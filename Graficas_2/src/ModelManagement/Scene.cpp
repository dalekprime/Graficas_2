#include "Scene.h"
#define TINYGLTF_IMPLEMENTATION
#define STB_IMAGE_WRITE_IMPLEMENTATION

void Scene::AddRootNode(std::unique_ptr<Node> node) {
    rootNodes.push_back(std::move(node));
}

void Scene::UpdateScene() {
    for (auto& node : rootNodes) {
        if (node) {
            node->UpdateWorldMatrix();
        }
    }
}

void Scene::Draw(ShaderProgram& shader, Camera& camera, bool isShadowPass) {
    for (auto& node : rootNodes) {
        if (node) {
            node->Draw(shader, camera, isShadowPass);
        }
    }
}

void Scene::RenderRaytraced(ShaderProgram& shader, Camera& camera, const std::vector<std::shared_ptr<Light>>& lights) {
    std::vector<GPUVertex> gpuVertices;
    std::vector<GLuint> gpuIndices;
    std::vector<GPUMaterial> gpuMaterials;
    std::vector<GPUNode> gpuNodes;

    std::unordered_map<Material*, int> matMap;

    auto ProcessNodeForRT = [&](Node* node, auto& self) -> void {
        if (!node) return;
        if (node->mesh) {
            int matID = 0;
            if (node->material) {
                if (matMap.find(node->material.get()) == matMap.end()) {
                    matID = gpuMaterials.size();
                    matMap[node->material.get()] = matID;
                    GPUMaterial m;
                    m.albedo = node->material->baseColorFactor;
                    int albedoTexIdx = -1;
                    if (node->material->albedoMap) {
                        auto it = std::find(textureCache.begin(), textureCache.end(), node->material->albedoMap);
                        if (it != textureCache.end()) albedoTexIdx = std::distance(textureCache.begin(), it);
                    }
                    m.texIndices = glm::vec4((float)albedoTexIdx, -1.0f, -1.0f, -1.0f);

                    if (glm::length(glm::vec3(m.albedo)) < 0.01f) {
                        m.albedo = glm::vec4(0.8f, 0.8f, 0.8f, 1.0f); // Default grey for textures without base color
                    }
                    m.properties.x = node->material->roughnessFactor;
                    m.properties.y = node->material->metallicFactor;
                    m.properties.z = 1.5f; 
                    if (node->material->baseColorFactor.a < 0.9f) {
                        m.properties.w = 2.0f;
                    }
                    else {
                        m.properties.w = 0.0f; 
                    }
                    // No convertir texturas con alpha mask en cristal por defecto
                    // if (node->material->baseColorFactor.a < 0.9f) m.properties.w = 2.0f; 
                    gpuMaterials.push_back(m);
                } else {
                    matID = matMap[node->material.get()];
                }
            } else {
                if (matMap.find(nullptr) == matMap.end()) {
                    matID = gpuMaterials.size();
                    matMap[nullptr] = matID;
                    GPUMaterial m;
                    m.albedo = glm::vec4(1.0f);
                    m.properties = glm::vec4(1.0f, 0.0f, 1.5f, 0.0f);
                    gpuMaterials.push_back(m);
                } else {
                    matID = matMap[nullptr];
                }
            }

            int baseVertex = gpuVertices.size();
            for (const auto& v : node->mesh->vertices) {
                GPUVertex gv;
                glm::vec4 worldPos = node->worldMatrix * glm::vec4(v.position, 1.0f);
                gv.position = glm::vec4(worldPos.x, worldPos.y, worldPos.z, (float)matID);
                glm::mat3 normalMatrix = glm::transpose(glm::inverse(glm::mat3(node->worldMatrix)));
                glm::vec3 worldNormal = glm::normalize(normalMatrix * v.normal);
                gv.normal = glm::vec4(worldNormal.x, worldNormal.y, worldNormal.z, v.texCoords.x);
                gv.extra = glm::vec4(v.texCoords.y, 0.0f, 0.0f, 0.0f);
                gpuVertices.push_back(gv);
            }
            int baseIndex = gpuIndices.size();
            for (const auto& idx : node->mesh->indices) {
                gpuIndices.push_back(baseVertex + idx);
            }
            int numIndices = gpuIndices.size() - baseIndex;
            
            // Calcular AABB local del nodo para optimizacion BVH
            glm::vec3 nMin(99999.0f);
            glm::vec3 nMax(-99999.0f);
            for (const auto& v : node->mesh->vertices) {
                glm::vec3 worldPos = glm::vec3(node->worldMatrix * glm::vec4(v.position, 1.0f));
                nMin = glm::min(nMin, worldPos);
                nMax = glm::max(nMax, worldPos);
            }
            // Expandir un poco para precision
            nMin -= glm::vec3(0.01f);
            nMax += glm::vec3(0.01f);
            
            GPUNode gn;
            gn.aabbMin = glm::vec4(nMin, (float)baseIndex);
            gn.aabbMax = glm::vec4(nMax, (float)numIndices);
            gpuNodes.push_back(gn);
        }
        for (const auto& child : node->children) {
            self(child.get(), self);
        }
    };

    for (const auto& rootNode : rootNodes) {
        ProcessNodeForRT(rootNode.get(), ProcessNodeForRT);
    }

    if (gpuVertices.empty()) return; 

    if (rtVertexSSBO == 0) glGenBuffers(1, &rtVertexSSBO);
    if (rtIndexSSBO == 0) glGenBuffers(1, &rtIndexSSBO);
    if (rtMaterialSSBO == 0) glGenBuffers(1, &rtMaterialSSBO);
    if (rtNodeSSBO == 0) glGenBuffers(1, &rtNodeSSBO);

    glBindBuffer(GL_SHADER_STORAGE_BUFFER, rtVertexSSBO);
    glBufferData(GL_SHADER_STORAGE_BUFFER, gpuVertices.size() * sizeof(GPUVertex), gpuVertices.data(), GL_STATIC_DRAW);
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, rtVertexSSBO);

    glBindBuffer(GL_SHADER_STORAGE_BUFFER, rtIndexSSBO);
    glBufferData(GL_SHADER_STORAGE_BUFFER, gpuIndices.size() * sizeof(GLuint), gpuIndices.data(), GL_STATIC_DRAW);
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 1, rtIndexSSBO);

    glBindBuffer(GL_SHADER_STORAGE_BUFFER, rtMaterialSSBO);
    glBufferData(GL_SHADER_STORAGE_BUFFER, gpuMaterials.size() * sizeof(GPUMaterial), gpuMaterials.data(), GL_STATIC_DRAW);
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 2, rtMaterialSSBO);

    glBindBuffer(GL_SHADER_STORAGE_BUFFER, rtNodeSSBO);
    glBufferData(GL_SHADER_STORAGE_BUFFER, gpuNodes.size() * sizeof(GPUNode), gpuNodes.data(), GL_STATIC_DRAW);
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 3, rtNodeSSBO);

    // Calcular AABB de la escena
    glm::vec3 aabbMin(99999.0f);
    glm::vec3 aabbMax(-99999.0f);
    for (const auto& v : gpuVertices) {
        aabbMin = glm::min(aabbMin, glm::vec3(v.position));
        aabbMax = glm::max(aabbMax, glm::vec3(v.position));
    }
    // Expandir un poco el AABB para evitar problemas de precision
    aabbMin -= glm::vec3(0.1f);
    aabbMax += glm::vec3(0.1f);

    shader.Activate();
    shader.SetVec3("uSceneMin", aabbMin);
    shader.SetVec3("uSceneMax", aabbMax);
    shader.SetVec3("viewPos", camera.position);
    shader.SetMatrix4("uInvView", glm::inverse(camera.view));
    shader.SetMatrix4("uInvProj", glm::inverse(camera.projection));
    shader.SetVec2("uResolution", glm::vec2((float)camera.width, (float)camera.height));
    shader.SetInt("uMaxBounces", rtMaxBounces);
    shader.SetVec3("uAmbientColor", glm::vec3(0.05f));
    shader.SetInt("uNumIndices", (int)gpuIndices.size());
    shader.SetInt("uNumNodes", (int)gpuNodes.size());
    for (size_t i = 0; i < textureCache.size() && i < 16; i++) {
        textureCache[i]->Bind(i + 5); 
        shader.SetInt(("uTextures[" + std::to_string(i) + "]").c_str(), i + 5);
    }

    shader.SetInt("uNumLights", (int)lights.size());
    for (size_t i = 0; i < lights.size(); ++i) {
        auto& light = lights[i];
        std::string baseName = "uLights[" + std::to_string(i) + "].";
        shader.SetInt((baseName + "type").c_str(), light->type);
        if (light->type == DIRECTIONAL) {
            shader.SetVec3((baseName + "position").c_str(), -glm::normalize(light->direction) * 1000.0f);
        }
        else {
            shader.SetVec3((baseName + "position").c_str(), light->position);
        }
        shader.SetVec3((baseName + "direction").c_str(), glm::normalize(light->direction));
        shader.SetVec3((baseName + "color").c_str(), light->color);
        shader.SetFloat((baseName + "intensity").c_str(), light->intensity);
    }

    if (fullscreenVAO == 0) {
        float quadVertices[] = {
            -1.0f,  1.0f, 0.0f,
            -1.0f, -1.0f, 0.0f,
             1.0f, -1.0f, 0.0f,
            -1.0f,  1.0f, 0.0f,
             1.0f, -1.0f, 0.0f,
             1.0f,  1.0f, 0.0f
        };
        glGenVertexArrays(1, &fullscreenVAO);
        glGenBuffers(1, &fullscreenVBO);
        glBindVertexArray(fullscreenVAO);
        glBindBuffer(GL_ARRAY_BUFFER, fullscreenVBO);
        glBufferData(GL_ARRAY_BUFFER, sizeof(quadVertices), &quadVertices, GL_STATIC_DRAW);
        glEnableVertexAttribArray(0);
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
    }

    glBindVertexArray(fullscreenVAO);
    glDrawArrays(GL_TRIANGLES, 0, 6);
    glBindVertexArray(0);
}

std::shared_ptr<Texture> Scene::LoadTexture(int textureIndex, texType type, 
                    tinygltf::Model& model, std::vector<std::shared_ptr<Texture>>& Map, const std::string& basePath) {
    if (textureIndex < 0) return nullptr;
    if (Map[textureIndex] != nullptr) return Map[textureIndex];
    int imageIndex = model.textures[textureIndex].source;
    const tinygltf::Image& gltfImage = model.images[imageIndex];
    
    std::shared_ptr<Texture> newTex;
    // Si hay datos en memoria (empaquetado en el glb o decodificados por tinygltf), usamos los datos binarios
    if (!gltfImage.image.empty()) {
        // Cargar desde la memoria del archivo binario glb
        newTex = std::make_shared<Texture>(gltfImage.image.data(), gltfImage.width, gltfImage.height, gltfImage.component, type);
    }
    // Si la imagen tiene un URI valido (no vacio y no es data en base64) y no estaba en memoria, lo cargamos desde disco
    else if (!gltfImage.uri.empty() && gltfImage.uri.find("data:") != 0) {
        // Cargar fisicamente desde el disco
        std::string fullPath = basePath + gltfImage.uri;
        std::cout << fullPath << std::endl;
        newTex = std::make_shared<Texture>(fullPath.c_str(), type, 0, GL_RGB);
    } 
    else {
        return nullptr;
    }
    
    newTex->path = gltfImage.uri;
    Map[textureIndex] = newTex;
    textureCache.push_back(newTex);
    return newTex;
}

bool Scene::GetAttributeData(const std::string& attrName, const unsigned char*& outData, 
                    int& outStride, tinygltf::Model& model, tinygltf::Primitive& primitive) {
    auto actualPrimitive = primitive.attributes.find(attrName);
    if (actualPrimitive == primitive.attributes.end()) return false;
    const tinygltf::Accessor& accessor = model.accessors[actualPrimitive->second];
    const tinygltf::BufferView& bufferView = model.bufferViews[accessor.bufferView];
    const tinygltf::Buffer& buffer = model.buffers[bufferView.buffer];
    outData = &buffer.data[bufferView.byteOffset + accessor.byteOffset];
    outStride = bufferView.byteStride == 0 ?
        (tinygltf::GetComponentSizeInBytes(accessor.componentType) * tinygltf::GetNumComponentsInType(accessor.type)) :
        bufferView.byteStride;
    return true;
}

std::unique_ptr<Node> Scene::ProcessNode(int nodeIndex, tinygltf::Model& model) {
    const tinygltf::Node& gltfNode = model.nodes[nodeIndex];
    auto newNode = std::make_unique<Node>(gltfNode.name.empty() ? "Node_" + std::to_string(nodeIndex) : gltfNode.name);
    //Matriz de Transfomacion
    if (gltfNode.matrix.size() == 16) {
        newNode->localMatrix = glm::make_mat4(gltfNode.matrix.data());
    } else {
        //Esto es por si esta separada la matriz model
        glm::mat4 t = glm::mat4(1.0f);
        glm::mat4 r = glm::mat4(1.0f);
        glm::mat4 s = glm::mat4(1.0f);
        if (gltfNode.translation.size() == 3) {
            t = glm::translate(t, glm::vec3(gltfNode.translation[0], gltfNode.translation[1], gltfNode.translation[2]));
        }
        if (gltfNode.rotation.size() == 4) {
            glm::quat q = glm::make_quat(gltfNode.rotation.data());
            r = glm::mat4_cast(q);
        }
        if (gltfNode.scale.size() == 3)
            s = glm::scale(s, glm::vec3(gltfNode.scale[0], gltfNode.scale[1], gltfNode.scale[2]));
        newNode->localMatrix = t * r * s;
    }
    //Asignar Geometría y Material
    if (gltfNode.mesh > -1) {
        const auto& primitives = meshCache[gltfNode.mesh];
        const tinygltf::Mesh& originalGltfMesh = model.meshes[gltfNode.mesh];
        if (primitives.size() == 1) {
            // Si el objeto es simple
            newNode->mesh = primitives[0];
            int matIdx = originalGltfMesh.primitives[0].material;
            if (matIdx > -1) newNode->material = materialCache[matIdx];
        } else {
            // Si el objeto es complejo
            for (size_t i = 0; i < primitives.size(); i++) {
                auto subNode = std::make_unique<Node>(newNode->name + "_part" + std::to_string(i));
                subNode->mesh = primitives[i];
                int matIdx = originalGltfMesh.primitives[i].material;
                if (matIdx > -1) subNode->material = materialCache[matIdx];
                newNode->AddChild(std::move(subNode));
            }
        }
    }
    //Procesar hijos
    for (int childIdx : gltfNode.children) {
        newNode->AddChild(ProcessNode(childIdx, model));
    }
    return newNode;
}

void Scene::LoadFromGLTF(const std::string& filename) {
    tinygltf::Model model;
    tinygltf::TinyGLTF loader;
    std::string err, warn;
    bool ret = false;
    //Detectar si el archivo es Binario (.glb) o Texto (.gltf)
    if (filename.find(".glb") != std::string::npos) {
        ret = loader.LoadBinaryFromFile(&model, &err, &warn, filename);
    } else {
        ret = loader.LoadASCIIFromFile(&model, &err, &warn, filename);
    }
    if (!ret) {
        throw std::runtime_error("GLTF Load Error: " + err);
    }
    //Extraer el basePath
    std::string basePath = "";
    size_t lastSlash = filename.find_last_of("/\\");
    if (lastSlash != std::string::npos) {
        basePath = filename.substr(0, lastSlash + 1);
    }
    //Cargar de Texturas
    std::vector<std::shared_ptr<Texture>> loadedTexturesMap(model.textures.size(), nullptr);
    for (auto& gltfMat : model.materials) {
        auto actualMaterial = std::make_shared<Material>();
        //PBR Global
        actualMaterial->roughnessFactor = gltfMat.pbrMetallicRoughness.roughnessFactor;
        actualMaterial->metallicFactor = gltfMat.pbrMetallicRoughness.metallicFactor;
        if (gltfMat.pbrMetallicRoughness.baseColorFactor.size() == 4) {
            actualMaterial->baseColorFactor = glm::vec4(
                gltfMat.pbrMetallicRoughness.baseColorFactor[0],
                gltfMat.pbrMetallicRoughness.baseColorFactor[1],
                gltfMat.pbrMetallicRoughness.baseColorFactor[2],
                gltfMat.pbrMetallicRoughness.baseColorFactor[3]
            );
        }
        //Albedo texture
        int albedoIdx = gltfMat.pbrMetallicRoughness.baseColorTexture.index;
        if (albedoIdx > -1) {
            // Se extrae del binario y se etiqueta
            std::shared_ptr<Texture> albedoTex = LoadTexture(albedoIdx, ALBEDO, model, loadedTexturesMap, basePath);
            actualMaterial->albedoMap = albedoTex;
        }
        //Normal texture
        int normalIdx = gltfMat.normalTexture.index;
        if (normalIdx > -1) {
            // Se extrae del binario y se etiqueta
            std::shared_ptr<Texture> normalTex = LoadTexture(normalIdx, NORMAL, model, loadedTexturesMap, basePath);
            actualMaterial->normalMap = normalTex;
        }
        //PBR texture
        int pbrIdx = gltfMat.pbrMetallicRoughness.metallicRoughnessTexture.index;
        if (pbrIdx > -1) {
            // Se extrae del binario y se etiqueta
            std::shared_ptr<Texture> pbrTex = LoadTexture(pbrIdx, PBR, model, loadedTexturesMap, basePath);
            actualMaterial->pbrMap = pbrTex;
        }
        materialCache.push_back(actualMaterial);
    }
    //Carga de Meshes
    for (auto& gltfMesh : model.meshes) {
        std::vector<std::shared_ptr<Mesh>> primitiveList;
        for (auto& primitive : gltfMesh.primitives) {
            std::vector<Vertex> vertices;
            std::vector<GLuint> indices;
            //Base
            auto posIt = primitive.attributes.find("POSITION");
            const unsigned char* dataPtr = nullptr;
            int stride = 0;
            if (posIt == primitive.attributes.end()) continue;
            const tinygltf::Accessor& posAccessor = model.accessors[posIt->second];
            vertices.resize(posAccessor.count);
            //Posiciones
            if (GetAttributeData("POSITION", dataPtr, stride, model, primitive)) {
                for (size_t i = 0; i < vertices.size(); i++) {
                    const float* ptr = reinterpret_cast<const float*>(dataPtr + i * stride);
                    vertices[i].position = glm::vec3(ptr[0], ptr[1], ptr[2]);
                    //Inicializar el resto
                    vertices[i].normal = glm::vec3(0.0f);
                    vertices[i].color = glm::vec3(1.0f);
                    vertices[i].texCoords = glm::vec2(0.0f);
                    vertices[i].tangent = glm::vec3(0.0f);
                    vertices[i].biTangent = glm::vec3(0.0f);
                }
            }
            //Normal
            if (GetAttributeData("NORMAL", dataPtr, stride, model, primitive)) {
                for (size_t i = 0; i < vertices.size(); i++) {
                    const float* ptr = reinterpret_cast<const float*>(dataPtr + i * stride);
                    vertices[i].normal = glm::vec3(ptr[0], ptr[1], ptr[2]);
                }
            }
            //Color
            if (GetAttributeData("COLOR_0", dataPtr, stride, model, primitive)) {
                for (size_t i = 0; i < vertices.size(); i++) {
                    const float* ptr = reinterpret_cast<const float*>(dataPtr + i * stride);
                    vertices[i].color = glm::vec3(ptr[0], ptr[1], ptr[2]);
                }
            }
            //UVs
            auto uvIt = primitive.attributes.find("TEXCOORD_0");
            if (uvIt != primitive.attributes.end()) {
                const tinygltf::Accessor& uvAcc = model.accessors[uvIt->second];
                if (GetAttributeData("TEXCOORD_0", dataPtr, stride, model, primitive)) {
                    for (size_t i = 0; i < vertices.size(); i++) {
                        if (uvAcc.componentType == TINYGLTF_COMPONENT_TYPE_FLOAT) {
                            const float* ptr = reinterpret_cast<const float*>(dataPtr + i * stride);
                            vertices[i].texCoords = glm::vec2(ptr[0], ptr[1]);
                        } else if (uvAcc.componentType == TINYGLTF_COMPONENT_TYPE_UNSIGNED_BYTE) {
                            const uint8_t* ptr = reinterpret_cast<const uint8_t*>(dataPtr + i * stride);
                            vertices[i].texCoords = glm::vec2(ptr[0] / 255.0f, ptr[1] / 255.0f);
                        } else if (uvAcc.componentType == TINYGLTF_COMPONENT_TYPE_UNSIGNED_SHORT) {
                            const uint16_t* ptr = reinterpret_cast<const uint16_t*>(dataPtr + i * stride);
                            vertices[i].texCoords = glm::vec2(ptr[0] / 65535.0f, ptr[1] / 65535.0f);
                        }
                    }
                }
            }
            //Tangentes y Bitangentes
            bool hasTangents = false;
            if (GetAttributeData("TANGENT", dataPtr, stride, model, primitive)) {
                hasTangents = true;
                for (size_t i = 0; i < vertices.size(); i++) {
                    const float* ptr = reinterpret_cast<const float*>(dataPtr + i * stride);
                    vertices[i].tangent = glm::vec3(ptr[0], ptr[1], ptr[2]);
                    float w = ptr[3];
                    //Generar la Bitangente local
                    vertices[i].biTangent = glm::cross(vertices[i].normal, vertices[i].tangent) * w;
                }
            }
            //Indices
            if (primitive.indices > -1) {
                const tinygltf::Accessor& indAccessor = model.accessors[primitive.indices];
                const tinygltf::BufferView& indView = model.bufferViews[indAccessor.bufferView];
                const tinygltf::Buffer& indBuffer = model.buffers[indView.buffer];
                const unsigned char* indData = &indBuffer.data[indView.byteOffset + indAccessor.byteOffset];
                indices.resize(indAccessor.count);
                if (indAccessor.componentType == TINYGLTF_COMPONENT_TYPE_UNSIGNED_INT) {
                    const uint32_t* buf = reinterpret_cast<const uint32_t*>(indData);
                    for (size_t i = 0; i < indAccessor.count; i++) indices[i] = buf[i];
                } else if (indAccessor.componentType == TINYGLTF_COMPONENT_TYPE_UNSIGNED_SHORT) {
                    const uint16_t* buf = reinterpret_cast<const uint16_t*>(indData);
                    for (size_t i = 0; i < indAccessor.count; i++) indices[i] = buf[i];
                } else if (indAccessor.componentType == TINYGLTF_COMPONENT_TYPE_UNSIGNED_BYTE) {
                    const uint8_t* buf = reinterpret_cast<const uint8_t*>(indData);
                    for (size_t i = 0; i < indAccessor.count; i++) indices[i] = buf[i];
                }
            }
            //Cálculo manual de Tangentes si el archivo glTF no las incluye
            if (!hasTangents && indices.size() > 0) {
                for (size_t i = 0; i + 2 < indices.size(); i += 3) {
                    Vertex& v0 = vertices[indices[i]];
                    Vertex& v1 = vertices[indices[i+1]];
                    Vertex& v2 = vertices[indices[i+2]];
                    glm::vec3 edge1 = v1.position - v0.position;
                    glm::vec3 edge2 = v2.position - v0.position;
                    glm::vec2 deltaUV1 = v1.texCoords - v0.texCoords;
                    glm::vec2 deltaUV2 = v2.texCoords - v0.texCoords;
                    float f = 1.0f / (deltaUV1.x * deltaUV2.y - deltaUV2.x * deltaUV1.y);
                    if (std::isinf(f) || std::isnan(f)) f = 0.0f; // Evitar division por cero
                    glm::vec3 tangent;
                    tangent.x = f * (deltaUV2.y * edge1.x - deltaUV1.y * edge2.x);
                    tangent.y = f * (deltaUV2.y * edge1.y - deltaUV1.y * edge2.y);
                    tangent.z = f * (deltaUV2.y * edge1.z - deltaUV1.y * edge2.z);
                    v0.tangent += tangent;
                    v1.tangent += tangent;
                    v2.tangent += tangent;
                }
                for (auto& v : vertices) {
                    if (glm::length(v.tangent) > 0.0001f) {
                        v.tangent = glm::normalize(v.tangent);
                    } else {
                        // Fallback si no hay UVs válidas para este vértice
                        v.tangent = glm::vec3(1.0f, 0.0f, 0.0f);
                    }
                    // Ortogonalizar usando Gram-Schmidt
                    v.tangent = glm::normalize(v.tangent - v.normal * glm::dot(v.normal, v.tangent));
                    v.biTangent = glm::cross(v.normal, v.tangent);
                }
            }
            auto actualMesh = std::make_shared<Mesh>(vertices, indices);
            primitiveList.push_back(actualMesh);
        }
        meshCache.push_back(primitiveList);
    }
    //Nodos
    const tinygltf::Scene& gltfScene = model.scenes[model.defaultScene > -1 ? model.defaultScene : 0];
    for (int rootNodeIdx : gltfScene.nodes) {
        this->AddRootNode(ProcessNode(rootNodeIdx, model));
    }
    for (auto& rootNode : rootNodes) {
        rootNode->UpdateWorldMatrix();
    }
    //Camaras
    for (size_t i = 0; i < model.nodes.size(); ++i) {
        if (model.nodes[i].camera != -1) {
            const tinygltf::Camera& gltfCam = model.cameras[model.nodes[i].camera];
            //Instanciar con dimensiones por defecto
            Camera newCam(1280, 720, glm::vec3(0.0f), glm::vec3(0.0f, 0.0f, -1.0f), glm::vec3(0.0f, 1.0f, 0.0f));
            if (gltfCam.type == "perspective") {
                newCam.fov = glm::degrees(gltfCam.perspective.yfov);
                newCam.znear = gltfCam.perspective.znear;
                newCam.zfar = gltfCam.perspective.zfar > 0.0f ? gltfCam.perspective.zfar : 100.0f;
            }
            if (gltfCam.extras.IsObject() && gltfCam.extras.Has("cameraType")) {
                newCam.type = static_cast<CameraType>(gltfCam.extras.Get("cameraType").Get<int>());
            }
            //Función lambda para buscar el nodo dueño de esta cámara
            std::function<Node* (Node*, const std::string&)> FindNode = [&](Node* n, const std::string& name) -> Node* {
                if (n->name == name) return n;
                for (auto& child : n->children) {
                    Node* found = FindNode(child.get(), name);
                    if (found) return found;
                }
                return nullptr;
                };
            //El nombre del nodo
            std::string targetName = model.nodes[i].name.empty() ? "Node_" + std::to_string(i) : model.nodes[i].name;
            Node* camNode = nullptr;
            for (auto& root : rootNodes) {
                camNode = FindNode(root.get(), targetName);
                if (camNode) break;
            }
            //Si encontramos el nodo, extraemos su posición y orientación global
            if (camNode) {
                newCam.position = glm::vec3(camNode->worldMatrix[3]);
                newCam.orientation = glm::normalize(glm::vec3(-camNode->worldMatrix[2]));
                newCam.up = glm::normalize(glm::vec3(camNode->worldMatrix[1])); 
            }
            this->cameras.push_back(newCam);
        }
    }
    std::cout << "Escena cargada: " << filename << std::endl;
};

void Scene::SaveToGLTF(const std::string& filename, Camera* currentCamera) {
    tinygltf::Model model;
    //Configurar metadatos
    model.asset.version = "2.0";
    model.asset.generator = "Graficas_2_Exporter";
    //Crear la escena por defecto dentro del archivo
    tinygltf::Scene gltfScene;
    gltfScene.name = "Escena_Exportada";
    //Un único buffer para toda la geometría de la escena
    tinygltf::Buffer mainBuffer;
    //Recorrer los nodos raíz de nuestro grafo de escena
    for (const auto& rootNode : rootNodes) {
        if (rootNode) {
            int rootIdx = ExportNode(rootNode.get(), model, mainBuffer);
            gltfScene.nodes.push_back(rootIdx);
        }
    }
    //Camaras
    for (size_t i = 0; i < cameras.size(); ++i) {
        tinygltf::Camera gltfCam;
        gltfCam.type = "perspective";
        gltfCam.perspective.yfov = glm::radians(cameras[i].fov);
        gltfCam.perspective.znear = cameras[i].znear;
        gltfCam.perspective.zfar = cameras[i].zfar;
        gltfCam.perspective.aspectRatio = (float)cameras[i].width / (float)cameras[i].height;
        
        tinygltf::Value::Object extras;
        extras["cameraType"] = tinygltf::Value(static_cast<int>(cameras[i].type));
        gltfCam.extras = tinygltf::Value(extras);
        
        model.cameras.push_back(gltfCam);
        tinygltf::Node camNode;
        camNode.name = "CameraNode_" + std::to_string(i);
        camNode.camera = static_cast<int>(i);
        glm::mat4 view = glm::lookAt(cameras[i].position, cameras[i].position + cameras[i].orientation, cameras[i].up);
        glm::mat4 invView = glm::inverse(view);
        const float* matrixPtr = glm::value_ptr(invView);
        camNode.matrix = std::vector<double>(matrixPtr, matrixPtr + 16);
        model.nodes.push_back(camNode);
        gltfScene.nodes.push_back(static_cast<int>(model.nodes.size() - 1));
    }
    
    // Export Camera State si existe
    if (currentCamera) {
        tinygltf::Camera gltfCam;
        gltfCam.type = "perspective";
        gltfCam.perspective.yfov = glm::radians(currentCamera->fov);
        gltfCam.perspective.znear = currentCamera->znear;
        gltfCam.perspective.zfar = currentCamera->zfar;
        
        tinygltf::Value::Object extras;
        extras["cameraType"] = tinygltf::Value(static_cast<int>(currentCamera->type));
        gltfCam.extras = tinygltf::Value(extras);
        
        model.cameras.push_back(gltfCam);
        
        tinygltf::Node camNode;
        camNode.camera = static_cast<int>(model.cameras.size() - 1);
        camNode.name = "SavedCamera";
        camNode.translation = { currentCamera->position.x, currentCamera->position.y, currentCamera->position.z };
        
        // Calcular Quaternion desde la Orientacion y el Up
        glm::mat4 viewMatrix = glm::lookAt(currentCamera->position, currentCamera->position + currentCamera->orientation, currentCamera->up);
        // La matriz view transforma de mundo a cámara. La rotación del nodo glTF es de cámara a mundo (inversa de view)
        glm::mat4 camToWorld = glm::inverse(viewMatrix);
        glm::quat q = glm::quat_cast(camToWorld);
        camNode.rotation = { q.x, q.y, q.z, q.w };
        
        model.nodes.push_back(camNode);
        gltfScene.nodes.push_back(static_cast<int>(model.nodes.size() - 1));
    }
    
    //Vincular la escena y el buffer principal al modelo
    model.scenes.push_back(gltfScene);
    model.defaultScene = 0;
    model.buffers.push_back(mainBuffer);
    //Detectar formato por extensión
    tinygltf::TinyGLTF writer;
    bool isGlb = (filename.find(".glb") != std::string::npos);
    //Parámetros
    bool success = writer.WriteGltfSceneToFile(&model, filename, true, true, true, isGlb);
    if (success) {
        std::cout << "Escena exportada correctamente a: " << filename << std::endl;
    } else {
        std::cerr << "Error crítico: No se pudo guardar el archivo glTF." << std::endl;
    }
};

int Scene::ExportNode(Node* node, tinygltf::Model& model, tinygltf::Buffer& mainBuffer) {
    tinygltf::Node gltfNode;
    gltfNode.name = node->name;
    //Conversión de matriz
    const float* matrixPtr = glm::value_ptr(node->localMatrix);
    gltfNode.matrix = std::vector<double>(matrixPtr, matrixPtr + 16);
    //Si el nodo actual tiene geometría
    if (node->mesh) {
        gltfNode.mesh = ExportMesh(node->mesh.get(), node->material.get(), model, mainBuffer);
    }
    //Procesar a todos los hijos
    for (const auto& child : node->children) {
        if (child) {
            int childIndex = ExportNode(child.get(), model, mainBuffer);
            gltfNode.children.push_back(childIndex);
        }
    }
    //Guardar el nodo en la lista global
    model.nodes.push_back(gltfNode);
    return static_cast<int>(model.nodes.size() - 1);
};

int Scene::ExportMaterial(Material* material, tinygltf::Model& model, tinygltf::Buffer& mainBuffer) {
    tinygltf::Material gltfMat;
    gltfMat.name = "Material_Exportado";
    //Exportar factores PBR básicos
    gltfMat.pbrMetallicRoughness.metallicFactor = material->metallicFactor;
    gltfMat.pbrMetallicRoughness.roughnessFactor = material->roughnessFactor;
    gltfMat.pbrMetallicRoughness.baseColorFactor = {
        material->baseColorFactor.r,
        material->baseColorFactor.g,
        material->baseColorFactor.b,
        material->baseColorFactor.a
    };
    //Mapear las texturas de nuestro material a los slots a glTF
    if (material->albedoMap) {
        int texIdx = ExportTexture(material->albedoMap.get(), model, mainBuffer);
        gltfMat.pbrMetallicRoughness.baseColorTexture.index = texIdx;
    }
    if (material->normalMap) {
        int texIdx = ExportTexture(material->normalMap.get(), model, mainBuffer);
        gltfMat.normalTexture.index = texIdx;
    }
    if (material->pbrMap) {
        int texIdx = ExportTexture(material->pbrMap.get(), model, mainBuffer);
        gltfMat.pbrMetallicRoughness.metallicRoughnessTexture.index = texIdx;
    }
    model.materials.push_back(gltfMat);
    return static_cast<int>(model.materials.size() - 1);
};

int Scene::ExportTexture(Texture* texture, tinygltf::Model& model, tinygltf::Buffer& mainBuffer) {
    tinygltf::Texture gltfTex;
    tinygltf::Image gltfImg;
    gltfImg.name = "Textura_Exportada_" + std::to_string(texture->ID);
    // Preguntar a la GPU las dimensiones reales de esta textura
    GLint width = 0, height = 0;
    glGetTextureLevelParameteriv(texture->ID, 0, GL_TEXTURE_WIDTH, &width);
    glGetTextureLevelParameteriv(texture->ID, 0, GL_TEXTURE_HEIGHT, &height);
    if (width > 0 && height > 0) {
        //Reservar memoria para recibir los píxeles
        std::vector<unsigned char> rawPixels(width * height * 4);
        //Descarga Textura
        glGetTextureImage(texture->ID, 0, GL_RGBA, GL_UNSIGNED_BYTE, static_cast<GLsizei>(rawPixels.size()), rawPixels.data());
        //Contexto de STB para escribir
        auto stbiBufferCallback = [](void* context, void* data, int size) {
            auto* vec = static_cast<std::vector<unsigned char>*>(context);
            const auto* bytes = static_cast<const unsigned char*>(data);
            vec->insert(vec->end(), bytes, bytes + size);
        };
        std::vector<unsigned char> compressedPngBytes;
        //Comprimir los píxeles a binario PNG
        stbi_write_png_to_func(stbiBufferCallback, &compressedPngBytes, width, height, 4, rawPixels.data(), width * 4);
        //Empaquetar el PNG binario
        size_t imgOffset = mainBuffer.data.size();
        size_t imgLength = compressedPngBytes.size();
        mainBuffer.data.resize(imgOffset + imgLength);
        std::memcpy(&mainBuffer.data[imgOffset], compressedPngBytes.data(), imgLength);
        //Crear una vista de buffer para que el lector glTF sepa dónde está el archivo PNG
        tinygltf::BufferView imgView;
        imgView.buffer = 0;
        imgView.byteOffset = imgOffset;
        imgView.byteLength = imgLength;
        int imgViewIdx = static_cast<int>(model.bufferViews.size());
        model.bufferViews.push_back(imgView);
        //Configurar la imagen glTF apuntando a esa vista de memoria
        gltfImg.bufferView = imgViewIdx;
        gltfImg.mimeType = "image/png";
        gltfImg.width = width;
        gltfImg.height = height;
        gltfImg.component = 4;
    } else {
        gltfImg.width = 1; gltfImg.height = 1; gltfImg.component = 4;
        gltfImg.image = { 255, 255, 255, 255 };
    }
    int imgIdx = static_cast<int>(model.images.size());
    model.images.push_back(gltfImg);
    gltfTex.source = imgIdx;
    model.textures.push_back(gltfTex);
    return static_cast<int>(model.textures.size() - 1);
};

void Scene::DrawLightGizmos(ShaderProgram& shaderProgram) {
    for (auto& light : lights) {
        if (light) {
            light->DrawGizmo(shaderProgram);
        }
    }
}

int Scene::ExportMesh(Mesh* mesh, Material* material, tinygltf::Model& model, tinygltf::Buffer& mainBuffer) {
    tinygltf::Mesh gltfMesh;
    tinygltf::Primitive primitive;
    primitive.mode = TINYGLTF_MODE_TRIANGLES;
    size_t vertexCount = mesh->vertices.size();
    if (vertexCount == 0) return -1;
    std::vector<glm::vec3> positions(vertexCount);
    std::vector<glm::vec3> normals(vertexCount);
    std::vector<glm::vec3> colors(vertexCount);
    std::vector<glm::vec2> uvs(vertexCount);
    std::vector<glm::vec4> gltfTangents(vertexCount);
    // Calcular el AABB de posiciones
    glm::vec3 pMin = mesh->vertices[0].position;
    glm::vec3 pMax = mesh->vertices[0].position;
    for (size_t i = 0; i < vertexCount; i++) {
        positions[i] = mesh->vertices[i].position;
        normals[i] = mesh->vertices[i].normal;
        colors[i] = mesh->vertices[i].color;
        uvs[i] = mesh->vertices[i].texCoords;
        glm::vec3 t = mesh->vertices[i].tangent;
        glm::vec3 b = mesh->vertices[i].biTangent;
        pMin = glm::min(pMin, positions[i]);
        pMax = glm::max(pMax, positions[i]);
        float w = (glm::dot(glm::cross(mesh->vertices[i].normal, t), b) < 0.0f) ? -1.0f : 1.0f;
        gltfTangents[i] = glm::vec4(t.x, t.y, t.z, w);
    }
    //SERIALIZAR POSICIONES
    size_t posOffset = mainBuffer.data.size();
    size_t posLength = vertexCount * sizeof(glm::vec3);
    mainBuffer.data.resize(posOffset + posLength);
    std::memcpy(&mainBuffer.data[posOffset], positions.data(), posLength);
    tinygltf::BufferView posView;
    posView.buffer = 0;
    posView.byteOffset = posOffset;
    posView.byteLength = posLength;
    posView.target = TINYGLTF_TARGET_ARRAY_BUFFER;
    int posViewIdx = static_cast<int>(model.bufferViews.size());
    model.bufferViews.push_back(posView);
    tinygltf::Accessor posAccessor;
    posAccessor.bufferView = posViewIdx;
    posAccessor.byteOffset = 0;
    posAccessor.componentType = TINYGLTF_COMPONENT_TYPE_FLOAT;
    posAccessor.count = vertexCount;
    posAccessor.type = TINYGLTF_TYPE_VEC3;
    posAccessor.minValues = { (double)pMin.x, (double)pMin.y, (double)pMin.z };
    posAccessor.maxValues = { (double)pMax.x, (double)pMax.y, (double)pMax.z };
    int posAccIdx = static_cast<int>(model.accessors.size());
    model.accessors.push_back(posAccessor);
    primitive.attributes["POSITION"] = posAccIdx;
    //SERIALIZAR NORMALES
    size_t normOffset = mainBuffer.data.size();
    size_t normLength = vertexCount * sizeof(glm::vec3);
    mainBuffer.data.resize(normOffset + normLength);
    std::memcpy(&mainBuffer.data[normOffset], normals.data(), normLength);
    tinygltf::BufferView normView;
    normView.buffer = 0;
    normView.byteOffset = normOffset;
    normView.byteLength = normLength;
    normView.target = TINYGLTF_TARGET_ARRAY_BUFFER;
    int normViewIdx = static_cast<int>(model.bufferViews.size());
    model.bufferViews.push_back(normView);
    tinygltf::Accessor normAccessor;
    normAccessor.bufferView = normViewIdx;
    normAccessor.byteOffset = 0;
    normAccessor.componentType = TINYGLTF_COMPONENT_TYPE_FLOAT;
    normAccessor.count = vertexCount;
    normAccessor.type = TINYGLTF_TYPE_VEC3;
    int normAccIdx = static_cast<int>(model.accessors.size());
    model.accessors.push_back(normAccessor);
    primitive.attributes["NORMAL"] = normAccIdx;
    //SERIALIZAR UVs
    size_t uvOffset = mainBuffer.data.size();
    size_t uvLength = vertexCount * sizeof(glm::vec2);
    mainBuffer.data.resize(uvOffset + uvLength);
    std::memcpy(&mainBuffer.data[uvOffset], uvs.data(), uvLength);
    tinygltf::BufferView uvView;
    uvView.buffer = 0;
    uvView.byteOffset = uvOffset;
    uvView.byteLength = uvLength;
    uvView.target = TINYGLTF_TARGET_ARRAY_BUFFER;
    int uvViewIdx = static_cast<int>(model.bufferViews.size());
    model.bufferViews.push_back(uvView);
    tinygltf::Accessor uvAccessor;
    uvAccessor.bufferView = uvViewIdx;
    uvAccessor.byteOffset = 0;
    uvAccessor.componentType = TINYGLTF_COMPONENT_TYPE_FLOAT;
    uvAccessor.count = vertexCount;
    uvAccessor.type = TINYGLTF_TYPE_VEC2;
    int uvAccIdx = static_cast<int>(model.accessors.size());
    model.accessors.push_back(uvAccessor);
    primitive.attributes["TEXCOORD_0"] = uvAccIdx;
    //SERIALIZAR ÍNDICES (EBO)
    size_t indOffset = mainBuffer.data.size();
    size_t indLength = mesh->indices.size() * sizeof(GLuint);
    mainBuffer.data.resize(indOffset + indLength);
    std::memcpy(&mainBuffer.data[indOffset], mesh->indices.data(), indLength);
    tinygltf::BufferView indView;
    indView.buffer = 0;
    indView.byteOffset = indOffset;
    indView.byteLength = indLength;
    indView.target = TINYGLTF_TARGET_ELEMENT_ARRAY_BUFFER;
    int indViewIdx = static_cast<int>(model.bufferViews.size());
    model.bufferViews.push_back(indView);
    tinygltf::Accessor indAccessor;
    indAccessor.bufferView = indViewIdx;
    indAccessor.byteOffset = 0;
    indAccessor.componentType = TINYGLTF_COMPONENT_TYPE_UNSIGNED_INT;
    indAccessor.count = mesh->indices.size();
    indAccessor.type = TINYGLTF_TYPE_SCALAR;
    int indAccIdx = static_cast<int>(model.accessors.size());
    model.accessors.push_back(indAccessor);
    primitive.indices = indAccIdx;
    //SERIALIZAR TANGENTES
    size_t tanOffset = mainBuffer.data.size();
    size_t tanLength = vertexCount * sizeof(glm::vec4);
    mainBuffer.data.resize(tanOffset + tanLength);
    std::memcpy(&mainBuffer.data[tanOffset], gltfTangents.data(), tanLength);
    tinygltf::BufferView tanView;
    tanView.buffer = 0;
    tanView.byteOffset = tanOffset;
    tanView.byteLength = tanLength;
    tanView.target = TINYGLTF_TARGET_ARRAY_BUFFER;
    int tanViewIdx = static_cast<int>(model.bufferViews.size());
    model.bufferViews.push_back(tanView);
    tinygltf::Accessor tanAccessor;
    tanAccessor.bufferView = tanViewIdx;
    tanAccessor.byteOffset = 0;
    tanAccessor.componentType = TINYGLTF_COMPONENT_TYPE_FLOAT;
    tanAccessor.count = vertexCount;
    tanAccessor.type = TINYGLTF_TYPE_VEC4;
    int tanAccIdx = static_cast<int>(model.accessors.size());
    model.accessors.push_back(tanAccessor);
    primitive.attributes["TANGENT"] = tanAccIdx;
    //SERIALIZAR COLOR
    size_t colOffset = mainBuffer.data.size();
    size_t colLength = vertexCount * sizeof(glm::vec3);
    mainBuffer.data.resize(colOffset + colLength);
    std::memcpy(&mainBuffer.data[colOffset], colors.data(), colLength);
    tinygltf::BufferView colView;
    colView.buffer = 0;
    colView.byteOffset = colOffset;
    colView.byteLength = colLength;
    colView.target = TINYGLTF_TARGET_ARRAY_BUFFER;
    int colViewIdx = static_cast<int>(model.bufferViews.size());
    model.bufferViews.push_back(colView);
    tinygltf::Accessor colAccessor;
    colAccessor.bufferView = colViewIdx;
    colAccessor.byteOffset = 0;
    colAccessor.componentType = TINYGLTF_COMPONENT_TYPE_FLOAT;
    colAccessor.count = vertexCount;
    colAccessor.type = TINYGLTF_TYPE_VEC3;
    int colAccIdx = static_cast<int>(model.accessors.size());
    model.accessors.push_back(colAccessor);
    primitive.attributes["COLOR_0"] = colAccIdx;
    if (material) {
        primitive.material = ExportMaterial(material, model, mainBuffer);
    }
    gltfMesh.primitives.push_back(primitive);
    model.meshes.push_back(gltfMesh);
    return static_cast<int>(model.meshes.size() - 1);
};

RayHit Scene::Raycast(Ray& worldRay) {
    RayHit finalHit;
    finalHit.distance = std::numeric_limits<float>::infinity();
    for (const auto& rootNode : rootNodes) {
        if (!rootNode) continue;
        RayHit tempHit;
        if (rootNode->Raycast(worldRay, tempHit)) {
            if (tempHit.distance < finalHit.distance) {
                finalHit = tempHit;
                finalHit.rootNode = rootNode.get();
            }
        }
    }
    if (finalHit.hasHit) {
        worldRay.hasHit = true;
        worldRay.hitPoint = worldRay.origin + (worldRay.direction * finalHit.distance);
        finalHit.worldPoint = worldRay.hitPoint;
        worldRay.UpdateLine();
    } else {
        worldRay.hasHit = false;
    }
    return finalHit;
}