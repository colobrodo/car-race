#pragma once

#include <iostream>

#include "../glad/glad.h"

// we include the library for images loading
#define STB_IMAGE_IMPLEMENTATION
#include "../stb_image/stb_image.h"


class Texture {
public:
    Texture(char *path, bool flip=false) {
        // we load the image from disk and we create an OpenGL texture
        int w, h, channels;
        unsigned char* image;
        if(flip) stbi_set_flip_vertically_on_load(true);
        image = stbi_load(path, &w, &h, &channels, STBI_default);

        if (image == nullptr)
            std::cout << "Failed to load texture " << path << "!" << std::endl;

        glGenTextures(1, &textureImage);
        glBindTexture(GL_TEXTURE_2D, textureImage);
        // 3 channels = RGB ; 4 channel = RGBA
        if (channels == 3)
            glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, w, h, 0, GL_RGB, GL_UNSIGNED_BYTE, image);
        else if (channels == 4)
            glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, w, h, 0, GL_RGBA, GL_UNSIGNED_BYTE, image);
        glGenerateMipmap(GL_TEXTURE_2D);
        // we set how to consider UVs outside [0,1] range
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
        // we set the filtering for minification and magnification
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST_MIPMAP_NEAREST);

        // we free the memory once we have created an OpenGL texture
        stbi_image_free(image);

        // we set the binding to 0 once we have finished
        glBindTexture(GL_TEXTURE_2D, 0);

        Id = currentId;
        currentId++;
    }

    void Activate(GLint shaderLocation) {
        glActiveTexture(GL_TEXTURE0 + Id);
        glBindTexture(GL_TEXTURE_2D, textureImage);
        glUniform1i(shaderLocation, Id);
    }

private:
    int Id;
    GLuint textureImage;
    static int currentId; 
};

// HACK: currentId starts from 1 to avoid postprocessing texture
int Texture::currentId = 1;
