#pragma once

#include "./object_renderer.h";
#include "./model.h";
#include "./physics.h";

class Obstacle {
public:
    Obstacle(Model *model, glm::vec3 pos, glm::vec3 dim=glm::vec3(1.f, 1.f, 1.f)): Model(model), Position(pos), Dimension(dim) {
        glm::vec3 rot = glm::vec3(0.0f, 0.0f, 0.0f);
        auto &simulation = Physics::GetInstance();
        // create one rigid body for mesh
        for(auto &mesh: Model->meshes) {
            auto rigidBody = simulation.createRigidBodyFromMesh(mesh, pos, rot, dim);
            rigidBodies.push_back(rigidBody);
        }
    }

    void Draw(ObjectRenderer &renderer) {
        float matrix[16];
        btTransform transform;
        renderer.UpdateIlluminationModel(Illumination);
        for(auto rigidBody: rigidBodies) {
            rigidBody->getMotionState()->getWorldTransform(transform);
            transform.getOpenGLMatrix(matrix);
            auto modelMatrix = glm::make_mat4(matrix);
            modelMatrix = glm::scale(modelMatrix, Dimension);
            renderer.SetModelTrasformation(modelMatrix);
            Model->Draw();
        }
    }

    Obstacle(const Obstacle& copy) = delete; //disallow copy
    Obstacle& operator=(const Obstacle &) = delete;

    IlluminationModelParameters Illumination;
    // model is deallocated by the main application: you can have obstacle of 
    // the same type, so with the same model
    Model *Model;
    glm::vec3 Position;
    glm::vec3 Dimension;

private:
    // bullet object created by the physics class are deallocated by the world
    // object without the necessity to do that here
    std::vector<btRigidBody*> rigidBodies;
};