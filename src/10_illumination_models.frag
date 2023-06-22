/*

10_illumination_models.frag: Fragment shader for the Lambert, Phong, Blinn-Phong and GGX illumination models

N.B. 1)  "09_illumination_models.vert" must be used as vertex shader

N.B. 2)  the different illumination models are implemented using Shaders Subroutines

author: Davide Gadia

Real-Time Graphics Programming - a.a. 2022/2023
Master degree in Computer Science
Universita' degli Studi di Milano

*/

#version 410 core

const float PI = 3.14159265359;

// output shader variable
out vec4 colorFrag;

// light incidence direction (calculated in vertex shader, interpolated by rasterization)
in vec3 lightDir;
// the transformed normal has been calculated per-vertex in the vertex shader
in vec3 vNormal;
in vec3 vTangent;
in vec3 vBitangent;
// vector from fragment to camera (in view coordinate)
in vec3 vViewPosition;

// UV texture coordinates, interpolated in each fragment by the rasterization process
in vec2 interp_UV;

uniform mat4 modelMatrix;

// ambient, diffusive and specular components (passed from the application)
uniform vec3 ambientColor;
uniform vec3 color1;
uniform vec3 color2;
uniform vec3 specularColor;
// weight of the components
// in this case, we can pass separate values from the main application even if Ka+Kd+Ks>1. In more "realistic" situations, I have to set this sum = 1, or at least Kd+Ks = 1, by passing Kd as uniform, and then setting Ks = 1.0-Kd
uniform float Ka;
uniform float Kd;
uniform float Ks;

// optional texture
uniform sampler2D tex;
uniform sampler2D normalMap;
// normals transformation matrix (= transpose of the inverse of the model-view matrix)
uniform mat3 normalMatrix;

// shininess coefficients (passed from the application)
uniform float shininess;

// uniforms for GGX model
uniform float alpha; // rugosity - 0 : smooth, 1: rough
uniform float F0; // fresnel reflectance at normal incidence

float repeat = 40.0;


////////////////////////////////////////////////////////////////////

// the "type" of the Subroutine
subroutine vec3 ill_model(vec3 diffuseColor);

// Subroutine Uniform (it is conceptually similar to a C pointer function)
subroutine uniform ill_model Illumination_Model;

// subroutine to calculate the diffuse color of the model 
subroutine vec3 color_pattern();

subroutine uniform color_pattern Color_Pattern;

// subroutine to calculate the normal of the model, can be from the mesh or from a normalmap 
subroutine vec3 get_normal();

subroutine uniform get_normal Get_Normal;


////////////////////////////////////////////////////////////////////

//////////////////////////////////////////
// a subroutine for the Lambert model
subroutine(ill_model)
vec3 Lambert(vec3 diffuseColor) // this name is the one which is detected by the SetupShaders() function in the main application, and the one used to swap subroutines
{
    // normalization of the per-fragment normal
    vec3 N = normalize(Get_Normal());
    // normalization of the per-fragment light incidence direction
    vec3 L = normalize(lightDir.xyz);

    // Lambert coefficient
    float lambertian = max(dot(L, N), 0.0);

    // Lambert illumination model
    return vec3(Kd * lambertian * diffuseColor);
}
//////////////////////////////////////////

//////////////////////////////////////////
// a subroutine for the Phong model
subroutine(ill_model)
vec3 Phong(vec3 diffuseColor) // this name is the one which is detected by the SetupShaders() function in the main application, and the one used to swap subroutines
{
    // ambient component can be calculated at the beginning
    vec3 color = Ka*ambientColor;

    // normalization of the per-fragment normal
    vec3 N = normalize(Get_Normal());

    // normalization of the per-fragment light incidence direction
    vec3 L = normalize(lightDir.xyz);

    // Lambert coefficient
    float lambertian = max(dot(L, N), 0.0);

    // if the lambert coefficient is positive, then I can calculate the specular component
    if(lambertian > 0.0)
    {
      // the view vector has been calculated in the vertex shader, already negated to have direction from the mesh to the camera
      vec3 V = normalize( vViewPosition );

      // reflection vector
      vec3 R = reflect(-L, N);

      // cosine of angle between R and V
      float specAngle = max(dot(R, V), 0.0);
      // shininess application to the specular component
      float specular = pow(specAngle, shininess);

      // We add diffusive and specular components to the final color
      // N.B. ): in this implementation, the sum of the components can be different than 1
      color += vec3( Kd * lambertian * diffuseColor +
                      Ks * specular * specularColor);
    }
    return color;
}
//////////////////////////////////////////

//////////////////////////////////////////
// a subroutine for the Blinn-Phong model
subroutine(ill_model)
vec3 BlinnPhong(vec3 diffuseColor) // this name is the one which is detected by the SetupShaders() function in the main application, and the one used to swap subroutines
{
    // ambient component can be calculated at the beginning
    vec3 color = Ka*ambientColor;

    // normalization of the per-fragment normal
    vec3 N = normalize(Get_Normal());

    // normalization of the per-fragment light incidence direction
    vec3 L = normalize(lightDir.xyz);

    // Lambert coefficient
    float lambertian = max(dot(L,N), 0.0);

    // if the lambert coefficient is positive, then I can calculate the specular component
    if(lambertian > 0.0)
    {
      // the view vector has been calculated in the vertex shader, already negated to have direction from the mesh to the camera
      vec3 V = normalize( vViewPosition );

      // in the Blinn-Phong model we do not use the reflection vector, but the half vector
      vec3 H = normalize(L + V);

      // we use H to calculate the specular component
      float specAngle = max(dot(H, N), 0.0);
      // shininess application to the specular component
      float specular = pow(specAngle, shininess);

      // We add diffusive and specular components to the final color
      // N.B. ): in this implementation, the sum of the components can be different than 1
      color += vec3( Kd * lambertian * diffuseColor +
                      Ks * specular * specularColor);
    }
    return color;
}
//////////////////////////////////////////

//////////////////////////////////////////
// Schlick-GGX method for geometry obstruction (used by GGX model)
float G1(float angle, float alpha)
{
    // in case of Image Based Lighting, the k factor is different:
    // usually it is set as k=(alpha*alpha)/2
    float r = (alpha + 1.0);
    float k = (r*r) / 8.0;

    float num   = angle;
    float denom = angle * (1.0 - k) + k;

    return num / denom;
}

//////////////////////////////////////////
// a subroutine for the GGX model
subroutine(ill_model)
vec3 GGX(vec3 diffuseColor) // this name is the one which is detected by the SetupShaders() function in the main application, and the one used to swap subroutines
{
    // normalization of the per-fragment normal
    vec3 N = normalize(Get_Normal());
    // normalization of the per-fragment light incidence direction
    vec3 L = normalize(lightDir.xyz);

    // cosine angle between direction of light and normal
    float NdotL = max(dot(N, L), 0.0);

    // diffusive (Lambert) reflection component
    vec3 lambert = (Kd * diffuseColor) / PI;

    // we initialize the specular component
    vec3 specular = vec3(0.0);

    // if the cosine of the angle between direction of light and normal is positive, then I can calculate the specular component
    if(NdotL > 0.0)
    {
        // the view vector has been calculated in the vertex shader, already negated to have direction from the mesh to the camera
        vec3 V = normalize( vViewPosition );

        // half vector
        vec3 H = normalize(L + V);

        // we implement the components seen in the slides for a PBR BRDF
        // we calculate the cosines and parameters to be used in the different components
        float NdotH = max(dot(N, H), 0.0);
        float NdotV = max(dot(N, V), 0.0);
        float VdotH = max(dot(V, H), 0.0);
        float alpha_Squared = alpha * alpha;
        float NdotH_Squared = NdotH * NdotH;

        // Geometric factor G2
        // Smith’s method (uses Schlick-GGX method for both geometry obstruction and shadowing )
        float G2 = G1(NdotV, alpha)*G1(NdotL, alpha);

        // Rugosity D
        // GGX Distribution
        float D = alpha_Squared;
        float denom = (NdotH_Squared*(alpha_Squared-1.0)+1.0);
        D /= PI*denom*denom;

        // Fresnel reflectance F (approx Schlick)
        vec3 F = vec3(pow(1.0 - VdotH, 5.0));
        F *= (1.0 - F0);
        F += F0;

        // we put everything together for the specular component
        specular = (F * G2 * D) / (4.0 * NdotV * NdotL);
    }

    // the rendering equation is:
    // integral of: BRDF * Li * (cosine angle between N and L)
    // BRDF in our case is: the sum of Lambert and GGX
    // Li is considered as equal to 1: light is white, and we have not applied attenuation. With colored lights, and with attenuation, the code must be modified and the Li factor must be multiplied to finalColor
    return (lambert + specular)*NdotL;
}
//////////////////////////////////////////

// get the normal from the mesh
subroutine(get_normal)
vec3 NormalMatrix() {
  return normalize(normalMatrix * vNormal);
}

subroutine(get_normal)
vec3 NormalMap() {
  vec2 repeated_UV = mod(interp_UV * repeat, 1.0);
  vec3 color = texture(normalMap, repeated_UV).rgb;
  vec3 sampledNormal = normalize(2 * color - vec3(1, 1, 1));
  vec3 T = normalize(vec3(normalMatrix * vTangent));
  vec3 B = normalize(vec3(normalMatrix * vBitangent));
  vec3 N = normalize(vec3(normalMatrix * vNormal));
  mat3 TBN = mat3(T, B, N);
  return normalize(TBN * sampledNormal);
}

//////////////////////////////////////////

subroutine(color_pattern)
vec3 SingleColor() {
    return color1;
}

subroutine(color_pattern)
vec3 Texture() {
    vec2 repeated_UV = mod(interp_UV * repeat, 1.0);
    return texture(tex, repeated_UV).rgb;
}

subroutine(color_pattern)
vec3 Grid() {
    // s coordinates -> from 0.0 to 1.0
    // multiplied for "repeat" -> from 0.0 to "repeat"
    float repeat = 50.0;
    // fractional part gives the repetition of a gradient from 0.0 to 1.0 for "repeat" times
    vec2 k = fract(interp_UV * repeat);
    // step function forces half pattern to white and the other half to black
    // we use this value to mix the colors, to obtain a colored stripes pattern
    // A LOT OF ALIASING!
    vec2 st = step(0.5, k);
    // TODO: kinda hacky, refere to regular_patterns shader
    // xor pattern
    float factor = (st.x * st.y) + ((1 - st.x) * (1 - st.y));
    return factor * color1 + (1 - factor) * color2;
}


// main
void main(void)
{
    // we call the pointer function Illumination_Model():
    // the subroutine selected in the main application will be called and executed
  	vec3 diffuseColor = Color_Pattern();
  	
    vec3 color = Illumination_Model(diffuseColor);
    // DEBUG: normals
    // color = Get_Normal();

    colorFrag = vec4(color, 1.0);
}
