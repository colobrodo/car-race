#version 330 core

uniform sampler2D previousFrame;
uniform vec2 viewportSize;

void main() {
    float previousDepth = texture(previousFrame, (gl_FragCoord.xy - .5) / viewportSize).r;
    gl_FragDepth = min(gl_FragCoord.z, previousDepth);
    // gl_FragDepth = gl_FragCoord.z;
}