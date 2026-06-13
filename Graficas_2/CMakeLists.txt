# Add source to this project's executable.
add_executable (Graficas_2 "src/Graficas_2.h" "src/Graficas_2.cpp"
                           "src/Shader.h" "src/Shader.cpp" 
                           "src/BuffersManagement/VBO.h" "src/BuffersManagement/VBO.cpp" 
                           "src/BuffersManagement/VAO.h" "src/BuffersManagement/VAO.cpp"  
                           "src/BuffersManagement/EBO.h" "src/BuffersManagement/EBO.cpp" 
                           "src/ModelManagement/Node.h" "src/ModelManagement/Node.cpp" 
                           "src/ModelManagement/Scene.h" "src/ModelManagement/Scene.cpp" 
                           "src/ModelManagement/Texture.h" "src/ModelManagement/Texture.cpp"
                           "src/ModelManagement/Solid.h" "src/ModelManagement/Solid.cpp"
                           "src/CameraManagement/Camera.h" "src/CameraManagement/Camera.cpp" 
                           "src/CameraManagement/Ray.h" "src/CameraManagement/Ray.cpp"
                           "src/IluminationManagement/Light.h" "src/IluminationManagement/Light.cpp"
                           "src/IluminationManagement/ShadowEngine.h" "src/IluminationManagement/ShadowEngine.cpp"
                           "${CMAKE_SOURCE_DIR}/assets/shaders/standardShader.vert"
                           "${CMAKE_SOURCE_DIR}/assets/shaders/flatShader.frag"
                           "${CMAKE_SOURCE_DIR}/assets/shaders/lambertShader.frag"
                           "${CMAKE_SOURCE_DIR}/assets/shaders/phongShader.frag"
                           "${CMAKE_SOURCE_DIR}/assets/shaders/blinnPhongShader.frag"
                           "${CMAKE_SOURCE_DIR}/assets/shaders/pbrShader.frag"
                           "${CMAKE_SOURCE_DIR}/assets/shaders/RTShader.vert"
                           "${CMAKE_SOURCE_DIR}/assets/shaders/RTShader.frag"
                           "${CMAKE_SOURCE_DIR}/assets/shaders/gizmoShader.vert"
                           "${CMAKE_SOURCE_DIR}/assets/shaders/gizmoShader.frag"
                           "${CMAKE_SOURCE_DIR}/assets/shaders/planar.vert"
                           "${CMAKE_SOURCE_DIR}/assets/shaders/planar.frag"
                           "${CMAKE_SOURCE_DIR}/assets/shaders/depth.vert"
                           "${CMAKE_SOURCE_DIR}/assets/shaders/depth.frag"
)

add_custom_command(
    TARGET Graficas_2 POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory
        "${CMAKE_SOURCE_DIR}/assets"
        "${CMAKE_CURRENT_BINARY_DIR}/assets"
)

if (CMAKE_VERSION VERSION_GREATER 3.12)
  set_property(TARGET Graficas_2 PROPERTY CXX_STANDARD 20)
endif()

target_include_directories(Graficas_2 PUBLIC "${PROJECT_SOURCE_DIR}/src")

target_link_libraries(Graficas_2 PRIVATE 
	glfw  
    glm
    glad
    stb
    gltf
    imgui
    tinyobjloader
    OpenGL::GL
)