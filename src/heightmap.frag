#version 410 core
out vec4 FragColor;

in vec2 interp_UV;

// heightmap texture
uniform sampler2D tex;
// image texture to draw
uniform sampler2D imageTex;
// shadows
uniform sampler2DShadow shadowMap;

in vec4 lightFragPosition;


vec2 poissonDisk[4] = vec2[](
  vec2( -0.94201624, -0.39906216 ),
  vec2( 0.94558609, -0.76890725 ),
  vec2( -0.094184101, -0.92938870 ),
  vec2( 0.34495938, 0.29387760 )
);

float calculateShadow() {
    // given the fragment position in light coordinates, we apply the perspective divide. Usually, perspective divide is applied in an automatic way to the coordinates saved in the gl_Position variable. In this case, the vertex position in light coordinates has been saved in a separate variable, so we need to do it manually
    vec3 lightPosOnPlane = lightFragPosition.xyz / lightFragPosition.w;
    // after the perspective divide the values are in the range [-1,1]: we must convert them in [0,1]
    lightPosOnPlane = lightPosOnPlane * 0.5 + 0.5;
    float bias = 0.001;
    lightPosOnPlane.z -= bias;
    // checking if the fragment coordinate is outside the light view frustum
    if(lightPosOnPlane.z > 1.0)
        return 1.0;
    // poisson sampling for better shadows, inspired from:
    // http://www.opengl-tutorial.org/intermediate-tutorials/tutorial-16-shadow-mapping/#poisson-sampling
    float shadow = 1.0; 
    for (int i = 0; i < 4; i++) {
        vec3 samplePoint = vec3(lightPosOnPlane.xy + poissonDisk[i] / 700, lightPosOnPlane.z);
        float shadowDepth = 1.0 - texture(shadowMap, samplePoint);
        shadow -= 0.25 * shadowDepth;
    }
    return shadow;
}

void main() {
    // float h;
    // h = texture(tex, interp_UV).r;
    // vec3 white = vec3(1.0, 1.0, 1.0);
    // vec3 color = mix(bottomColor, white, h);
    
    float shadowFactor = calculateShadow();;
    vec4 color = texture(imageTex, interp_UV) * shadowFactor;
    FragColor = vec4(color.rgb, 1.0);
    
}