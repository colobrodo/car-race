#pragma once

#include <iostream>
#include <vector>

#include "../glad/glad.h"

// we include the library for images loading
#ifndef STB_IMAGE_IMPLEMENTATION
#define STB_IMAGE_IMPLEMENTATION
#include "../stb_image/stb_image.h"
#endif

#include "texture.h"

// the skybox texture class take as parameter a path to a folder of texture.
// this folder should contains the following images:
//  - px.jpg (positive x)
//  - nx.jpg (negative x)
//  - py.jpg (positive y)
//  - ny.jpg (negative y)
//  - pz.jpg (positive z)
//  - nz.jpg (negative z)
// note this images should be in the jpg format
class SkyboxTexture: public Texture {
public:
    SkyboxTexture(char *skyboxFolder, bool flip=false) {
        glGenTextures(1, &textureImage);
        glBindTexture(GL_TEXTURE_CUBE_MAP, textureImage);

        // we listed all the 6 images in the folder in order 
        std::vector<char *> textureImages{"px.jpg", "nx.jpg", "py.jpg", "ny.jpg", "pz.jpg", "nz.jpg"};
        int width, height, nChannels;
        unsigned char *data;  
        stbi_set_flip_vertically_on_load(flip);
        for(int i = 0; i < textureImages.size(); i++) {
            std::string textureName = textureImages[i];
            auto texturePath = skyboxFolder + textureName;
            
            data = stbi_load(texturePath.c_str(), &width, &height, &nChannels, 0);
            if(!data) {
                std::cout << "Error: cannot load cubemap texture: '" << texturePath << "'!" << std::endl;
            } else {
                glTexImage2D(
                    GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, 
                    0, GL_RGB, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, data
                );
            }
            // we free the memory once we have created an OpenGL texture
            stbi_image_free(data);
        }
        // we set how to consider UVs outside [0,1] range (shouldn't happen)
        glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);  
        // we set the filtering for minification and magnification
        glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR);

        // we set the binding to 0 once we have finished
        glBindTexture(GL_TEXTURE_CUBE_MAP, 0);

        // get the Id of the texture, needed for active texture
        Id = GetId();
    }

    void Activate() {
        glActiveTexture(GL_TEXTURE0 + Id);
        // override in order to support different texture binding type
        glBindTexture(GL_TEXTURE_CUBE_MAP, textureImage);
    }

    void SendToShader(GLint shaderLocation) {
        Activate();
        glUniform1i(shaderLocation, Id);
    }
};

