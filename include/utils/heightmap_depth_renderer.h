#pragma once

#include "../glm/glm.hpp"
#include "../glm/gtc/type_ptr.hpp"

#include "object_renderer.h"
#include "depth_texture.h"
#include "shader.h"

class HeightmapDepthRenderer: public ObjectRenderer {
public:
    HeightmapDepthRenderer() {
        // this shader save do a min between the depth of this frame and the previous one
        shader = new Shader("heightmap_depth.vert", "heightmap_depth.frag");
    }

    void SetPreviousFrame(DepthTexture &texture) {
        auto textureLocation = glGetUniformLocation(shader->Program, "previousFrame");
        texture.SendToShader(textureLocation);
    }

    void SetViewportSize(glm::vec2 size) {
        GLint viewportSizeLocation = glGetUniformLocation(shader->Program, "viewportSize");
        glUniform2fv(viewportSizeLocation, 1, glm::value_ptr(size));
    }

    // no texture, colors, normals or parallaxmap used for depth buffers
    virtual void SetColor(glm::vec3 color) { };
    
    virtual void SetTexture(Texture &texture, float repeat=1.f) { };
    
    virtual void SetNormalMap(Texture &texture) { };

    virtual void SetParallaxMap(Texture &texture, float scale=1.f) { };

    virtual void SetPattern(PatternType pattern) { };

    virtual void SetNormalCalculation(NormalCalculation normalCalculation) { };

    virtual void SetTexCoordinateCalculation(TextureCoordinateCalculation calculation) { };

    virtual void UpdateIlluminationModel(IlluminationModelParameters &illumination) { };

};