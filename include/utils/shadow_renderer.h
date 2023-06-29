#pragma once

#include "../glm/glm.hpp"
#include "../glm/gtc/type_ptr.hpp"

#include "object_renderer.h"
#include "shader.h"

class ShadowRenderer: public ObjectRenderer {
    public:
    ShadowRenderer() {
        shader = new Shader("shadow_map.vert", "shadow_map.frag");
    }
    
    virtual void SetModelTrasformation(glm::mat4 modelMatrix) {

    };

    // no texture, colors, normals or parallaxmap used for shadow maps, 
    // only depth buffer is used
    virtual void SetColor(glm::vec3 color) { };
    
    virtual void SetTexture(Texture &texture) { };
    
    virtual void SetNormalMap(Texture &texture) { };

    virtual void SetParallaxMap(Texture &texture, float scale=1.f) { };

    virtual void SetPattern(PatternType pattern) { };

    virtual void SetNormalCalculation(NormalCalculation normalCalculation) { };

    virtual void SetTexCoordinateCalculation(TextureCoordinateCalculation calculation) { };

};