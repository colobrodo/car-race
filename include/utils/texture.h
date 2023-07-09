#pragma once

#include "../glad/glad.h"


class Texture {
public:
    inline void Activate() {
        glActiveTexture(GL_TEXTURE0 + Id);
        glBindTexture(GL_TEXTURE_2D, textureImage);
    }

    void SendToShader(GLint shaderLocation) {
        Activate();
        glUniform1i(shaderLocation, Id);
    }

protected:
    int GetId() {
        Id = Texture::currentId;
        Texture::currentId++;
        return Id;
    }

    int Id;
    GLuint textureImage;
    static int currentId; 
};

// HACK: currentId starts from 1 to avoid postprocessing texture
int Texture::currentId = 1;
