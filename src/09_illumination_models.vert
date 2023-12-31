/*
09_illumination_models.vert: Vertex shader for the Lambert, Phong, Blinn-Phong and GGX illumination models

N. B.) the shader treats a simplified situation, with a single point light.
For more point lights, a for cycle is needed to sum the contribution of each light
For different kind of lights, the computation must be changed (for example, a directional light is defined by the direction of incident light, so the lightDir is passed as uniform and not calculated in the shader like in this case with a point light).

author: Davide Gadia

Real-Time Graphics Programming - a.a. 2022/2023
Master degree in Computer Science
Universita' degli Studi di Milano

*/

#version 410 core

// vertex position in world coordinates
layout (location = 0) in vec3 position;
// vertex normal in world coordinate
layout (location = 1) in vec3 normal;
// UV texture coordinates, used for procedural generation of checked textures
layout (location = 2) in vec2 UV;
// the numbers used for the location in the layout qualifier are the positions of the vertex attribute
// as defined in the Mesh class

layout (location = 3) in vec3 tangent;
layout (location = 4) in vec3 bitangent;

// model matrix
uniform mat4 modelMatrix;
// view matrix
uniform mat4 viewMatrix;
// Projection matrix
uniform mat4 projectionMatrix;

uniform mat4 lightView;
uniform mat4 lightProjection;


// the position of the point light is passed as uniform
// N. B.) with more lights, and of different kinds, the shader code must be modified with a for cycle, with different treatment of the source lights parameters (directions, position, cutoff angle for spot lights, etc)
uniform vec3 lightVector;

// light incidence direction (in view coordinates)
out vec3 lightDir;
// the transformed normal (in view coordinate) is set as an output variable, to be "passed" to the fragment shader
// this means that the normal values in each vertex will be interpolated on each fragment created during rasterization between two vertices
out vec3 vNormal;
out vec3 vTangent;
out vec3 vBitangent;

// UV texture coordinates, interpolated in each fragment by the rasterization process
out vec2 interp_UV;

// in the subroutines in fragment shader where specular reflection is considered,
// we need to calculate also the reflection vector for each fragment
// to do this, we need to calculate in the vertex shader the view direction (in view coordinates) for each vertex, and to have it interpolated for each fragment by the rasterization stage
out vec3 vViewPosition;

out vec4 lightFragPosition;


void main() {

  // vertex position in ModelView coordinate (see the last line for the application of projection)
  // when I need to use coordinates in camera coordinates, I need to split the application of model and view transformations from the projection transformations
  vec4 mvPosition = viewMatrix * modelMatrix * vec4( position, 1.0 );

  // view direction, negated to have vector from the vertex to the camera
  vViewPosition = -mvPosition.xyz;

  // transformations are applied to the normal
  vNormal = normal;
  vTangent = tangent;
  vBitangent = bitangent;

  // light incidence direction (in view coordinate)
  lightDir = (viewMatrix  * vec4(lightVector, 0.0)).xyz;
  
  // we apply the projection transformation
  gl_Position = projectionMatrix * mvPosition;

  lightFragPosition = lightProjection * lightView * modelMatrix * vec4(position, 1.0);

  interp_UV = UV;
}
