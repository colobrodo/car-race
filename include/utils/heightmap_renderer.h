#pragma once

#include <vector>

#include "../glm/glm.hpp"
#include "../glm/gtc/matrix_inverse.hpp"
#include "../glm/gtc/type_ptr.hpp"

#include "object_renderer.h";
#include "texture.h";
#include "shader.h";


// we are drawing a quad for heightmap so the patch cardinality is 4
constexpr int NUM_PATCH_POINTS = 4;

class Heightmap {
public:
    Heightmap(float y, float width, float height, float depth): width(width), height(height), depth(depth), y(y) {
        std::vector<float> vertices;

        resolution = 20;
        float rez = resolution;
        for(unsigned i = 0; i <= rez-1; i++)
        {
            for(unsigned j = 0; j <= rez-1; j++)
            {
                vertices.push_back(-width / 2.0f + width * i / (float)rez); // v.x
                vertices.push_back(0); // v.y
                vertices.push_back(-height / 2.0f + height * j / (float)rez); // v.z
                vertices.push_back(i / (float)rez); // u
                vertices.push_back(j / (float)rez); // v

                vertices.push_back(-width / 2.0f + width * (i+1) / (float)rez); // v.x
                vertices.push_back(0); // v.y
                vertices.push_back(-height / 2.0f + height * j / (float)rez); // v.z
                vertices.push_back((i+1) / (float)rez); // u
                vertices.push_back(j / (float)rez); // v

                vertices.push_back(-width / 2.0f + width * i / (float)rez); // v.x
                vertices.push_back(0); // v.y
                vertices.push_back(-height / 2.0f + height * (j+1) / (float)rez); // v.z
                vertices.push_back(i / (float)rez); // u
                vertices.push_back((j+1) / (float)rez); // v

                vertices.push_back(-width / 2.0f + width * (i+1) / (float)rez); // v.x
                vertices.push_back(0); // v.y
                vertices.push_back(-height / 2.0f + height * (j+1) / (float)rez); // v.z
                vertices.push_back((i+1) / (float)rez); // u
                vertices.push_back((j+1) / (float)rez); // v
            }
        }

        // first, configure the cube's VAO (and terrainVBO)
        glGenVertexArrays(1, &VAO);
        glBindVertexArray(VAO);

        glGenBuffers(1, &VBO);
        glBindBuffer(GL_ARRAY_BUFFER, VBO);
        glBufferData(GL_ARRAY_BUFFER, sizeof(float) * vertices.size(), &vertices[0], GL_STATIC_DRAW);

        // position attribute
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)0);
        glEnableVertexAttribArray(0);
        // texCoord attribute
        glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)(sizeof(float) * 3));
        glEnableVertexAttribArray(1);

        // cause we are drawing quads
        glPatchParameteri(GL_PATCH_VERTICES, NUM_PATCH_POINTS);
    }

    void Draw() {
        glBindVertexArray(VAO);
        glDrawArrays(GL_PATCHES, 0, NUM_PATCH_POINTS * resolution * resolution);
    }

    float y, width, height;
    // the z dimension of the heightmap
    float depth;
    glm::vec2 size;
private:
    float resolution;
    GLuint VAO, VBO;
};

class HeightmapRenderer: public ObjectRenderer {
public:
    HeightmapRenderer(Heightmap heightmap): heightmap(heightmap) {
        // the vertex, tesselation and fragment shader for the heightmap
        shader = new Shader("heightmap.vert", "heightmap.frag", "heightmap.tcs", "heightmap.tes");
        // fragment shader subroutines
        // set the illumination model to GGX, look a the comment about subroutines
        GLuint illuminationModelIndex = glGetSubroutineIndex(shader->Program, GL_FRAGMENT_SHADER, "GGX");
        subroutines[3] = illuminationModelIndex;
        SetPattern(COLOR);
        SetTexCoordinateCalculation(UV);
        SetNormalCalculation(FROM_MATRIX);
    }

    void Activate(glm::mat4 viewMatrix, glm::mat4 projection) {
        ObjectRenderer::Activate(viewMatrix, projection);
        UpdateHeightmapParameters();
    }

    void SetDepthTexture(Texture &texture) {
        auto textureLocation = glGetUniformLocation(shader->Program, "tex");
        texture.SendToShader(textureLocation);
    }

    void SetTexture(Texture &texture, float repeat=1.f) {
        auto repeatLocation = glGetUniformLocation(shader->Program, "repeat");
        glUniform1f(repeatLocation, repeat);
        auto textureLocation = glGetUniformLocation(shader->Program, "imageTex");
        texture.SendToShader(textureLocation);
    }

    void SetNormalMap(Texture &texture) { }
    
    // TODO: pass scale and maybe bias
    void SetParallaxMap(Texture &texture, float scale=1.f) {}

    void SetShadowMap(Texture &texture, glm::mat4 lightView, glm::mat4 lightProjection) {
        glUniformMatrix4fv(glGetUniformLocation(shader->Program, "lightView"), 1, GL_FALSE, glm::value_ptr(lightView));
        glUniformMatrix4fv(glGetUniformLocation(shader->Program, "lightProjection"), 1, GL_FALSE, glm::value_ptr(lightProjection));
        auto shadowMapLocation = glGetUniformLocation(shader->Program, "shadowMap");
        texture.SendToShader(shadowMapLocation);
    }

    void SetLightDirection(glm::vec3 lightDirection) {
        GLint lightDirLocation = glGetUniformLocation(shader->Program, "lightVector");
        glUniform3fv(lightDirLocation, 1, glm::value_ptr(lightDirection));
    }

    void UpdateHeightmapParameters() {
        // heightmap parameters: y position and height
        glUniform1f(glGetUniformLocation(shader->Program, "heightmapHeight"), heightmap.depth);
        glUniform1f(glGetUniformLocation(shader->Program, "heightmapY"), heightmap.y);
    }

    // empty for now: we only use textures
    virtual void SetPattern(PatternType pattern) {};
    void SetColor(glm::vec3 color) {}
    void SetGrid(glm::vec3 color1, glm::vec3 color2) {}

    // no lighting calculation done on heightmap
    void UpdateIlluminationModel(IlluminationModelParameters &illumination) {}

    // unavailable normal calculation (no normal mapping implemented)
    virtual void SetNormalCalculation(NormalCalculation normalCalculation) {};
    // no parallax mapping for heightmaps, empty
    virtual void SetTexCoordinateCalculation(TextureCoordinateCalculation calculation) {};

    Heightmap heightmap;

private:

    glm::mat4 view;
    // the first element is the pattern subroutine
    // the second is the normal subroutine: we can choose if 
    //   calculate the normal from the view/model matrix or use a normal map
    //   (automaticly setted when setted a normal map)
    // the last is illumination model, always setted to GGX
    GLuint subroutines[4] = {0, 0, 0, 0};
};