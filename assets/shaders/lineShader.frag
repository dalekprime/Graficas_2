#version 460 core

in vec3 Color;
out vec4 FragColor;

void main() {
    // Simplemente dibujamos el color que venga interpolado del vértice
    FragColor = vec4(Color, 1.0);
}