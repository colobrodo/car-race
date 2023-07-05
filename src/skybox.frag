#version 410 core
out vec4 FragColor;

in vec3 interp_UVW;

uniform samplerCube tex;

void main() {
    FragColor = texture(tex, interp_UVW);
    //FragColor = vec4(interp_UVW, 1);
}