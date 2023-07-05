#version 410 core
layout (location = 0) in vec3 position;
layout (location = 1) in vec2 uv;

out vec2 UV_coord;

uniform mat4 modelMatrix;
uniform mat4 viewMatrix;
uniform mat4 projectionMatrix;

void main() {
    UV_coord = uv;
    gl_Position = vec4(position, 1.0);
}