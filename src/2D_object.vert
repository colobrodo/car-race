#version 410 core

// vertex position in world coordinates
layout (location = 0) in vec3 position;
// UV texture coordinates, used for procedural generation of checked textures
layout (location = 1) in vec2 UV;
// matrix model trasformation
layout (location = 2) in mat4 modelMatrix;
// the numbers used for the location in the layout qualifier are the positions of the vertex attribute
// as defined in the Mesh class

// view matrix
uniform mat4 viewMatrix;
// Projection matrix
uniform mat4 projectionMatrix;

uniform mat4 lightProjection;
uniform mat4 lightView;

out vec4 lightFragPosition;

out vec2 interp_UV;

void main() {

  // vertex position in ModelView coordinate (see the last line for the application of projection)
  // when I need to use coordinates in camera coordinates, I need to split the application of model and view transformations from the projection transformations
  vec4 mPosition = modelMatrix * vec4( position, 1.0 );
  vec4 mvPosition = viewMatrix * mPosition;

  interp_UV = UV;
  // we apply the projection transformation
  gl_Position = projectionMatrix * mvPosition;
  lightFragPosition = lightProjection * lightView * mPosition;
}
