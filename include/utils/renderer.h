#pragma once

#include <glad/glad.h>

#include "../glm/glm.hpp"
#include "../glm/gtc/type_ptr.hpp"
#include "../glm/gtc/matrix_inverse.hpp"

#include "./shader.h"


class Renderer {
public:
    void Activate(glm::mat4 viewMatrix, glm::mat4 projection) {
        // save the view matrix to later calculate the normal matrix to be passed to the shader
        view = viewMatrix;
        // activates the shader and instantiate the view and projection matrix for the scene
        shader->Use();
        glUniformMatrix4fv(glGetUniformLocation(shader->Program, "projectionMatrix"), 1, GL_FALSE, glm::value_ptr(projection));
        glUniformMatrix4fv(glGetUniformLocation(shader->Program, "viewMatrix"), 1, GL_FALSE, glm::value_ptr(view));
    }
    
    void Delete() {
        shader->Delete();
    }

protected:
    glm::mat4 view;
    Shader *shader;
};