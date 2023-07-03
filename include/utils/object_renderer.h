#pragma once

#include "../glm/glm.hpp"

#include "./renderer.h"
#include "./texture.h"

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

class ObjectRenderer: public Renderer {
    public:
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
    
};