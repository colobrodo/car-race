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
in vec3 lightDir;
// the height of the map in world space 
in float Height;
// relative height
// the height depth from 0 to 1 (directly sampled from the texture)
in float h;


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
    // interpolation of two color based on relative height (used for debugging)
    vec3 topColor = texture(imageTex, interp_UV).rgb;
    vec3 bottomColor = vec3(1, 0, 0);
    /*
    // pcf on the heightmap
    vec2 texelSize = 1.0 / textureSize(tex, 0);
    for(int i = -1; i <= 1; i++) {
        float dx = i * texelSize.x;
        for(int j = -1; j <= 1; j++) {
            float dy = j * texelSize.x;
            vec2 close_uv = interp_UV + vec2(dx, dy);
            h += texture(tex, interp_UV).r;
        }
    }
    h /= 9;
    vec3 color = mix(bottomColor, topColor, h);
    */

    // reducing the shadow factor a bit for a shadow non total black that leave
    // objects visible
    float shadowFactor = mix(.2, 1, calculateShadow());

    // setting color interpolation
    vec3 color = topColor * mix(.7, 1, h) * shadowFactor;
    
    /*
    // check if some of the texels in the direction of the light are occluding it
    // simply if the y coordinate is greater
    // looping and branching in the fragment: inefficent
    vec2 off = lightDir.xz / 50;
    for(float k = 0; k <= 1; k += .05) {
        vec2 occlusionPoint = interp_UV + off * k;
        float heightOfOcclusion = texture(tex, occlusionPoint).r;
        float heightOfLightRay  = h + lightDir.y * k;
        if(heightOfOcclusion > heightOfLightRay) {
            // occluded light 
            shadowFactor -= .8;
            break;
        }
    }
    shadowFactor = max(shadowFactor, 0);
    vec4 color = texture(imageTex, interp_UV) * shadowFactor;
    */
    
    FragColor = vec4(color.rgb, 1.0);
}