/*
Es06b: physics simulation using Bullet library.
Using Physics class (in include/utils), we set the gravity of the world, mass and physical characteristics of the objects in the scene.
Pressing the space key, we "shoot" a sphere inside the scene, which it will collide with the other objects.

N.B. 1) to test different parameters of the shaders, it is convenient to use some GUI library, like e.g. Dear ImGui (https://github.com/ocornut/imgui)

author: Davide Gadia

Real-Time Graphics Programming - a.a. 2022/2023
Master degree in Computer Science
Universita' degli Studi di Milano
*/

/*
OpenGL coordinate system (right-handed)
positive X axis points right
positive Y axis points up
positive Z axis points "outside" the screen


                              Y
                              |
                              |
                              |________X
                             /
                            /
                           /
                          Z
*/

// Std. Includes
#include <string>

// Loader estensioni OpenGL
// http://glad.dav1d.de/
// THIS IS OPTIONAL AND NOT REQUIRED, ONLY USE THIS IF YOU DON'T WANT GLAD TO INCLUDE windows.h
// GLAD will include windows.h for APIENTRY if it was not previously defined.
// Make sure you have the correct definition for APIENTRY for platforms which define _WIN32 but don't use __stdcall
#ifdef _WIN32
    #define APIENTRY __stdcall
#endif

#include <cmath>

#include <glad/glad.h>

// GLFW library to create window and to manage I/O
#include <glfw/glfw3.h>

// another check related to OpenGL loader
// confirm that GLAD didn't include windows.h
#ifdef _WINDOWS_
    #error windows.h was included!
#endif

// classes developed during lab lectures to manage shaders, to load models, for FPS camera, and for physical simulation
#include <utils/random.h>

#include <utils/shader.h>

#include <utils/model.h>
#include <utils/camera.h>
#include <utils/physics.h>
#include <utils/vehicle.h>
#include <utils/obstacle.h>

#include <utils/particle.h>

#include <utils/depth_texture.h>
#include <utils/skybox_texture.h>
#include <utils/image_texture.h>

#include <utils/mesh_renderer.h>
#include <utils/particle_renderer.h>
#include <utils/skybox_renderer.h>
#include <utils/heightmap_renderer.h>
#include <utils/heightmap_depth_renderer.h>
#include <utils/shadow_renderer.h>

#include <imgui/imgui.h>
#include <imgui/imgui_impl_glfw.h>
#include <imgui/imgui_impl_opengl3.h>

// we load the GLM classes used in the application
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/matrix_inverse.hpp>
#include <glm/gtc/type_ptr.hpp>

#define HEIGHTMAP_SIZE 1024

// Dimensioni della finestra dell'applicazione
GLuint screenWidth = 1400, screenHeight = 1100;

// shadow map framebuffer dimensions
const GLuint SHADOW_WIDTH = 2048, SHADOW_HEIGHT = 2048;

// callback functions for keyboard and mouse events
void key_callback(GLFWwindow* window, int key, int scancode, int action, int mode);
void mouse_callback(GLFWwindow* window, double xpos, double ypos);

// we initialize an array of booleans for each keyboard key
bool keys[1024];

// we need to store the previous mouse position to calculate the offset with the current frame
GLfloat lastX, lastY;

// we will use these value to "pass" the cursor position to the keyboard callback, in order to determine the bullet trajectory
double cursorX, cursorY;

// when rendering the first frame, we do not have a "previous state" for the mouse, so we need to manage this situation
bool firstMouse = true;

// parameters for time calculation (for animations)
GLfloat deltaTime = 0.0f;
GLfloat lastFrame = 0.0f;

// boolean to activate/deactivate wireframe rendering
GLboolean wireframe = GL_FALSE;

// view and projection matrices (global because we need to use them in the keyboard callback)
glm::mat4 view, projection;

// we create a camera. We pass the initial position as a parameter to the constructor. In this case, we use a "floating" camera (we pass false as last parameter)
Camera camera(glm::vec3(0.0f, 0.0f, 9.0f), GL_FALSE);
// constant distance from the camera and the vehicle
glm::vec3 cameraOffset(0.f, 10.f, -25.f);


// color of the falling objects
glm::vec3 diffuseColor{1.0f,0.0f,0.0f};
glm::vec3 carColor{.8f, .8f, .8f};
// color of the plane
glm::vec3 planeMaterial1{.6f, .6f, .7f};
glm::vec3 planeMaterial2{.3f, .3f, .3f};
// color of the bullets
glm::vec3 shootColor{1.0f,1.0f,0.0f};
// dimension of the bullets (global because we need it also in the keyboard callback)
glm::vec3 sphereSize = glm::vec3(0.2f, 0.2f, 0.2f);

unsigned int quadVAO, quadVBO;

// models 
Model *cubeModel;
Model *sphereModel;
Model *cylinderModel;
Model *carModel;
Model *bridgeModel;
Model *rampModel;
Model *skateParkModel;
Model *bowlingPinModel;


void drawQuad() {
    glBindVertexArray(quadVAO);
    glDrawArrays(GL_TRIANGLES, 0, 6);
}

glm::vec3 toGLM(const btVector3 &v) {
    return glm::vec3(v.getX(), v.getY(), v.getZ());
}

void calculateShadowMapViewFrustum(glm::mat4 view, glm::mat4 projection, glm::vec3 lightDirection, 
                                   glm::mat4 *lightView, glm::mat4 *lightProjection) {
    // HACK:
    auto center = camera.Position + camera.Front * glm::length(cameraOffset);
    *lightView = glm::lookAt(center + lightDirection,
                            center,
                            glm::vec3(0.0f, 1.0f, 0.0f));
    float near_plane = -100.0f, far_plane = 100.f;
    *lightProjection = glm::ortho(-70.0f, 70.0f, -70.0f, 70.0f, near_plane, far_plane);
}

void updateCameraPosition(Vehicle vehicle, Camera &camera, const glm::vec3 offset, float deltaTime) {
    // camera will always follow the car staying behind of it
    auto bulletVehicle = vehicle.GetBulletVehicle();
    btTransform chassisTransform = bulletVehicle.getChassisWorldTransform();
    btVector3 chassisPosition = chassisTransform * btVector3(0.f, 0.f, 0.f);
    btVector3 targetPosition = chassisTransform * btVector3(offset.x, offset.y, offset.z);
    if(targetPosition.getY() < 0) {
        // check if the camera is behind the plane
        // in that case invert the y axis
        targetPosition *= btVector3(1.f, -1.f, 1.f);
    }
    glm::vec3 diffVector = toGLM(targetPosition) - camera.Position;
    // the camera can move up to this force per second, a too fast camera is shaky
    // specialy when the vehicle is rolling or go upside down. 
    // in this way we are interpolating the position
    float maxMagnitude = 90.f * deltaTime;
    // clamp magnitude of the camera vector velocity
    float len = glm::length(diffVector);
    if(len > 0) {
        auto magnitude = len > maxMagnitude ? maxMagnitude : len;
        diffVector = diffVector * (magnitude / len);
    }
    glm::vec3 newCameraPosition = camera.Position + diffVector;
    camera.UpdateCameraVectors(toGLM(chassisPosition) - newCameraPosition);
    camera.Position = newCameraPosition;
}

void updateEmitterPosition(Vehicle vehicle, ParticleEmitter *emitter, float deltaTime) {
    // camera will always follow the car staying behind of it
    auto bulletVehicle = vehicle.GetBulletVehicle();
    btTransform chassisTransform = bulletVehicle.getChassisWorldTransform();
    auto chassisBox = vehicle.getChassisSize();
    btVector3 targetPosition = chassisTransform * btVector3(0, 0, -chassisBox.z);

    emitter->Position = toGLM(targetPosition);
}

void drawRigidBody(ObjectRenderer &renderer, btRigidBody *body) {
    Model *objectModel;

    // object transformation first getted as btTransform, then stored in "OpenGL"
    // matrix and then converted to glm datatype
    btTransform transform;
    float matrix[16];
    glm::mat4 modelMatrix = glm::mat4(1.0f);
    glm::mat4 normalMatrix = glm::mat3(1.0f);

    GLint program = 0;
    glGetIntegerv(GL_CURRENT_PROGRAM, &program);

    glm::vec3 obj_size;

    if (body->getCollisionShape()->getShapeType() == BOX) {
        // we point objectModel to the cube
        objectModel = cubeModel;
        // TODO: retrive size of the cube
        auto box = (btBoxShape*) body->getCollisionShape();
        auto extends = box->getHalfExtentsWithMargin();
        obj_size = toGLM(extends);
        // we pass red color to the shader
        renderer.SetColor(diffuseColor);
    }
    // if is not a box draw when only handle spheres for now
    else {
        // we point objectModel to the sphere
        objectModel = sphereModel;
        // TODO: retrive size of the sphere
        obj_size = sphereSize;
        // we pass yellow color to the shader
        renderer.SetColor(shootColor);
    }

    // we take the transformation matrix of the rigid boby, as calculated by the physics engine
    body->getMotionState()->getWorldTransform(transform);

    // we convert the Bullet matrix (transform) to an array of floats
    transform.getOpenGLMatrix(matrix);

    // we reset to identity at each frame
    modelMatrix = glm::mat4(1.0f);
    normalMatrix = glm::mat3(1.0f);

    // we create the GLM transformation matrix
    // 1) we convert the array of floats to a GLM mat4 (using make_mat4 method)
    // 2) Bullet matrix provides rotations and translations: it does not consider scale (usually the Collision Shape is generated using directly the scaled dimensions). If, like in our case, we have applied a scale to the original model, we need to multiply the scale to the rototranslation matrix created in 1). If we are working on an imported and not scaled model, we do not need to do this
    modelMatrix = glm::make_mat4(matrix) * glm::scale(modelMatrix, obj_size);
    // we create the normal matrix
    renderer.SetModelTrasformation(modelMatrix);

    // we render the model
    // N.B.) if the number of models is relatively low, this approach (we render the same mesh several time from the same buffers) can work. If we must render hundreds or more of copies of the same mesh,
    // there are more advanced techniques to manage Instanced Rendering (see https://learnopengl.com/#!Advanced-OpenGL/Instancing for examples).
    objectModel->Draw();
}

void drawPlane(ObjectRenderer &renderer, glm::vec3 planePos, glm::vec3 planeSize) {
    // The plane is static, so its Collision Shape is not subject to forces, and it does not move. Thus, we do not need to use dynamicsWorld to acquire the rototraslations, but we can just use directly glm to manage the matrices
    // if, for some reason, the plane becomes a dynamic rigid body, the following code must be modified
    // we reset to identity at each frame
    glm::mat4 planeModelMatrix(1.0f);
    planeModelMatrix = glm::translate(planeModelMatrix, planePos);
    planeModelMatrix = glm::scale(planeModelMatrix, planeSize);
    renderer.SetModelTrasformation(planeModelMatrix);
    
    // we render the plane
    cubeModel->Draw();
}

void drawVehicle(ObjectRenderer &renderer, Vehicle &vehicle) {
    auto bulletVehicle = vehicle.GetBulletVehicle();
    // drawing the chassis
    renderer.SetColor(carColor);
    // save a temp matrix for conversion from bullet to opengl
    float matrix[16];

    // get car dimension
    auto chassisBox = vehicle.getChassisSize();

    // we take the transformation matrix of the rigid boby, as calculated by the physics engine
    auto transform = bulletVehicle.getChassisWorldTransform();

    // we convert the Bullet matrix (transform) to an array of floats
    transform.getOpenGLMatrix(matrix);

    // we reset to identity at each frame
    glm::mat4 objModelMatrix(1.0f);

    // we create the GLM transformation matrix
    // 1) we convert the array of floats to a GLM mat4 (using make_mat4 method)
    // 2) Bullet matrix provides rotations and translations: it does not consider scale (usually the Collision Shape is generated using directly the scaled dimensions). If, like in our case, we have applied a scale to the original model, we need to multiply the scale to the rototranslation matrix created in 1). If we are working on an imported and not scaled model, we do not need to do this
    objModelMatrix = glm::make_mat4(matrix) * glm::scale(objModelMatrix, chassisBox);
    // we create the normal matrix
    renderer.SetModelTrasformation(objModelMatrix);
    carModel->Draw();

    // draw wheels
    for(int wheel = 0; wheel < bulletVehicle.getNumWheels(); wheel++) {
        auto wheelWidth = vehicle.WheelInfo.width;
        auto wheelRadius = vehicle.WheelInfo.radius;
        // scaling the wheel a bit, to better fit the cylinder model
        auto wheelSize = glm::vec3(wheelWidth, wheelRadius, wheelRadius) * .5f;
        renderer.SetColor(carColor);

        transform = bulletVehicle.getWheelTransformWS(wheel);

        // we convert the Bullet matrix (transform) to an array of floats
        transform.getOpenGLMatrix(matrix);

        // wheel transformation: cylinder is oriented toward the y axis, while we need a cylinder z 
        // we reset to identity at each frame
        objModelMatrix = glm::mat4(1.0f);
        objModelMatrix = glm::rotate(glm::scale(objModelMatrix, wheelSize), glm::radians(90.f), glm::vec3(0.f, 0.f, 1.f));
        objModelMatrix = glm::make_mat4(matrix) * objModelMatrix;
        // we pass the model trasformation to the renderer that creates the normal matrix for us
        renderer.SetModelTrasformation(objModelMatrix);

        cylinderModel->Draw();
    }

}

////////////////// MAIN function ///////////////////////
int main()
{
    // Initialization of OpenGL context using GLFW
    glfwInit();
    // We set OpenGL specifications required for this application
    // In this case: 4.1 Core
    // If not supported by your graphics HW, the context will not be created and the application will close
    // N.B.) creating GLAD code to load extensions, try to take into account the specifications and any extensions you want to use,
    // in relation also to the values indicated in these GLFW commands
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 1);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
    // we set if the window is resizable
    glfwWindowHint(GLFW_RESIZABLE, GL_FALSE);

    // we create the application's window
    GLFWwindow* window = glfwCreateWindow(screenWidth, screenHeight, "RGP_lecture06b", nullptr, nullptr);
    if (!window)
    {
        std::cout << "Failed to create GLFW window" << std::endl;
        glfwTerminate();
        return -1;
    }
    glfwMakeContextCurrent(window);

    // we put in relation the window and the callbacks
    glfwSetKeyCallback(window, key_callback);
    glfwSetCursorPosCallback(window, mouse_callback);

    // we disable the mouse cursor
    //glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);

    // GLAD tries to load the context set by GLFW
    if (!gladLoadGLLoader((GLADloadproc) glfwGetProcAddress))
    {
        std::cout << "Failed to initialize OpenGL context" << std::endl;
        return -1;
    }

    // we define the viewport dimensions
    int width, height;
    glfwGetFramebufferSize(window, &width, &height);
    glViewport(0, 0, width, height);

    // create the framebuffer where to render the whole screen
    GLuint fbo;
    glGenFramebuffers(1, &fbo);
    glBindFramebuffer(GL_FRAMEBUFFER, fbo);

    // create an rgb texture attached to the framebuffer
    GLuint texture;
    glGenTextures(1, &texture);
    glBindTexture(GL_TEXTURE_2D, texture);    
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, NULL);
    // no mgnification/minification our texture framebuffer is of the same exact dimension of the screen
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);  
    // attach texture to the framebuffer
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, texture, 0);

    // render buffer for stencil and depth testing
    GLuint rbo;
    glGenRenderbuffers(1, &rbo);
    glBindRenderbuffer(GL_RENDERBUFFER, rbo);
    // create depth and stencil buffer
    glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH24_STENCIL8, width, height);
    // attach the render buffer object
    glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT, GL_RENDERBUFFER, rbo);  

    // we enable Z test
    glEnable(GL_DEPTH_TEST);

    //the "clear" color for the frame buffer
    glClearColor(0.26f, 0.46f, 0.98f, 1.0f);

    // instance of the physics class
    Physics &bulletSimulation = Physics::GetInstance();

    MeshRenderer renderer;
    renderer.lightDirection = glm::vec3(1.0f, 1.0f, 1.0f);
    /// Parameters for the illumination model
    IlluminationModelParameters illumination;
    illumination.Kd = 3.0f;
    illumination.alpha = 0.6f;
    illumination.F0 = 0.9f;

    // renderer for skybox
    SkyboxRenderer skyboxRenderer;

    // renderer for the shadow map
    ShadowRenderer shadowRenderer;
    
    // renderer for the heightmap depth buffer
    HeightmapDepthRenderer heightmapDepthRenderer;

    // the Shader Program used on the framebuffer, for effect on the whole screen
    Shader postprocessing_shader = Shader("postprocessing.vert", "postprocessing.frag");

    // we load the model(s) (code of Model class is in include/utils/model.h)
    cubeModel = new Model("../models/cube.obj");
    sphereModel = new Model("../models/sphere.obj");
    carModel = new Model("../models/mclaren.obj");
    cylinderModel = new Model("../models/cylinder.obj");
    bridgeModel = new Model("../models/stone_bridge.obj");
    rampModel = new Model("../models/ramp.obj");
    skateParkModel = new Model("../models/skatepark_ramp.obj");
    bowlingPinModel = new Model("../models/bowling_pin.obj");

    // screen quad VAO
    glGenVertexArrays(1, &quadVAO);
    glGenBuffers(1, &quadVBO);
    glBindVertexArray(quadVAO);
    glBindBuffer(GL_ARRAY_BUFFER, quadVBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(quadVertices), &quadVertices, GL_STATIC_DRAW);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)(2 * sizeof(float)));

    // dimensions and position of the static plane
    // we will use the cube mesh to simulate the plane, because we need some "height" in the mesh
    // in order to make it work with the physics simulation
    glm::vec3 plane_pos = glm::vec3(0.0f, -1.0f, 0.0f);
    glm::vec3 plane_size = glm::vec3(400.0f, 0.1f, 400.0f);
    glm::vec3 plane_rot = glm::vec3(0.0f, 0.0f, 0.0f);

    // we create a rigid body for the plane. In this case, it is static, so we pass mass = 0;
    // in this way, the plane will not fall following the gravity force.
    btRigidBody *plane = bulletSimulation.createRigidBody(BOX, plane_pos, plane_size, plane_rot, 0.0f, 0.3f, 0.3f);
    
    glm::vec3 bridgePos(0.f, -1.f, 10.f);
    Obstacle bridge(bridgeModel, bridgePos);

    glm::vec3 spPos(60.f, -2.f, -70.f);
    Obstacle skatePark(skateParkModel, spPos);

    glm::vec3 rampPos(60.f, -1.f, -50.f);
    glm::vec3 rampDim(10.f, 10.f, 10.f);
    Obstacle ramp(rampModel, rampPos, rampDim);
    ramp.Illumination.Kd = 3.f;
    ramp.Illumination.alpha = .3f;
    ramp.Illumination.F0 = .9f;

    /// TODO: nice big ball
    // auto ball = bulletSimulation.createRigidBody(SPHERE, glm::vec3(-100.f, 1.f, -70.f), glm::vec3(3.f, 3.f, 3.f), glm::vec3(0, 0, 0), 5.f, .3f, 2.f);
    /// TODO: method DrawSphere
    // create ball
    auto ball = bulletSimulation.createRigidBody(SPHERE, glm::vec3(-100.f, 1.f, -70.f), glm::vec3(1.f, 1.f, 1.f), glm::vec3(0, 0, 0), 15.f, .3f, 3.f);
    // create pins
    std::vector<btRigidBody*> pins;
    glm::vec3 pinDim(10.f, 10.f, 10.f);
    {
        // create all pins
        std::vector<glm::vec3> pinOffsets {
            glm::vec3(4.f, 0.f, 0.f),
            glm::vec3(3.f, 0.f, 1.f),
            glm::vec3(5.f, 0.f, 1.f),
            glm::vec3(2.f, 0.f, 2.f),
            glm::vec3(4.f, 0.f, 2.f),
            glm::vec3(6.f, 0.f, 2.f),
            glm::vec3(1.f, 0.f, 3.f),
            glm::vec3(3.f, 0.f, 3.f),
            glm::vec3(5.f, 0.f, 3.f),
            glm::vec3(7.f, 0.f, 3.f),
        };

        glm::vec3 trianglePinPosition(-100.f, 1.f, -50.f);
        for(int i = 0; i < 10; i += 1) {
            auto pinPos = trianglePinPosition + pinOffsets[i] * glm::vec3(1.5f, 1.f, 2.f);
            glm::mat4 pinModelMatrix(1.0f);
            pinModelMatrix = glm::translate(pinModelMatrix, pinPos);
            pinModelMatrix = glm::scale(pinModelMatrix, pinDim);
            // creating a rigid body for each mesh of the model
            auto pin = bulletSimulation.createConvexDynamicRigidBodyFromModel(bowlingPinModel, pinPos, plane_rot, pinDim, .2f, .3f, .3f);
            pins.push_back(pin);
        }
    }

    /// create particles emitter for snow
    auto totalParticles = 1500;
    auto snowEmitter = new ParticleEmitter(totalParticles);
    snowEmitter->Position = glm::vec3(0.f, 15.f, 0.f);
    {
        // set the random variables
        snowEmitter->Size0     = 0.02f;
        snowEmitter->Size1     = 0.1f;
        snowEmitter->Rotation0 = 0.1f;
        snowEmitter->Rotation1 = 5.f;
        snowEmitter->Lifespan0 = 4.f;
        snowEmitter->Lifespan1 = 10.f;
        snowEmitter->Velocity  = glm::vec3(0.f, .1f, 0.f);
        snowEmitter->DeltaVelocity0 = glm::vec3( .8f, -.1f,  .8f);
        snowEmitter->DeltaVelocity1 = glm::vec3(-.8f, 0.f, -.8f);
        snowEmitter->Color0    = glm::vec3(.8f, .8f, .8f);
        snowEmitter->Color1    = glm::vec3(.5f, .5f, .5f);
        snowEmitter->Scale0    = glm::vec3(1.f, 1.f, 1.f);
        snowEmitter->Scale1    = glm::vec3(1.f, 1.f, 1.f);
        snowEmitter->Alpha0    = .6f;
        snowEmitter->Alpha1    = 1.f;
        snowEmitter->Gravity   = glm::vec3(0.f, -.5f, 0.f);
    }
    snowEmitter->spawnShape = RECTANGLE;
    snowEmitter->spawnRectSize = {50, 50};

    ParticleRenderer snowParticleRenderer(*snowEmitter);
    snowParticleRenderer.SetParticleShape(SQUARE);


    /// create particles emitter for turbo machine
    totalParticles = 500;
    auto emitter = new ParticleEmitter(totalParticles);
    {
        // set the random variables
        emitter->Size0     = 0.02f;
        emitter->Size1     = 0.1f;
        emitter->Rotation0 = 0.1f;
        emitter->Rotation1 = 5.f;
        emitter->Lifespan0 = .05f;
        emitter->Lifespan1 = .3f;
        emitter->Velocity  = glm::vec3(0.f, 1.f, 0.f);
        emitter->DeltaVelocity0 = glm::vec3( .8f, 1.f,  .8f);
        emitter->DeltaVelocity1 = glm::vec3(-.8f, 0.f, -.8f);
        emitter->Color0    = glm::vec3(.8f, .1f, .1f);
        emitter->Color1    = glm::vec3(.5f, .5f, .0f);
        emitter->Scale0    = glm::vec3(1.f, 1.f, 1.f);
        emitter->Scale1    = glm::vec3(1.f, 1.f, 1.f);
        emitter->Alpha0    = .6f;
        emitter->Alpha1    = 1.f;
        emitter->Gravity   = glm::vec3(0.f, -1.2f, 0.f);
    }

    emitter->Active = false;

    ParticleRenderer particleRenderer(*emitter);

    // set the type of the particle 
    particleRenderer.SetParticleShape(CIRCLE);

    /// create vehicle
    glm::vec3 chassisBox(1.f, .5f, 2.f);
    // default wheel infos
    WheelInfo wheelInfo;
    Vehicle vehicle(chassisBox, wheelInfo);
    IlluminationModelParameters carIlluminationParameter;
    carIlluminationParameter.Kd = 4.2f;
    carIlluminationParameter.alpha = 0.2f;
    carIlluminationParameter.F0 = 0.9f;

    // we create 25 rigid bodies for the cubes of the scene. In this case, we use BoxShape, with the same dimensions of the cubes, as collision shape of Bullet. For more complex cases, a Bounding Box of the model may have to be calculated, and its dimensions to be passed to the physics library
    GLint num_side = 5;
    // total number of the cubes
    GLint total_cubes = num_side*num_side;
    GLint i,j;
    // position of the cube
    glm::vec3 cube_pos;
    // dimension of the cube
    glm::vec3 cube_size = glm::vec3(.4f, 1.f, .4f);
    // we set a small initial rotation for the cubes
    glm::vec3 cube_rot = glm::vec3(0.1f, 0.0f, 0.1f);
    // rigid body
    btRigidBody *cube;

    int cubes_start_i = bulletSimulation.dynamicsWorld->getCollisionObjectArray().size();
    // we create a 5x5 grid of rigid bodies
    for(i = 0; i < num_side; i++ )
    {
        for(j = 0; j < num_side; j++ )
        {
            // position of each cube in the grid (we add 3 to x to have a bigger displacement)
            cube_pos = glm::vec3(3.0f + 5.f * i, 1.0f, j * 5.f);
            // we create a rigid body (in this case, a dynamic body, with mass = 2)
            cube = bulletSimulation.createRigidBody(BOX, cube_pos, cube_size, cube_rot, 2.0f, 0.3f, 0.3f);
        }
    }

    // creating ramp
    bulletSimulation.createRigidBody(BOX, glm::vec3(-10.f, -2.f, 0.f), glm::vec3(3.f), glm::vec3(0.f, 0.f, glm::radians(60.f)), 0, 0.3f, 0.3f);

    // we set the maximum delta time for the update of the physical simulation
    GLfloat maxSecPerFrame = 1.0f / 120.0f;

    // Projection matrix: FOV angle, aspect ratio, near and far planes
    projection = glm::perspective(45.0f, (float)screenWidth/(float)screenHeight, 0.1f, 10000.0f);

    // Model and Normal transformation matrices for the objects in the scene: we set to identity
    glm::mat4 objModelMatrix = glm::mat4(1.0f);
    // textures for plane
    ImageTexture brickTexture("../textures/bricks.jpg");
    ImageTexture brickNormalMap("../textures/bricks_NormalMap.jpg");
    ImageTexture planeTexture("../textures/DirtFloor.jpg");
    ImageTexture planeNormalMap("../textures/DirtFloor_NormalMap.png");
    ImageTexture planeDisplacementMap("../textures/DirtFloor_DispMap.png");
    ImageTexture snowTexture("../textures/snow.jpg");
    ImageTexture asphaltTexture("../textures/Asphalt.jpg");
    ImageTexture asphaltNormalMap("../textures/Asphalt_NormalMap.jpg");
    // ImageTexture planeTexture("../textures/Stone.jpg");
    // ImageTexture planeNormalMap("../textures/Stone_NormalMap.jpg");
    // ImageTexture planeDisplacementMap("../textures/Stone_DispMap.jpg");
    SkyboxTexture skybox("../textures/skyboxs/default/");
    
    // create shadow map frame buffer object
    GLuint depthMapFBO;
    glGenFramebuffers(1, &depthMapFBO);
    // create texture for shadow map
    DepthTexture shadowMap(SHADOW_WIDTH, SHADOW_HEIGHT);
    // attach the texture to the framebuffer object
    shadowMap.AttachToFrameBuffer(depthMapFBO);
    
    // create heightmap frame buffer object
    GLuint heightmapFBO;
    glGenFramebuffers(1, &heightmapFBO);
    float heightmap_width = 100, heightmap_height = 100;
    DepthTexture previousFrameHeightmap(HEIGHTMAP_SIZE, HEIGHTMAP_SIZE);
    previousFrameHeightmap.Clear();
    DepthTexture heightmapTexture(HEIGHTMAP_SIZE, HEIGHTMAP_SIZE);
    // attach the texture to the framebuffer object
    heightmapTexture.AttachToFrameBuffer(heightmapFBO);

    // heightmap object
    Heightmap heightmap(plane_pos.y, heightmap_width, heightmap_height, 1.f);
    // renderer for the heightmap
    HeightmapRenderer heightmapRenderer(heightmap);

    auto renderPlane = [&](ObjectRenderer &objectRenderer) {
        objectRenderer.UpdateIlluminationModel(illumination);
        // set texture for the plane
        objectRenderer.SetTexture(planeTexture, 20.f);
        // set the normal map
        objectRenderer.SetNormalMap(planeNormalMap);
        // set the displacement texture that is used as parallax map
        objectRenderer.SetParallaxMap(planeDisplacementMap, -.005f);

        drawPlane(objectRenderer, plane_pos, plane_size);
    };

    auto renderObjects = [&](ObjectRenderer &objectRenderer) {
        // car is supposed to be metal, so change the illumination model a bit
        objectRenderer.UpdateIlluminationModel(carIlluminationParameter);
        // do not use parallax map anymore but restore uv coordinate interpolation
        objectRenderer.SetTexCoordinateCalculation(UV);
        // reset normals calculation in vertex shader with normal matrix not anymore with normal map
        objectRenderer.SetNormalCalculation(FROM_MATRIX);
        drawVehicle(objectRenderer, vehicle);

        // illumination parameter for plastic objects
        objectRenderer.UpdateIlluminationModel(illumination);
        // we need two variables to manage the rendering of both cubes and bullets
        glm::vec3 obj_size;
        // we ask Bullet to provide the total number of Rigid Bodies in the scene
        int num_cobjs = bulletSimulation.dynamicsWorld->getNumCollisionObjects();

        // we cycle among all the Rigid Bodies (starting from 1 to avoid the plane)
        for (i=cubes_start_i; i< num_cobjs; i++)
        {
            // we take the Collision Object from the list
            btCollisionObject *obj = bulletSimulation.dynamicsWorld->getCollisionObjectArray()[i];
            // we upcast it in order to use the methods of the main class RigidBody
            btRigidBody *body = btRigidBody::upcast(obj);

            drawRigidBody(objectRenderer, body);
        }

        // draw the bridge
        bridge.Draw(objectRenderer);
        
        // drawing the curved ramp
        objectRenderer.SetTexture(asphaltTexture, 2.f);
        objectRenderer.SetNormalMap(asphaltNormalMap);
        ramp.Draw(objectRenderer);

        // reset colors for remaining obstacles
        objectRenderer.SetColor(glm::vec3(1.f, 0.f, 0.f));
        // drawing the skate park
        skatePark.Draw(objectRenderer);
        
        // draw bowling ball
        objectRenderer.SetColor(glm::vec3(0.f, 1.f, 1.f));
        float matrix[16];
        btTransform transform;
        // we take the transformation matrix of the rigid boby, as calculated by the physics engine
        ball->getMotionState()->getWorldTransform(transform);
        // we convert the Bullet matrix (transform) to an array of floats
        transform.getOpenGLMatrix(matrix);
        // we reset to identity at each frame
        auto modelMatrix = glm::mat4(1.0f);
        auto normalMatrix = glm::mat3(1.0f);
        // we create the GLM transformation matrix
        // 1) we convert the array of floats to a GLM mat4 (using make_mat4 method)
        // 2) Bullet matrix provides rotations and translations: it does not consider scale (usually the Collision Shape is generated using directly the scaled dimensions). If, like in our case, we have applied a scale to the original model, we need to multiply the scale to the rototranslation matrix created in 1). If we are working on an imported and not scaled model, we do not need to do this
        modelMatrix = glm::make_mat4(matrix) * glm::scale(modelMatrix, glm::vec3(1.f, 1.f, 1.f));
        // we create the normal matrix
        objectRenderer.SetModelTrasformation(modelMatrix);
        sphereModel->Draw();
        
        // drawing the bowling pins
        objectRenderer.SetColor(glm::vec3(1.f, 1.f, 1.f));
        for(auto pin: pins) {
            btTransform transform;
            pin->getMotionState()->getWorldTransform(transform);
            transform.getOpenGLMatrix(matrix);
            auto pinModelMatrix = glm::make_mat4(matrix);
            pinModelMatrix = glm::scale(pinModelMatrix, pinDim);
            objectRenderer.SetModelTrasformation(pinModelMatrix);
            bowlingPinModel->Draw();
        }
    };

    auto renderHeightmap = [&](ObjectRenderer &objectRenderer) {
        // plane texture
        objectRenderer.SetTexture(snowTexture, 2.f);
        // drawing the heightmap, for now is positioned with the center at
        glm::mat4 model = glm::mat4(1.0f);
        objectRenderer.SetModelTrasformation(model);
        heightmap.Draw();
    };

    // function for drawing the entire scene, the passed renderer should be active
    auto renderScene = [&](ObjectRenderer &objectRenderer) {
        renderPlane(objectRenderer);
        renderObjects(objectRenderer);
    };

    /// Imgui setup
    {
        IMGUI_CHECKVERSION();
        ImGui::CreateContext();
        ImGuiIO& io = ImGui::GetIO(); (void)io;
        io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;      // Enable Gamepad Controls

        // Setup Dear ImGui style
        ImGui::StyleColorsDark();
        //ImGui::StyleColorsLight();

        // Setup Platform/Renderer backends
        ImGui_ImplGlfw_InitForOpenGL(window, true);
        ImGui_ImplOpenGL3_Init();
    }

    // enable blending for semitransparent particle
    glEnable(GL_ALPHA_TEST);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    // Rendering loop: this code is executed at each frame
    while(!glfwWindowShouldClose(window))
    {
        // we determine the time passed from the beginning
        // and we calculate time difference between current frame rendering and the previous one
        GLfloat currentFrame = glfwGetTime();
        deltaTime = currentFrame - lastFrame;
        lastFrame = currentFrame;

        // Check is an I/O event is happening
        glfwPollEvents();

        // if the is more fast then 100 km/h we activate the turbo using the particle system
        emitter->Active = vehicle.GetSpeed() > 100.f;

        // let the camera follow the vehicle
        updateCameraPosition(vehicle, camera, cameraOffset, deltaTime);

        // View matrix (=camera): position, view direction, camera "up" vector
        // in this example, it has been defined as a global variable (we need it in the keyboard callback function)
        view = camera.GetViewMatrix();

        // move the particle source with the car
        updateEmitterPosition(vehicle, emitter, deltaTime);

        // Snow clear
        if(keys[GLFW_KEY_S]) {
            previousFrameHeightmap.Clear();
        }

        /// key handling
        // if space is pressed and we waited at least 'shootCooldown' since the last bullet
        if(keys[GLFW_KEY_SPACE]) {
            /// bullet management (space key)
            // if space is pressed, we "shoot" a bullet in the scene
            vehicle.Shoot();
        }

        /// vehicle input handling
        // if R is pressed, we reset the position of the car
        if(keys[GLFW_KEY_R]) {
            // TODO: should be easy to set up a simple blink animation
            vehicle.ResetRotation();
        }
        
        // acceleration
        if (keys[GLFW_KEY_UP]) {
            vehicle.Accelerate();
        }
        // deceleration
        if (keys[GLFW_KEY_DOWN]) {
            vehicle.Decelerate();
        }
    
        // steering
        if (keys[GLFW_KEY_RIGHT]) {
            vehicle.SteerRight(deltaTime);
        } 
        if (keys[GLFW_KEY_LEFT]) {
            vehicle.SteerLeft(deltaTime);
        }

        vehicle.Update(deltaTime);
        
        // we update the physics simulation. We must pass the deltatime to be used for the update of the physical state of the scene.
        // Bullet works with a default timestep of 120 Hz (1/120 seconds). For smaller timesteps (i.e., if the current frame is computed faster than 1/120 seconds), Bullet applies interpolation rather than actual simulation.
        // In this example, we use deltatime from the last rendering: if it is < 1\120 sec, then we use it (thus "forcing" Bullet to apply simulation rather than interpolation), otherwise we use the default timestep (1/120 seconds) we have set above
        // We also set the max number of substeps to consider for the simulation (=10)
        // The "correct" values to set up the timestep depends on the characteristics and complexity of the physical simulation, the amount of time spent for the other computations (e.g., complex shading), etc.
        // For example, this example, with limited lighting, simple materials, no texturing, works correctly even setting:
        // bulletSimulation.dynamicsWorld->stepSimulation(1.0/120.0,10);
        bulletSimulation.dynamicsWorld->stepSimulation((deltaTime < maxSecPerFrame ? deltaTime : maxSecPerFrame), 10);

        // reactivate depth test
        glEnable(GL_DEPTH_TEST);

        /// REFACTOR: the heightmap height, the resolution should be refactored in a separate class
        /// render the scene from the plane looking to the sky into the heightmap depthbuffer
        // the projection matrix should be big as the heightmap and the near/far planes should match 
        // the distance from the ground up to the maximum height of the heightmap 
        // activate the fbo attached to the heightmap texture
        glViewport(0, 0, HEIGHTMAP_SIZE, HEIGHTMAP_SIZE);
        glBindFramebuffer(GL_FRAMEBUFFER, heightmapFBO);
        glClear(GL_DEPTH_BUFFER_BIT);
        glm::mat4 heightmapProjection = glm::ortho(-heightmap.width / 2, heightmap.width / 2, -heightmap.height / 2, heightmap.height / 2, -1.f, 1.f),
                  heightmapView = glm::lookAt(glm::vec3(0.f, 0.01f, 0.f), glm::vec3(0.f, 1.f, 0.f), glm::vec3(0.f, 0.f, 1.f));
        heightmapDepthRenderer.Activate(heightmapView, heightmapProjection);
        heightmapDepthRenderer.SetTexture(snowTexture);
        heightmapDepthRenderer.SetViewportSize(glm::vec2(HEIGHTMAP_SIZE, HEIGHTMAP_SIZE));
        heightmapDepthRenderer.SetPreviousFrame(previousFrameHeightmap);
        
        /// HACK: drawing a quad on the maximum depth frame to let the fragment
        ///       shader pass on each pixel of the texture
        glm::mat4 quadTrasformation = glm::scale(glm::mat4(1.f), glm::vec3(heightmap.width / 2, heightmap.depth, heightmap.height / 2));
        quadTrasformation = glm::translate(quadTrasformation, glm::vec3(0.f, heightmap.depth, 0.f));
        quadTrasformation = glm::rotate(quadTrasformation, glm::radians(90.f), glm::vec3(1.f, 0.f, 0.f));
        heightmapDepthRenderer.SetModelTrasformation(quadTrasformation);
        drawQuad();

        renderObjects(heightmapDepthRenderer);
        previousFrameHeightmap.CopyFromCurrentFrameBuffer();

        /// rendering the shadow map to the depth map framebuffer
        glViewport(0, 0, SHADOW_WIDTH, SHADOW_HEIGHT);
        glBindFramebuffer(GL_FRAMEBUFFER, depthMapFBO);
        glClear(GL_DEPTH_BUFFER_BIT);

        glm::mat4 lightProjection, lightView;
        calculateShadowMapViewFrustum(view, projection, renderer.lightDirection, &lightView, &lightProjection);

        shadowRenderer.Activate(lightView, lightProjection);
        renderScene(shadowRenderer);

        // render the standard scene
        // reset the framebuffer to main buffer application
        glBindFramebuffer(GL_FRAMEBUFFER, fbo);
        glViewport(0, 0, screenWidth, screenHeight);
        glClearColor(0.1f, 0.1f, 0.1f, 1.0f);
        // we "clear" the frame and z buffer
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        // before render the scene on the main buffer we set the rendering mode
        if (wireframe)
            // Draw in wireframe
            glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
        else
            glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);

        /// Draw: after update all the object in our scene we draw them  
        // We "install" the selected Shader Program as part of the current rendering process
        // we pass projection and view matrices to the Shader Program
        // the renderer object can also render the object on a buffer (like shadow map)
        renderer.Activate(view, projection);
        renderer.SetShadowMap(shadowMap, lightView, lightProjection);
        renderScene(renderer);

        // activate the heightmap renderer with the view/projection transformations of the camera
        heightmapRenderer.Activate(view, projection);
        // set the shadowmap        
        heightmapRenderer.SetShadowMap(shadowMap, lightView, lightProjection);
        heightmapRenderer.SetLightDirection(renderer.lightDirection);
        // depth frame buffer
        heightmapRenderer.SetDepthTexture(heightmapTexture);
        renderHeightmap(heightmapRenderer);

        // update and always draw snow particles
        snowEmitter->Update(deltaTime);
        snowParticleRenderer.Activate(view, projection);
        snowParticleRenderer.Draw();

        // update all particles for turbo
        emitter->Update(deltaTime);
        if(emitter->Active) {
            particleRenderer.Activate(view, projection);
            /// draw particles using the particle renderer
            particleRenderer.Draw();
        }

        /// rendering the skybox
        // we render it after all the other objects, in order to avoid the depth tests as much as possible.
        // we will set, in the vertex shader for the skybox, all the values to the maximum depth. Thus, the environment map is rendered only where there are no other objects in the image (so, only on the background).
        //Thus, we set the depth test to GL_LEQUAL, in order to let the fragments of the background pass the depth test (because they have the maximum depth possible, and the default setting is GL_LESS)
        glDepthFunc(GL_LEQUAL);
        // to have the background fixed during camera movements, we have to remove the translations from the view matrix
        // thus, we consider only the top-left submatrix, and we create a new 4x4 matrix
        auto skyboxView = glm::mat4(glm::mat3(view)); 
        skyboxRenderer.Activate(skyboxView, projection);
        skyboxRenderer.SetTexture(skybox);
        // we render the cube with the environment map
        cubeModel->Draw();
        // we set again the depth test to the default operation for the next frame
        glDepthFunc(GL_LESS);

        // for backbuffer we always reset the wireframe to fill
        // (otherwise we don't see anything)
        glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);

        {
            /// ImGui Dialog
            // Start the Dear ImGui frame
            ImGui_ImplOpenGL3_NewFrame();
            ImGui_ImplGlfw_NewFrame();
            ImGui::NewFrame();

            // Actual ImgGui Dialogs drawing
            // Dialog for Vehicle configuration
            ImGui::Begin("Vehicle");
            ImGui::Text("Speed: %f Km/h", vehicle.GetSpeed());

            ImGui::SeparatorText("Wheel");
            ImGui::SliderFloat("Width", &vehicle.WheelInfo.width, .3f, .6f);
            ImGui::SliderFloat("Radius", &vehicle.WheelInfo.radius, .1f, 1.f);
            ImGui::SliderFloat("Friction", &vehicle.WheelInfo.friction, 1.f, 1000.f);
            ImGui::SeparatorText("Suspension");
            ImGui::SliderFloat("Stiffness", &vehicle.WheelInfo.suspensionStiffness, 0.f, 20.f);
            ImGui::SliderFloat("Damping", &vehicle.WheelInfo.suspensionDamping, 1.f, 10.f);
            ImGui::SliderFloat("Compression", &vehicle.WheelInfo.suspensionCompression, 1.f, 10.f);
            ImGui::SliderFloat("Rest Length", &vehicle.WheelInfo.suspensionRestLength, 0.f, 2.f);
            ImGui::Separator();
            ImGui::SliderFloat("Engine Force", &vehicle.maxEngineForce, 500.0f, 3000.0f);
            ImGui::SliderFloat("Roll Influence", &vehicle.WheelInfo.rollInfluence, 0.0f, 2.0f);
            ImGui::End();

            /*
            /// Options for ggx material parameter
            ImGui::Begin("Illuminance Model");
            ImGui::SliderFloat3("Position", glm::value_ptr(renderer.lightDirection), -1.f, 1.f);
            ImGui::SliderFloat("Diffusive component weight", &illumination.Kd, 0.f, 10.f);
            ImGui::SliderFloat("Roughness", &illumination.alpha, 0.01f, 1.f);
            ImGui::SliderFloat("Fresnel reflectance", &illumination.F0, 0.01f, 1.f);
            ImGui::End();
            */

            /// Options for camera
            ImGui::Begin("Camera");
            ImGui::SliderFloat3("Offset", glm::value_ptr(cameraOffset), -40.f, 40.f);
            ImGui::End();

            // Render ImgGui
            ImGui::Render();
        }

        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

        // back to default framebuffer
        glBindFramebuffer(GL_FRAMEBUFFER, 0); 
        glClearColor(1.0f, 1.0f, 1.0f, 1.0f); 
        glClear(GL_COLOR_BUFFER_BIT);
        // render the framebuffer to the screen
        postprocessing_shader.Use();  
        glDisable(GL_DEPTH_TEST);
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, texture);
        drawQuad();

        // Faccio lo swap tra back e front buffer
        glfwSwapBuffers(window);
    }

    // ImGui Cleanup
    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();

    // when I exit from the graphics loop, it is because the application is closing
    // we delete the Shader Programs and renderers
    renderer.Delete();
    skyboxRenderer.Delete();
    shadowRenderer.Delete();
    heightmapDepthRenderer.Delete();
    heightmapRenderer.Delete();
    snowParticleRenderer.Delete();
    particleRenderer.Delete();
    postprocessing_shader.Delete();
    // we delete the data of the physical simulation
    bulletSimulation.Clear();

    // clear the particle emitter
    emitter->Delete();

    // delete all the models
    delete cubeModel;
    delete sphereModel;
    delete cylinderModel;
    delete carModel;
    delete bridgeModel;
    delete rampModel;
    delete skateParkModel;
    delete bowlingPinModel;
    // we close and delete the created context
    glfwTerminate();
    return 0;
}

//////////////////////////////////////////
// callback for keyboard events
void key_callback(GLFWwindow* window, int key, int scancode, int action, int mode)
{
    // if ESC is pressed, we close the application
    if(key == GLFW_KEY_ESCAPE && action == GLFW_PRESS)
        glfwSetWindowShouldClose(window, GL_TRUE);

    // if 0 is pressed, we activate/deactivate wireframe rendering of models
    if(key == GLFW_KEY_0 && action == GLFW_PRESS)
        wireframe = !wireframe;
    
    // we keep trace of the pressed keys
    // with this method, we can manage 2 keys pressed at the same time:
    // many I/O managers often consider only 1 key pressed at the time (the first pressed, until it is released)
    // using a boolean array, we can then check and manage all the keys pressed at the same time
    if(action == GLFW_PRESS)
        keys[key] = true;
    else if(action == GLFW_RELEASE)
        keys[key] = false;
}

//////////////////////////////////////////
// callback for mouse events
void mouse_callback(GLFWwindow* window, double xpos, double ypos)
{
    // used fixed camera for now
}
