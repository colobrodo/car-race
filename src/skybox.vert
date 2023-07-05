#version 410 core
layout (location = 0) in vec3 position;

out vec3 interp_UVW;

uniform mat4 projectionMatrix;
uniform mat4 viewMatrix;

void main() {
    interp_UVW = position;
    vec4 pos = projectionMatrix * viewMatrix * vec4(position, 1.0);
    // we want to set the Z coordinate of the projected vertex at the maximum depth (i.e., we want Z to be equal to 1.0 after the projection divide)
    // -> we set Z equal to W (because in the projection divide, after clipping, all the components will be divided by W).
    // This means that, during the depth test, the fragments of the environment map will have maximum depth (see comments in the code of the main application)
    gl_Position = pos.xyww;
}