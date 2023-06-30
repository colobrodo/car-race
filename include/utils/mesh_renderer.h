#pragma once

#include "../glm/glm.hpp"
#include "../glm/gtc/matrix_inverse.hpp"
#include "../glm/gtc/type_ptr.hpp"

#include "object_renderer.h";
#include "texture.h";
#include "shader.h";

struct IlluminationModelParameter {
    // point light position
    glm::vec3 lightDirection;
    // weight for the diffusive component
    float Kd;
    // Fresnel reflectance at 0 degree (Schlik's approximation)
    float F0;
    // roughness index for GGX shader
    float alpha;
};


class MeshRenderer: public ObjectRenderer {
public:
    MeshRenderer() {
        // the Shader Program for the objects used in the application
        shader = new Shader("09_illumination_models.vert", "10_illumination_models.frag");
        // shader = new Shader("debug_shadowmap.vert", "debug_shadowmap.frag");
        // fragment shader subroutines
        // set the illumination model to GGX, look a the comment about subroutines
        GLuint illuminationModelIndex = glGetSubroutineIndex(shader->Program, GL_FRAGMENT_SHADER, "GGX");
        subroutines[3] = illuminationModelIndex;
        SetPattern(COLOR);
        SetTexCoordinateCalculation(UV);
        SetNormalCalculation(FROM_MATRIX);
    }

    void Activate(glm::mat4 viewMatrix, glm::mat4 projection) {
        ObjectRenderer::Activate(viewMatrix, projection);
        UpdateIlluminationModel();
    }

    void SetTerrainTexture(Texture &dirtTexture, Texture &dirtNormalMap,
                           Texture &grassTexture, Texture &grassNormalMap) {
        // two texture with normal map interpolated in a noise pattern
        // not accessible from the extern
        setPatternSubroutine("InterpolatedTextures");
        auto textureLocation = glGetUniformLocation(shader->Program, "tex1");
        dirtTexture.Activate(textureLocation);
        textureLocation = glGetUniformLocation(shader->Program, "tex2");
        grassTexture.Activate(textureLocation);
        setNormalSubroutine("InterpolatedNormalMaps");
        auto normalMapLocation = glGetUniformLocation(shader->Program, "normalMap1");
        dirtNormalMap.Activate(normalMapLocation);
        normalMapLocation = glGetUniformLocation(shader->Program, "normalMap2");
        grassNormalMap.Activate(normalMapLocation);
    }

    void SetColor(glm::vec3 color) {
        // restore the pattern in the shader to fill with single color
        SetPattern(COLOR);
        // we determine the position in the Shader Program of the uniform variables
        GLint color1Location = glGetUniformLocation(shader->Program, "color1");
        glUniform3fv(color1Location, 1, glm::value_ptr(color));
    }

    void SetGrid(glm::vec3 color1, glm::vec3 color2) {
        // restore the pattern in the shader to use a grid pattern
        SetPattern(GRID);
        // we determine the position in the Shader Program of the uniform variables
        GLint color1Location = glGetUniformLocation(shader->Program, "color1");
        GLint color2Location = glGetUniformLocation(shader->Program, "color2");
        glUniform3fv(color1Location, 1, glm::value_ptr(color1));
        glUniform3fv(color2Location, 1, glm::value_ptr(color2));
    }

    void SetTexture(Texture &texture) {
        SetPattern(TEXTURE);
        auto textureLocation = glGetUniformLocation(shader->Program, "tex1");
        texture.Activate(textureLocation);
    }

    void SetNormalMap(Texture &texture) {
        SetNormalCalculation(NORMAL_MAP);
        auto normalMapLocation = glGetUniformLocation(shader->Program, "normalMap1");
        texture.Activate(normalMapLocation);
    }
    
    // TODO: pass scale and maybe bias
    void SetParallaxMap(Texture &texture, float scale=1.f) {
        SetTexCoordinateCalculation(PARALLAX_MAP);
        auto parallaxMapLocation = glGetUniformLocation(shader->Program, "parallaxMap");
        auto parallaxMapScaleLocation = glGetUniformLocation(shader->Program, "parallaxMappingScale");
        glUniform1f(parallaxMapScaleLocation, scale);
        texture.Activate(parallaxMapLocation);
    }

    void SetShadowMap(GLuint texture, glm::mat4 lightView, glm::mat4 lightProjection) {
        glUniformMatrix4fv(glGetUniformLocation(shader->Program, "lightView"), 1, GL_FALSE, glm::value_ptr(lightView));
        glUniformMatrix4fv(glGetUniformLocation(shader->Program, "lightProjection"), 1, GL_FALSE, glm::value_ptr(lightProjection));
        auto shadowMapLocation = glGetUniformLocation(shader->Program, "shadowMap");
        glActiveTexture(GL_TEXTURE0 + texture + 2);
        glBindTexture(GL_TEXTURE_2D, texture);
        glUniform1i(shadowMapLocation, texture + 2);
    }

    void SetPattern(PatternType pattern) {
        char *subroutineName;
        switch(pattern) {
            case COLOR: {
                subroutineName = "SingleColor";
                break;
            }
            case TEXTURE: {
                subroutineName = "Texture";
                break;
            }
            case GRID: {
                subroutineName = "Grid";
                break;
            }
        }
        setPatternSubroutine(subroutineName);
    }

    void SetNormalCalculation(NormalCalculation normalCalculation) {
        char *subroutineName;
        switch(normalCalculation) {
            case FROM_MATRIX: {
                subroutineName = "NormalMatrix";
                break;
            }
            case NORMAL_MAP: {
                subroutineName = "NormalMap";
                break;
            }
        }
        setNormalSubroutine(subroutineName);
    }

    void SetTexCoordinateCalculation(TextureCoordinateCalculation calculation) {
        char *subroutineName;
        switch(calculation) {
            case UV: {
                subroutineName = "InterpolatedUV";
                break;
            }
            case PARALLAX_MAP: {
                subroutineName = "ParallaxMapping";
                break;
            }
        }
        setTexCoordinateSubroutine(subroutineName);
    }

    IlluminationModelParameter illumination;
private:
    void setPatternSubroutine(char *subroutineName) {
        GLuint patternSubroutine = glGetSubroutineIndex(shader->Program, GL_FRAGMENT_SHADER, subroutineName);
        subroutines[0] = patternSubroutine;
        // we activate the subroutine using the index (this is where shaders swapping happens)
        glUniformSubroutinesuiv(GL_FRAGMENT_SHADER, 4, subroutines);
    }
    
    void setNormalSubroutine(char *subroutineName) {
        GLuint patternSubroutine = glGetSubroutineIndex(shader->Program, GL_FRAGMENT_SHADER, subroutineName);
        subroutines[1] = patternSubroutine;
        // we activate the subroutine using the index (this is where shaders swapping happens)
        glUniformSubroutinesuiv(GL_FRAGMENT_SHADER, 4, subroutines);
    }
    
    void setTexCoordinateSubroutine(char *subroutineName) {
        GLuint patternSubroutine = glGetSubroutineIndex(shader->Program, GL_FRAGMENT_SHADER, subroutineName);
        subroutines[2] = patternSubroutine;
        // we activate the subroutine using the index (this is where shaders swapping happens)
        glUniformSubroutinesuiv(GL_FRAGMENT_SHADER, 4, subroutines);
    }

    void UpdateIlluminationModel() {
        GLint lightDirLocation = glGetUniformLocation(shader->Program, "lightVector");
        GLint kdLocation = glGetUniformLocation(shader->Program, "Kd");
        GLint alphaLocation = glGetUniformLocation(shader->Program, "alpha");
        GLint f0Location = glGetUniformLocation(shader->Program, "F0");
        // we assign the values of the illumination model to the uniform variable
        glUniform3fv(lightDirLocation, 1, glm::value_ptr(illumination.lightDirection));
        glUniform1f(kdLocation, illumination.Kd);
        glUniform1f(alphaLocation, illumination.alpha);
        glUniform1f(f0Location, illumination.F0);
    }

    glm::mat4 view;
    // the first element is the pattern subroutine
    // the second is the normal subroutine: we can choose if 
    //   calculate the normal from the view/model matrix or use a normal map
    //   (automaticly setted when setted a normal map)
    // the last is illumination model, always setted to GGX
    GLuint subroutines[4] = {0, 0, 0, 0};
};