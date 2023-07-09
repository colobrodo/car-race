#pragma once

#include <glad/glad.h>
#include <string.h>

#include "texture.h"


#include <iostream>

class DepthTexture: public Texture {
public:
    DepthTexture(float width, float height): width(width), height(height) {
        glGenTextures(1, &textureImage);
        glBindTexture(GL_TEXTURE_2D, textureImage);
        Id = GetId();
        
        glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT, 
                    width, height, 0, GL_DEPTH_COMPONENT, GL_FLOAT, NULL);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER); 
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);
        float borderColor[] = { 1.0f, 1.0f, 1.0f, 1.0f };
        glTexParameterfv(GL_TEXTURE_2D, GL_TEXTURE_BORDER_COLOR, borderColor);
    }

    void AttachToFrameBuffer(GLuint frameBuffer) {
        glBindFramebuffer(GL_FRAMEBUFFER, frameBuffer);
        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, textureImage, 0);
        glDrawBuffer(GL_NONE);
        glReadBuffer(GL_NONE);
        glBindFramebuffer(GL_FRAMEBUFFER, 0); 
    }
    
    void CopyFromCurrentFrameBuffer() {
        Activate();
        glCopyTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT,
                         0, 0, width, height,
                         0);
    }

    void Clear() {
        std::vector<GLfloat> emptyData(width * height * 4, 1.f);
        glBindTexture(GL_TEXTURE_2D, textureImage);
        glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, width, height, GL_DEPTH_COMPONENT, GL_FLOAT, &emptyData[0]);
    }
private:
    float width, height;

};