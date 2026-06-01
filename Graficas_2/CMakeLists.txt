# Add source to this project's executable.
add_executable (Graficas_2 "src/Graficas_2.cpp" "src/Graficas_2.h")

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