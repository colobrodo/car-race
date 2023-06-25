#pragma once

#include "../glm/glm.hpp"
#include "../glm/gtc/matrix_inverse.hpp"
#include "../glm/gtc/type_ptr.hpp"


#include "texture.h";
#include "shader.h";

enum PatternType {
    COLOR,
    GRID,
    TEXTURE,
};

enum NormalCalculation {
    NORMAL_MAP,
    FROM_MATRIX,
};

struct IlluminationModelParameter {
    // point light position
    glm::vec3 lightPosition;
    // weight for the diffusive component
    float Kd;
    // Fresnel reflectance at 0 degree (Schlik's approximation)
    float F0;
    // roughness index for GGX shader
    float alpha;
};

class MeshRenderer {
public:
    MeshRenderer() {
        // the Shader Program for the objects used in the application
        shader = new Shader("09_illumination_models.vert", "10_illumination_models.frag");
        // fragment shader subroutines
        // set the illumination model to GGX, look a the comment about subroutines
        GLuint illuminationModelIndex = glGetSubroutineIndex(shader->Program, GL_FRAGMENT_SHADER, "GGX");
        subroutines[2] = illuminationModelIndex;
        SetPattern(COLOR);
        SetNormalCalculation(FROM_MATRIX);
    }

    void Activate(glm::mat4 viewMatrix, glm::mat4 projection) {
        // save the view matrix to later calculate the normal matrix to be passed to the shader
        view = viewMatrix;
        // activates the shader and instantiate the view and projection matrix for the scene
        shader->Use();
        glUniformMatrix4fv(glGetUniformLocation(shader->Program, "projectionMatrix"), 1, GL_FALSE, glm::value_ptr(projection));
        glUniformMatrix4fv(glGetUniformLocation(shader->Program, "viewMatrix"), 1, GL_FALSE, glm::value_ptr(view));
        UpdateIlluminationModel();
    }

    void SetModelTrasformation(glm::mat4 modelMatrix) {
        glm::mat3 normalMatrix = glm::inverseTranspose(glm::mat3(view * modelMatrix));
        glUniformMatrix4fv(glGetUniformLocation(shader->Program, "modelMatrix"), 1, GL_FALSE, glm::value_ptr(modelMatrix));
        glUniformMatrix3fv(glGetUniformLocation(shader->Program, "normalMatrix"), 1, GL_FALSE, glm::value_ptr(normalMatrix));
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
        auto textureLocation = glGetUniformLocation(shader->Program, "tex");
        texture.Activate(textureLocation);
    }

    void SetNormalMap(Texture &texture) {
        SetNormalCalculation(NORMAL_MAP);
        auto normalMapLocation = glGetUniformLocation(shader->Program, "normalMap");
        texture.Activate(normalMapLocation);
    }

    void Delete() {
        shader->Delete();
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
        GLuint patternSubroutine = glGetSubroutineIndex(shader->Program, GL_FRAGMENT_SHADER, subroutineName);
        subroutines[1] = patternSubroutine;
        // we activate the subroutine using the index (this is where shaders swapping happens)
        glUniformSubroutinesuiv(GL_FRAGMENT_SHADER, 3, subroutines);
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
        GLuint patternSubroutine = glGetSubroutineIndex(shader->Program, GL_FRAGMENT_SHADER, subroutineName);
        subroutines[0] = patternSubroutine;
        // we activate the subroutine using the index (this is where shaders swapping happens)
        glUniformSubroutinesuiv(GL_FRAGMENT_SHADER, 3, subroutines);

    }

    IlluminationModelParameter illumination;
    // TODO: set as private
    Shader *shader;
private:
    void UpdateIlluminationModel() {
        GLint pointLightLocation = glGetUniformLocation(shader->Program, "pointLightPosition");
        GLint kdLocation = glGetUniformLocation(shader->Program, "Kd");
        GLint alphaLocation = glGetUniformLocation(shader->Program, "alpha");
        GLint f0Location = glGetUniformLocation(shader->Program, "F0");
        // we assign the values of the illumination model to the uniform variable
        glUniform3fv(pointLightLocation, 1, glm::value_ptr(illumination.lightPosition));
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
    GLuint subroutines[3] = {0, 0, 0};
};