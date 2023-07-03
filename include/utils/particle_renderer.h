#pragma once

#include <glad/glad.h>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include "./geometry.h"
#include "./renderer.h"

#include "./particle.h"

enum ParticleShape {
    CIRCLE,
    SQUARE,
};

class ParticleRenderer : public Renderer {
public:
    ParticleRenderer(ParticleEmitter &particleEmitter): emitter(particleEmitter) {
        // the Shader Program for rendering the particles
        shader = new Shader("particle.vert", "particle.frag");
        
        auto size = emitter.size();
        particleColors = new glm::vec4[size];
        modelMatrices = new glm::mat4[size];
        for(int i = 0; i < size; i++) {
            modelMatrices[i] = glm::mat4(1.0f);
        }

        // creating particle VAO/VBO, different from normal quads cause of instancing
        GLuint particleVBO;
        glGenVertexArrays(1, &particleVAO);
        glGenBuffers(1, &particleVBO);
        glBindVertexArray(particleVAO);
        glBindBuffer(GL_ARRAY_BUFFER, particleVBO);
        glBufferData(GL_ARRAY_BUFFER, sizeof(quadVertices), &quadVertices, GL_STATIC_DRAW);
        glEnableVertexAttribArray(0);
        glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)0);
        glEnableVertexAttribArray(1);
        glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)(2 * sizeof(float)));
        // vertex buffer object
        glGenBuffers(1, &modelMatrixBuffer);
        glBindBuffer(GL_ARRAY_BUFFER, modelMatrixBuffer);
        bufferSize = size * sizeof(glm::mat4);
        glBufferData(GL_ARRAY_BUFFER, bufferSize, &modelMatrices[0], GL_STATIC_DRAW);
        // vertex attributes
        // we need to allocate space for each matrix4 trasformation, we should do it row by row
        auto vec4Size = sizeof(glm::vec4);
        glEnableVertexAttribArray(2); 
        glVertexAttribPointer(2, 4, GL_FLOAT, GL_FALSE, 4 * vec4Size, (void*)0);
        glEnableVertexAttribArray(3); 
        glVertexAttribPointer(3, 4, GL_FLOAT, GL_FALSE, 4 * vec4Size, (void*)(1 * vec4Size));
        glEnableVertexAttribArray(4); 
        glVertexAttribPointer(4, 4, GL_FLOAT, GL_FALSE, 4 * vec4Size, (void*)(2 * vec4Size));
        glEnableVertexAttribArray(5); 
        glVertexAttribPointer(5, 4, GL_FLOAT, GL_FALSE, 4 * vec4Size, (void*)(3 * vec4Size));
        // buffer for color particle
        glGenBuffers(1, &particleColorBuffer);
        glBindBuffer(GL_ARRAY_BUFFER, particleColorBuffer);
        colorBufferSize = size * sizeof(glm::vec4);
        glBufferData(GL_ARRAY_BUFFER, bufferSize, &modelMatrices[0], GL_STATIC_DRAW);
        // attrib array for particles color
        glEnableVertexAttribArray(6); 
        glVertexAttribPointer(6, 4, GL_FLOAT, GL_FALSE, vec4Size, (void*)0);
        glVertexAttribDivisor(2, 1);
        glVertexAttribDivisor(3, 1);
        glVertexAttribDivisor(4, 1);
        glVertexAttribDivisor(5, 1);
        glVertexAttribDivisor(6, 1);

        glBindVertexArray(0);
    }

    void Draw() {
        // Model transformation matrix for the objects in the scene: we set to identity
        glm::mat4 objModelMatrix = glm::mat4(1.0f);
        for(int i = 0; i < emitter.size(); i++) {
            auto &particle = emitter.particles[i];
            // setting the color of the particle
            glm::vec4 color(particle.color, particle.alpha);
            particleColors[i] = color;

            // scaling the particle according to his variable
            auto particleSize = particle.scale * particle.size;
            auto transform = glm::translate(glm::mat4(1.f), particle.position);

            objModelMatrix = glm::rotate(glm::mat4(1.f), particle.rotation, glm::vec3(0, 1, 0));
            objModelMatrix = glm::scale(objModelMatrix, particleSize);
            // we reset to identity at each frame
            modelMatrices[i] = transform * objModelMatrix;
        }
        // write each particle information (model-matrix and color) in the buffer
        glBindBuffer(GL_ARRAY_BUFFER, modelMatrixBuffer);
        glBufferSubData(GL_ARRAY_BUFFER, 0, bufferSize, &modelMatrices[0]);
        glBindBuffer(GL_ARRAY_BUFFER, particleColorBuffer);
        glBufferSubData(GL_ARRAY_BUFFER, 0, colorBufferSize, &particleColors[0]);
        // draw all particles in one single call
        glBindVertexArray(particleVAO);
        glDrawArraysInstanced(GL_TRIANGLES, 0, 6, emitter.size());
        glBindVertexArray(0);
    }

    void SetParticleShape(ParticleShape shape) {
        char *subroutineName;
        switch (shape)
        {
        case CIRCLE:
            subroutineName = "Circle";
            break;
        case SQUARE:
            subroutineName = "Square";
            break;
        }
        setParticleShapeSubroutine(subroutineName);
    }

    void Delete() {
        Renderer::Delete();
        delete[] particleColors;
        delete[] modelMatrices;
    }

private:
    // openGL buffer indeces
    GLuint particleVAO;
    // buffer for storing color particles
    size_t colorBufferSize;
    GLuint particleColorBuffer;
    // buffer for storing particle trasformations
    GLuint modelMatrixBuffer;
    size_t bufferSize;

    glm::vec4 *particleColors;
    glm::mat4 *modelMatrices;
    ParticleEmitter &emitter;
    
    // only needed subroutine for particle shape, no need for array
    GLuint subroutine = 0;

    void setParticleShapeSubroutine(char *subroutineName) {
        GLuint patternSubroutine = glGetSubroutineIndex(shader->Program, GL_FRAGMENT_SHADER, subroutineName);
        subroutine = patternSubroutine;
        // we activate the subroutine using the index (this is where shaders swapping happens)
        glUniformSubroutinesuiv(GL_FRAGMENT_SHADER, 1, &subroutine);
    }

};