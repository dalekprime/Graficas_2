#glad
add_library(glad STATIC glad/src/gl.c)
target_include_directories(glad PUBLIC glad/include)
#stb
add_library(stb STATIC stb/stb_image.cpp)
target_include_directories(stb PUBLIC stb)
#gltf
add_library(gltf STATIC gltf/tiny_gltf.cpp)
target_include_directories(gltf PUBLIC gltf)
target_link_libraries(gltf PRIVATE stb)