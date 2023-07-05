#version 410 core
out vec4 FragColor;

in vec2 interp_UV;

void main() {
    FragColor = vec4(interp_UV, 0, 1);
}