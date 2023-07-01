#pragma once

#include <cmath>
#include <glm/glm.hpp>
// TODO: remove later
#include <vector>

#include "./random.h"

struct Particle
{
    glm::vec3 velocity;
    glm::vec3 acceleration;
    glm::vec3 position;
    glm::vec3 scale;
    float rotation;
    float size;
    float alpha;
    float lifespan = 3.f;
    float lifetime = lifespan;
    glm::vec3 color;
};


class ParticleEmitter {
public:
    ParticleEmitter(int size): Size(size) {
        // push a bunch of dead particles
        particles = new Particle[size];
        for(int i = 0; i < size; i++) {
            Particle particle;
            particle.lifetime = -1;
            particles[i] = particle;
        }
    }

    Particle *particles;
    glm::vec3 Position;

    // random variables to uniformly interpolate between during particle spawn
    glm::vec3 Color0;
    glm::vec3 Color1;
    glm::vec3 Velocity;
    glm::vec3 Gravity;
    glm::vec3 DeltaVelocity0;
    glm::vec3 DeltaVelocity1;
    float Lifespan0;
    float Lifespan1;
    float Size0;
    float Size1;
    glm::vec3 Scale0;
    glm::vec3 Scale1;
    float Rotation0;
    float Rotation1;
    float Alpha0;
    float Alpha1;

    bool Active = true;

    void Update(float deltaTime) {
        for(int i = 0; i < Size; i++) {
            auto particle = &particles[i];
            // update particle
            particle->lifetime -= deltaTime;
            if(particle->lifetime < 0.f) {
                // create a new particle when this die at this location
                Spawn(particle);
            } else {
                particle->position += particle->velocity * deltaTime;
                particle->velocity += Gravity * deltaTime;
            }
        }
    }

    // read-only size property
    int size() {
        return Size;
    }

    void Delete() {
        delete[] particles;
    }

private:
    int Size;

    void Spawn(Particle *particle) {
        float k = randf();
        particle->color = Color0 * k + (1 - k) * Color1;
        k = randf();
        particle->scale = Scale0 * k + (1 - k) * Scale1;
        k = randf();
        float vx = k * DeltaVelocity0.x + (1 - k) * DeltaVelocity1.x;
        k = randf();
        float vy = k * DeltaVelocity0.y + (1 - k) * DeltaVelocity1.y;
        k = randf();
        float vz = k * DeltaVelocity0.z + (1 - k) * DeltaVelocity1.z;
        particle->velocity = Velocity + glm::vec3(vx, vy, vz);
        particle->acceleration = glm::vec3(0.f, 0.f, 0.f);
        particle->size = uniform_between(Size0, Size1);
        // TODO: check degrees or radians
        particle->rotation = uniform_between(Rotation0, Rotation1);
        particle->alpha = uniform_between(Alpha0, Alpha1); 

        // reset position to origin
        particle->position = Position;

        // set lifetime to total lifespan
        particle->lifespan = uniform_between(Lifespan0, Lifespan1); 
        particle->lifetime = particle->lifespan;
        
    }
};
