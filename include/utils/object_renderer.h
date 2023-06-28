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
    virtual void Activate(glm::mat4 viewMatrix, glm::mat4 projection) = 0;
    
    virtual void SetModelTrasformation(glm::mat4 modelMatrix) = 0;

    virtual void SetColor(glm::vec3 color) = 0;
    
    virtual void SetTexture(Texture &texture) = 0;
    
    virtual void SetNormalMap(Texture &texture) = 0;

    virtual void Delete() = 0;

    virtual void SetPattern(PatternType pattern) = 0;

    virtual void SetNormalCalculation(NormalCalculation normalCalculation) = 0;

    virtual void SetTexCoordinateCalculation(TextureCoordinateCalculation calculation) = 0;
};