/*
Camera class
- creation of a reference system for the camera
- management of camera movement (FPS-style) using WASD keys and mouse movement

N.B.) adaptation of https://github.com/JoeyDeVries/LearnOpenGL/blob/master/includes/learnopengl/camera.h

author: Davide Gadia

Real-time Graphics Programming - a.a. 2022/2023
Master degree in Computer Science
Universita' degli Studi di Milano
*/

#pragma once

// we use GLM to create the view matrix and to manage camera transformations
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

// possible camera movements
enum Camera_Movement {
    FORWARD,
    BACKWARD,
    LEFT,
    RIGHT
};

// Default camera settings
// Initial camera orientation on Y and X (Z not considered)
const GLfloat YAW        = -90.0f; //Y
const GLfloat PITCH      =  0.0f; //X

// speed of camera movement
const GLfloat SPEED      =  3.0f;
// speed compensation in case of diagonal camera movement (= 1/sqrt(2))
const GLfloat DIAGONAL_COMPENSATION = 0.70710678f;
// parameter to weight mouse movement
const GLfloat SENSITIVITY =  0.25f;

///////////////////  CAMERA class ///////////////////////
class Camera
{
public:
    // Camera Attributes
    glm::vec3 Position;
    glm::vec3 Front;
    glm::vec3 WorldFront;
    glm::vec3 Up; // camera local UP vector
    glm::vec3 Right;
    glm::vec3 WorldUp; //  camera world UP vector -> needed for the initial computation of Right vector
    GLboolean onGround; // it defines if the camera is "anchored" to the ground, or if it strictly follows the current Front direction (even if this means that the camera "flies" in the scene)
    // N.B.) this version works only for flat terrains
    // Eular Angles
    GLfloat Yaw;
    GLfloat Pitch;
    // Camera speed parameters
    GLfloat MovementSpeed;
    GLfloat MovementCompensation = 1.0f;
    // Camera rotation parameter
    GLfloat MouseSensitivity;

    //////////////////////////////////////////
    // simplified constructor
    // it can be extended passing different values of speed and mouse sensitivity, etc...
    Camera(glm::vec3 position, GLboolean onGround)
        :Position(position),onGround(onGround),Yaw(YAW),Pitch(PITCH),MovementSpeed(SPEED),MouseSensitivity(SENSITIVITY)
    {
        this->WorldUp = glm::vec3(0.0f,1.0f,0.0f);
        // initialization of the camera reference system
        this->UpdateCameraVectors();
    }

    //////////////////////////////////////////
    // it returns the current view matrix
    glm::mat4 GetViewMatrix()
    {
        return glm::lookAt(this->Position, this->Position + this->Front, this->Up);
    }

    //////////////////////////////////////////
    // if a single WASD key is pressed, then we will apply the full value of velocity v in the corresponding direction.
    // However, if two keys are pressed together in order to move diagonally (W+D, W+A, S+D, S+A), 
    // then we will apply a compensated velocity in the single directions, in order to have the full v applied in the diagonal direction
    // The compensation is = 1/sqrt(2)
    void SetMovementCompensation(GLboolean diagonal_movement)
    {
        MovementCompensation = (diagonal_movement ? DIAGONAL_COMPENSATION : 1.0f);
    }

    //////////////////////////////////////////
    // it updates camera position when a WASD key is pressed
    void ProcessKeyboard(Camera_Movement direction, GLfloat deltaTime)
    {        
        // the velocity is weighted by a compensation factor
        // = 1 if a single WASD is pressed
        // = 1/sqrt(2) if two keys are pressed for a diagonal movement
        GLfloat velocity = this->MovementSpeed * deltaTime * MovementCompensation;

        if (direction == FORWARD)
            this->Position += (this->onGround ? this->WorldFront : this->Front) * velocity;
        if (direction == BACKWARD)
            this->Position -= (this->onGround ? this->WorldFront : this->Front) * velocity;
        if (direction == LEFT)
            this->Position -= this->Right * velocity;
        if (direction == RIGHT)
            this->Position += this->Right * velocity;
        
    }

    //////////////////////////////////////////
    // it updates camera orientation when mouse is moved
    void ProcessMouseMovement(GLfloat xoffset, GLfloat yoffset, GLboolean constraintPitch = GL_TRUE)
    {
        // the sensitivity is applied to weight the movement
        xoffset *= this->MouseSensitivity;
        yoffset *= this->MouseSensitivity;

        // rotation angles on Y and X are updated
        this->Yaw   += xoffset;
        this->Pitch += yoffset;

        // we apply a constraint to the rotation on X, to avoid to have the camera flipped upside down
        // N.B.) this constraint helps to avoid gimbal lock, if all the 3 rotations are considered
        if (constraintPitch)
        {
            if (this->Pitch > 89.0f)
                this->Pitch = 89.0f;
            if (this->Pitch < -89.0f)
                this->Pitch = -89.0f;
        }

        // the camera reference system is updated using the new camera rotations
        this->UpdateCameraVectors();
    }

    //////////////////////////////////////////
    // it updates the camera reference system
    void UpdateCameraVectors()
    {
        // it computes the new Front vector using trigonometric calculations using Yaw and Pitch angles
        // https://learnopengl.com/#!Getting-started/Camera
        glm::vec3 front;
        front.x = cos(glm::radians(this->Yaw)) * cos(glm::radians(this->Pitch));
        front.y = sin(glm::radians(this->Pitch));
        front.z = sin(glm::radians(this->Yaw)) * cos(glm::radians(this->Pitch));
        this->Front = glm::normalize(front);
        // if the camera is "anchored" to the ground, the world Front vector is equal to the local Front vector, but with the y component = 0
        front.y = 0.0f;
        this->WorldFront = glm::normalize(front);
        // Once calculated the new view direction, we re-calculate the Right vector as cross product between Front and world Up vector
        this->Right = glm::normalize(glm::cross(this->Front, this->WorldUp));
        // we calculate the camera local Up vector as cross product between Front and Right vectors
        this->Up    = glm::normalize(glm::cross(this->Right, this->Front));     
    }

    void UpdateCameraVectors(glm::vec3 front)
    {
        this->Front = glm::normalize(front);
        this->WorldFront = glm::normalize(front);
        // Once calculated the new view direction, we re-calculate the Right vector as cross product between Front and world Up vector
        this->Right = glm::normalize(glm::cross(this->Front, this->WorldUp));
        // we calculate the camera local Up vector as cross product between Front and Right vectors
        this->Up    = glm::normalize(glm::cross(this->Right, this->Front));     
    }
};
