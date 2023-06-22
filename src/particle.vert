#version 410 core

// vertex position in world coordinates
layout (location = 0) in vec3 position;
// UV texture coordinates, used for procedural generation of checked textures
layout (location = 1) in vec2 UV;
// matrix model trasformation
layout (location = 2) in mat4 modelMatrix;
// particle color
layout (location = 6) in vec4 particleColor;
// the numbers used for the location in the layout qualifier are the positions of the vertex attribute
// as defined in the Mesh class

// view matrix
uniform mat4 viewMatrix;
// Projection matrix
uniform mat4 projectionMatrix;

// UV texture coordinates, interpolated in each fragment by the rasterization process
out vec2 interp_UV;

// color to send to fragment shader
out vec4 color1;

// in the subroutines in fragment shader where specular reflection is considered,
// we need to calculate also the reflection vector for each fragment
// to do this, we need to calculate in the vertex shader the view direction (in view coordinates) for each vertex, and to have it interpolated for each fragment by the rasterization stage
out vec3 vViewPosition;


void main() {

  // vertex position in ModelView coordinate (see the last line for the application of projection)
  // when I need to use coordinates in camera coordinates, I need to split the application of model and view transformations from the projection transformations
  vec4 mvPosition = viewMatrix * modelMatrix * vec4( position, 1.0 );

  // view direction, negated to have vector from the vertex to the camera
  vViewPosition = -mvPosition.xyz;

  // we apply the projection transformation
  gl_Position = projectionMatrix * mvPosition;

  interp_UV = UV;
  color1 = particleColor;
}
