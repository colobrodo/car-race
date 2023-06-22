
#version 410 core

// output shader variable
out vec4 colorFrag;

// light incidence direction (calculated in vertex shader, interpolated by rasterization)
in vec3 lightDir;
// the transformed normal has been calculated per-vertex in the vertex shader
in vec3 vNormal;
// vector from fragment to camera (in view coordinate)
in vec3 vViewPosition;

// UV texture coordinates, interpolated in each fragment by the rasterization process
in vec2 interp_UV;

in vec4 color1;

// the "type" of the Subroutine
subroutine float particle_draw();

// Subroutine Uniform (it is conceptually similar to a C pointer function)
subroutine uniform particle_draw Particle_Draw;

subroutine(particle_draw)
float Circle() {
    return step(length(interp_UV - .5), .5);
}

subroutine(particle_draw)
float Square() {
    return 1;
}

void main(void)
{
    // k is always 0 or 1
    float k = Particle_Draw();
    if(k == 0.0) discard;
    colorFrag = color1 * k;
}
