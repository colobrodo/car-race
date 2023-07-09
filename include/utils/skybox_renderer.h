#pragma once

#include "skybox_texture.h";
#include "renderer.h";
#include "shader.h";

class SkyboxRenderer : public Renderer {
public:
    SkyboxRenderer() {
        shader = new Shader("skybox.vert", "skybox.frag");
    }

    void SetTexture(SkyboxTexture &texture) {
        auto textureLocation = glGetUniformLocation(shader->Program, "tex");
        texture.SendToShader(textureLocation);
    }
};