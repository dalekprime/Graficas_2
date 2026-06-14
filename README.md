# Proyecto 1: Técnicas de Rasterización y Ray Tracing

---

## Instrucciones de Compilación

Este proyecto es totalmente autocontenido y está diseñado para compilarse sin necesidad de configurar librerías en el sistema, ya que utiliza el sistema de dependencias dinámicas `FetchContent` de CMake en su ultima version.
**Pasos:**
1. Abrir **Visual Studio 2026**.
2. Seleccionar `Abrir una carpeta local` y buscar la carpeta raíz de este proyecto (donde se encuentra el `CMakeLists.txt` principal).
3. Esperar a que Visual Studio termine de configurar CMake e indexar el proyecto (ver ventana de Salida/Output). Esto descargará *GLFW, GLM, ImGui y TinyObjLoader*.
4. Asegurarse de tener el destino configurado como `Graficas_2.exe` (x64-Debug o x64-Release) en la barra superior.
5. Hacer clic en el botón de **Play (Run)** o presionar `F5` para compilar y ejecutar el proyecto.

---

## Librerías y Dependencias

- **GLFW:** Creación de ventana y manejo de inputs.
- **GLM:** Matemáticas (Vectores, Matrices, Cuaterniones).
- **ImGui:** Interfaz gráfica de usuario.
- **TinyObjLoader:** Para modelos `.obj` de respaldo. (No utilizada).
- **GLAD:** Cargador de perfiles de OpenGL (Core 4.6).
- **TinyGLTF:** Carga y guardado de escenas `.gltf` / `.glb`.
- **stb_image / stb_image_write:** Carga de imágenes en formatos PNG/JPG y generación de texturas.

---

## Controles de la Interfaz

* **Movimiento de la Cámara Libre:** Las letras **WASD** mueven la cámara en un plano horizontal, mientras que el mouse permite rotarla libremente.

* **Ray Casting & Picking:** Hacer clic derecho en cualquier lugar de la escena para lanzar un rayo; si intersecta un modelo, este se pintará de un color de resalte y abrirá el Inspector de Nodos en el panel lateral, permitiendo alterar sus propiedades.

---

## Funcionalidades Implementadas

1. **Cargar/Salvar escenas en formato glTF/GLB:** Abstraído totalmente con el uso de TinyGLTF; permite guardar el estado del árbol de objetos.

2. **Generación de Sólidos de Revolución (Bézier):** Módulo interactivo dentro del motor con un canvas 2D. Genera normales suaves calculando *Tangentes y Bitangentes* necesarias para normal mapping.

3. **Iluminación Local Básica:** Selección dinámica de shaders para evaluar ecuaciones Flat, Lambert, Phong y Blinn-Phong.

4. **Objeto Rayo y Resalte:** Visualización 3D del vector normalizado y cambio dinámico del color al impactar en AABB y Triángulos.

5. **Picking Básico Modificable:** Inspector en la interfaz para aplicar transformaciones geométricas locales e intercambiar texturas interactivamente.

6. **Sombras Planares:** La geometría se aplana contra el piso y carece de colisión con luces de escena.

7. **Shadow Mapping Básico:** Con controles visuales para mitigar *Shadow Acne* (`Bias Fijo`) e ilustración del problema de *Peter Panning*.

8. **PCF Configurable:** Kernel escalable desde la UI (3x3, 5x5, 7x7) para ocultar artefactos del Shadow Map, activable y desactivable.

9. **Cámaras Múltiples Libres:** Capacidad de almacenar estados de cámaras. Integración profunda de un renderizador **Ray Tracing por GPU**, empleando **SSBOs** para enviar de forma lineal todas las mallas, materiales e índices.

10. **Texturizado y Coordenadas Procedurales:** Modificación del UV mapping en Normal Map.

11. **Material PBR Físicamente Plausible:** Shader programado empleando *Cook-Torrance (GGX)* que responde dinámicamente a la rugosidad, metalicidad e iluminación ambiental.

12. **PCSS:** Implementación de sombras de contacto (Percentage Closer Soft Shadows) dinámicas, produciendo penumbras variadas.

---

## Limitaciones y Notas

* **Ray Tracing en Tiempo Real:** Dado que el algoritmo se está ejecutando puramente en Fragment Shaders leyendo SSBOs en bucles, escenas demasiado densas en polígonos bajarán sustancialmente los FPS.

* **Shadow Mapping:** Solamente la luz posicionada en el `Índice 0` de la lista global emitirá sombras.

* **Sistema de Iluminación Escalable:** Tanto el pipeline de Rasterización como el de Ray Tracing asumen la existencia de múltiples tipos de luces simultáneas y responden en consecuencia.

---

# Autor

Desarrollado para la cátedra de Técnicas y Fundamentos de la Computación Gráfica. Universidad Central de Venezuela (UCV). Por Bryan Silva, 2026.
