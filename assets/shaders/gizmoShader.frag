#version 460 core
in vec3 vertexColor;
out vec4 FragColor;

uniform vec3 uColorOverride;
uniform int uUseOverride;

void main() {
    if (uUseOverride == 1) {
        FragColor = vec4(uColorOverride, 1.0);
    } else {
        FragColor = vec4(vertexColor, 1.0);
    }
}
