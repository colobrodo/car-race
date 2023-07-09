#pragma once

#include "./shader.h"
#include "./object_renderer.h"

#include "../glm/gtc/type_ptr.hpp"


class GrassRenderer: public ObjectRenderer {
    public:
    GrassRenderer() {
        shader = new Shader("2D_object.vert", "2D_object.frag");
    }

    virtual void SetTexture(Texture &texture, float repeat=1.f) {
        auto textureLocation = glGetUniformLocation(shader->Program, "tex");
        texture.SendToShader(textureLocation);
    }

    void SetShadowMap(GLuint texture, glm::mat4 lightView, glm::mat4 lightProjection) {
        glUniformMatrix4fv(glGetUniformLocation(shader->Program, "lightView"), 1, GL_FALSE, glm::value_ptr(lightView));
        glUniformMatrix4fv(glGetUniformLocation(shader->Program, "lightProjection"), 1, GL_FALSE, glm::value_ptr(lightProjection));
        auto shadowMapLocation = glGetUniformLocation(shader->Program, "shadowMap");
        glActiveTexture(GL_TEXTURE0 + texture + 2);
        glBindTexture(GL_TEXTURE_2D, texture);
        glUniform1i(shadowMapLocation, texture + 2);
    }

    // no colors, normals or parallaxmap used for 2D texture object, 
    // only depth buffer is used
    virtual void SetColor(glm::vec3 color) { };
    
    virtual void SetNormalMap(Texture &texture) { };

    virtual void SetParallaxMap(Texture &texture, float scale=1.f) { };

    virtual void SetPattern(PatternType pattern) { };

    virtual void SetNormalCalculation(NormalCalculation normalCalculation) { };

    virtual void SetTexCoordinateCalculation(TextureCoordinateCalculation calculation) { };

};