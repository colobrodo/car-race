#pragma once

#include "../glm/glm.hpp"

#include "texture.h"

enum PatternType {
    COLOR,
    GRID,
    TEXTURE,
};

enum NormalCalculation {
    NORMAL_MAP,
    FROM_MATRIX,
};

enum TextureCoordinateCalculation {
    UV,
    PARALLAX_MAP,
};

class ObjectRenderer {
    public:
    void Activate(glm::mat4 viewMatrix, glm::mat4 projection) {
        // save the view matrix to later calculate the normal matrix to be passed to the shader
        view = viewMatrix;
        // activates the shader and instantiate the view and projection matrix for the scene
        shader->Use();
        glUniformMatrix4fv(glGetUniformLocation(shader->Program, "projectionMatrix"), 1, GL_FALSE, glm::value_ptr(projection));
        glUniformMatrix4fv(glGetUniformLocation(shader->Program, "viewMatrix"), 1, GL_FALSE, glm::value_ptr(view));
    }
    
    void SetModelTrasformation(glm::mat4 modelMatrix) {
        glm::mat3 normalMatrix = glm::inverseTranspose(glm::mat3(view * modelMatrix));
        glUniformMatrix4fv(glGetUniformLocation(shader->Program, "modelMatrix"), 1, GL_FALSE, glm::value_ptr(modelMatrix));
        glUniformMatrix3fv(glGetUniformLocation(shader->Program, "normalMatrix"), 1, GL_FALSE, glm::value_ptr(normalMatrix));
    }

    virtual void SetColor(glm::vec3 color) = 0;
    
    virtual void SetTexture(Texture &texture, float repeat=1.f) = 0;
    
    virtual void SetNormalMap(Texture &texture) = 0;

    virtual void SetParallaxMap(Texture &texture, float scale=1.f) = 0;

    virtual void SetPattern(PatternType pattern) = 0;

    virtual void SetNormalCalculation(NormalCalculation normalCalculation) = 0;

    virtual void SetTexCoordinateCalculation(TextureCoordinateCalculation calculation) = 0;
    
    void Delete() {
        shader->Delete();
    }

protected:
    glm::mat4 view;
    Shader *shader;
};