#version 410 core
out vec4 FragColor;

in vec2 interp_UV;

uniform sampler2D tex;

vec3 bottomColor = vec3(.7, .7, .7);

void main() {
    float h;
    h = texture(tex, interp_UV).r;
    vec3 white = vec3(1.0, 1.0, 1.0);
    vec3 color = mix(bottomColor, white, h);
    
    FragColor = vec4(color, 1.0);
}