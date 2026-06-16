# Diagrama de Clases del Proyecto `Graficas_2`

El motor gráfico `Graficas_2` es un proyecto estructurado en módulos para el manejo eficiente de gráficos en OpenGL. A continuación, se presenta un diagrama completo de la arquitectura del proyecto y sus componentes, generados a partir de los encabezados (headers).

## Diagrama Completo (Mermaid)

```mermaid
classDiagram

    %% ========== ENUMS ==========
    class ShaderType {
        <<enumeration>>
        VERTEX
        FRAGMENT
        PROGRAM
    }

    class CameraType {
        <<enumeration>>
        RASTERIZATION
        RAYTRACING
    }

    class RayPrecision {
        <<enumeration>>
        GLOBAL
        LOCAL
        TRIANGLE
    }

    class LightType {
        <<enumeration>>
        DIRECTIONAL
        POINT
        SPOT
    }

    class ShadowType {
        <<enumeration>>
        PLANAR
        MAPPING
        VOLUMEN
    }

    class texType {
        <<enumeration>>
        ALBEDO
        NORMAL
        BUMP
        PBR
        ROUGHNESS
        METALLIC
        AO
    }

    %% ========== STRUCTS ==========
    class Vertex {
        <<struct>>
        +glm::vec3 position
        +glm::vec3 normal
        +glm::vec3 color
        +glm::vec2 texCoords
        +glm::vec3 tangent
        +glm::vec3 biTangent
    }

    class RayHit {
        <<struct>>
        +bool hasHit
        +float distance
        +glm::vec3 worldPoint
        +Node* rootNode
        +Node* localNode
        +glm::vec3 triV0
        +glm::vec3 triV1
        +glm::vec3 triV2
        +int selectedTriangleIndex
    }

    class GPUVertex {
        <<struct>>
        +glm::vec4 position
        +glm::vec4 normal
        +glm::vec4 extra
    }

    class GPUMaterial {
        <<struct>>
        +glm::vec4 albedo
        +glm::vec4 properties
        +glm::vec4 texIndices
    }

    class GPUNode {
        <<struct>>
        +glm::vec4 aabbMin
        +glm::vec4 aabbMax
    }

    %% ========== CORE CLASSES ==========
    class MainEngine {
        -int width
        -int height
        -GLFWwindow* window
        -ShaderProgram* actualShader
        -ShadowEngine* shadowEngine
        -Camera* actualCamera
        -Scene* actualScene
        -Ray* pickingRay
        -float deltaTime
        -float lastFrame
        +MainEngine(int width, int height)
        +SetupWindow()
        +MainLoop()
        +Update()
        +DrawUI()
        +Cleanup()
        +static FramebufferSizeCallback()
    }

    class ShaderProgram {
        +GLuint ID
        +ShaderProgram(const char* vertexFile, const char* fragmentFile)
        +~ShaderProgram()
        +CheckCompileErrors(GLuint shader, ShaderType type)
        +Activate()
        +Deactivate()
        +Delete()
        +SetInt()
        +SetFloat()
        +SetVec3()
        +SetMatrix4()
    }

    %% ========== BUFFERS MANAGEMENT ==========
    class VAO {
        +GLuint ID
        +VAO()
        +~VAO()
        +LinkAttrib()
        +LinkEBO()
        +Bind()
        +Unbind()
        +Delete()
    }

    class VBO {
        +GLuint ID
        +VBO(const std::vector~Vertex~& vertices)
        +~VBO()
        +Bind()
        +Unbind()
        +Delete()
    }

    class EBO {
        +GLuint ID
        +EBO(const std::vector~GLuint~& indices)
        +~EBO()
        +Bind()
        +Unbind()
        +Delete()
    }

    %% ========== CAMERA MANAGEMENT ==========
    class Camera {
        +CameraType type
        +glm::vec3 position
        +glm::vec3 orientation
        +glm::vec3 up
        +glm::mat4 cameraMatrix
        +int width
        +int height
        +float fov
        +Camera(int width, int height, glm::vec3 pos, glm::vec3 ori, glm::vec3 up)
        +updateMatrix()
        +matrix()
        +Inputs()
    }

    class Ray {
        +VAO* vao
        +VBO* vbo
        +glm::vec3 origin
        +glm::vec3 direction
        +glm::vec3 hitPoint
        +bool hasHit
        +RayPrecision precision
        +Ray(glm::vec3 origin, glm::vec3 dir, glm::vec3 color)
        +Draw()
        +UpdateLine()
    }

    %% ========== ILLUMINATION MANAGEMENT ==========
    class Light {
        +LightType type
        +glm::vec3 position
        +glm::vec3 direction
        +glm::vec3 color
        +GLfloat intensity
        +Light(LightType type, glm::vec3 pos)
        +DrawGizmo()
    }

    class ShadowEngine {
        -ShadowType actualType
        -GLuint depthMapFBO
        -GLuint depthMap
        -int shadowRes
        +float groundHeight
        +ShadowEngine(ShadowType type)
        +RenderSceneWithShadows()
        +SetShadowType()
    }

    %% ========== MODEL MANAGEMENT ==========
    class Texture {
        +GLuint ID
        +texType type
        +std::string path
        +Texture()
        +Bind()
        +Unbind()
        +Delete()
    }

    class Material {
        +std::shared_ptr~Texture~ albedoMap
        +std::shared_ptr~Texture~ normalMap
        +std::shared_ptr~Texture~ pbrMap
        +float metallicFactor
        +float roughnessFactor
        +glm::vec4 baseColorFactor
        +Bind()
    }

    class Mesh {
        +std::unique_ptr~VAO~ vao
        +std::unique_ptr~VBO~ vbo
        +std::unique_ptr~EBO~ ebo
        +std::vector~Vertex~ vertices
        +std::vector~GLuint~ indices
        +Mesh(const std::vector~Vertex~& v, const std::vector~GLuint~& i)
        +Draw()
        +UpdateLocalAABB()
        +Raycast()
    }

    class Node {
        +std::string name
        +std::shared_ptr~Mesh~ mesh
        +std::shared_ptr~Material~ material
        +Node* parent
        +std::vector~std::unique_ptr~Node~~ children
        +glm::mat4 localMatrix
        +glm::mat4 worldMatrix
        +Node(const std::string& name)
        +AddChild()
        +Draw()
        +UpdateWorldMatrix()
        +UpdateWorldAABB()
        +Raycast()
    }

    class Scene {
        +std::vector~std::unique_ptr~Node~~ rootNodes
        +std::vector~Camera~ cameras
        +std::vector~std::shared_ptr~Light~~ lights
        +AddRootNode()
        +UpdateScene()
        +Draw()
        +RenderRaytraced()
        +Raycast()
        +SaveToGLTF()
        +LoadFromGLTF()
    }

    class Solid {
        +static GenerateRevolution()
        +static GeneratePlane()
        +static EvaluateBezier()
    }

    %% ========== RELATIONSHIPS ==========
    MainEngine "1" *-- "many" ShaderProgram
    MainEngine "1" *-- "1" ShadowEngine
    MainEngine "1" *-- "1" Camera : actualCamera
    MainEngine "1" *-- "1" Scene : actualScene
    MainEngine "1" *-- "1" Ray : pickingRay
    MainEngine "1" o-- "1" Node : selectedNode

    VAO "1" *-- "1" VBO
    VAO "1" *-- "1" EBO
    VBO "1" *-- "many" Vertex : vertices

    Ray "1" *-- "1" VAO
    Ray "1" *-- "1" VBO
    Ray ..> RayHit : generates

    ShadowEngine "1" *-- "many" ShaderProgram
    
    Material "1" o-- "many" Texture
    Mesh "1" *-- "1" VAO
    Mesh "1" *-- "1" VBO
    Mesh "1" *-- "1" EBO
    Mesh "1" *-- "many" Vertex
    
    Node "1" o-- "1" Mesh
    Node "1" o-- "1" Material
    Node "1" *-- "many" Node : children
    Node "1" o-- "1" Node : parent
    
    Scene "1" *-- "many" Node : rootNodes
    Scene "1" *-- "many" Camera
    Scene "1" o-- "many" Light
    
    Scene ..> GPUVertex
    Scene ..> GPUMaterial
    Scene ..> GPUNode
```

## Resumen de Módulos

El proyecto se divide de forma limpia en diferentes espacios y responsabilidades que trabajan en conjunto para renderizar la imagen final.

### **1. Motor Principal (`MainEngine`)**
Es la clase base que corre la lógica completa del ciclo de vida (`MainLoop`). Administra la inicialización de GLFW, el estado de la UI (ImGui), y posee referencias únicas (`std::unique_ptr`) a la Escena, las Cámaras, el gestor de Sombras, los Shaders de dibujo, y el control de selecciones interactivas vía Raycasting.

### **2. Manejo de Buffers (`BuffersManagement`)**
- `VAO` (Vertex Array Object), `VBO` (Vertex Buffer Object), y `EBO` (Element Buffer Object) actúan como interfaces directas de alto nivel a las llamadas de memoria en la GPU de OpenGL.
- La estructura `Vertex` engloba la información completa de cada vértice (Posición, Normal, Coordenadas de Textura, Tangente y Bitangente).

### **3. Modelo y Escena (`ModelManagement`)**
- Implementa una arquitectura en **Jerarquía de Nodos**. Cada `Node` puede tener un modelo en bruto `Mesh` (con su propia geometría), un material `Material` (y sus texturas `Texture`), además de hijos dependientes y matrices de transformación locales/globales.
- La clase `Scene` es el contenedor maestro. Alberga el cache de texturas y materiales, además de los nodos raíz. Expone funciones avanzadas para guardar/cargar el formato de estándar abierto **GLTF** y renderizado por raytracing.
- La clase `Solid` provee utilidades estáticas de generación procedural, como la creación de superficies de revolución y planos.

### **4. Cámara e Interacción (`CameraManagement`)**
- Provee la implementación de una cámara clásica virtual `Camera` con vista en perspectiva, matrices de proyección y manejo de inputs para navegar en la escena 3D.
- Adicionalmente, cuenta con el módulo de interacciones a través de la clase `Ray` para trazar rayos (picking y raycasting) y la estructura `RayHit` para procesar intersecciones con polígonos del `Mesh`.

### **5. Iluminación y Sombras (`IluminationManagement`)**
- Abstrae el manejo de luces puntuales, direccionales y focos mediante `Light`.
- Incluye el sistema `ShadowEngine` que orquesta los múltiples pases de la escena para mapeo de sombras en modos Planar, de Mapeo Clásico o Volumétrico para PBR.
