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
#include <utils/shader.h>
#include <utils/model.h>
#include <utils/camera.h>
#include <utils/physics.h>
#include <utils/vehicle.h>
#include <utils/particle.h>
#include <utils/texture.h>
#include <utils/mesh_renderer.h>

#include <imgui/imgui.h>
#include <imgui/imgui_impl_glfw.h>
#include <imgui/imgui_impl_opengl3.h>

// we load the GLM classes used in the application
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/matrix_inverse.hpp>
#include <glm/gtc/type_ptr.hpp>

// Dimensioni della finestra dell'applicazione
GLuint screenWidth = 1400, screenHeight = 1100;

// callback functions for keyboard and mouse events
void key_callback(GLFWwindow* window, int key, int scancode, int action, int mode);
void mouse_callback(GLFWwindow* window, double xpos, double ypos);

// we initialize an array of booleans for each keyboard key
bool keys[1024];

// we need to store the previous mouse position to calculate the offset with the current frame
GLfloat lastX, lastY;

// we will use these value to "pass" the cursor position to the keyboard callback, in order to determine the bullet trajectory
double cursorX,cursorY;

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


// vertex attributes for a quad that fills the entire screen in Normalized Device Coordinates
float quadVertices[] = { 
    // positions   // texCoords
    -1.f, 1.0f,  0.0f, 1.0f,
    -1.f, -1.f,  0.0f, 0.0f,
    1.f,  -1.f,  1.0f, 0.0f,

    -1.f, 1.0f,  0.0f, 1.0f,
    1.f,  -1.f,  1.0f, 0.0f,
    1.f,  1.0f,  1.0f, 1.0f
};

void drawQuad() {
    glBindVertexArray(quadVAO);
    glDrawArrays(GL_TRIANGLES, 0, 6);
}

glm::vec3 toGLM(const btVector3 &v) {
    return glm::vec3(v.getX(), v.getY(), v.getZ());
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

// TODO: light bug (not happen if the function is "inlined")
void drawRigidBody(MeshRenderer &renderer, btRigidBody *body) {
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
    Physics &bulletSimulation = Physics::getInstance();

    MeshRenderer renderer;
    /// Uniforms for the illumination model
    renderer.illumination.lightPosition = glm::vec3(5.0f, 20.0f, 20.0f);
    renderer.illumination.Kd = 3.0f;
    renderer.illumination.alpha = 0.6f;
    renderer.illumination.F0 = 0.9f;


    // the Shader Program for rendering the particles
    Shader particle_shader = Shader("particle.vert", "particle.frag");
    // the Shader Program used on the framebuffer, for effect on the whole screen
    Shader postprocessing_shader = Shader("postprocessing.vert", "postprocessing.frag");

    // we load the model(s) (code of Model class is in include/utils/model.h)
    cubeModel = new Model("../models/cube.obj");
    sphereModel = new Model("../models/sphere.obj");
    carModel = new Model("../models/mclaren.obj");
    cylinderModel = new Model("../models/cylinder.obj");

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
    glm::vec3 plane_size = glm::vec3(200.0f, 0.1f, 200.0f);
    glm::vec3 plane_rot = glm::vec3(0.0f, 0.0f, 0.0f);

    // we create a rigid body for the plane. In this case, it is static, so we pass mass = 0;
    // in this way, the plane will not fall following the gravity force.
    btRigidBody *plane = bulletSimulation.createRigidBody(BOX, plane_pos, plane_size, plane_rot, 0.0f, 0.3f, 0.3f);

    /// create particles
    auto totalParticles = 500;
    auto emitter = new ParticleEmitter(totalParticles);
    {
        // set the random variables
        emitter->Size0     = 0.01f;
        emitter->Size1     = 0.05f;
        emitter->Rotation0 = 0.1f;
        emitter->Rotation1 = 5.f;
        emitter->Lifespan0 = 1.f;
        emitter->Lifespan1 = 3.5f;
        emitter->Velocity  = glm::vec3(0.f, 2.f, 0.f);
        emitter->DeltaVelocity0 = glm::vec3( .2f, 2.f,  .2f);
        emitter->DeltaVelocity1 = glm::vec3(-.2f, 0.f, -.2f);
        emitter->Color0    = glm::vec3(.8f, .1f, .1f);
        emitter->Color1    = glm::vec3(.5f, .5f, .0f);
        emitter->Scale0    = glm::vec3(1.f, 1.f, 1.f);
        emitter->Scale1    = glm::vec3(1.f, 1.f, 1.f);
        emitter->Alpha0    = .7f;
        emitter->Alpha1    = 1.f;
        emitter->Gravity   = glm::vec3(0.f, -1.2f, 0.f);
    }

    // set the type of the particle 
    GLuint shapeSubroutine = glGetSubroutineIndex(particle_shader.Program, GL_FRAGMENT_SHADER, "Circle");
    glUniformSubroutinesuiv(GL_FRAGMENT_SHADER, 1, &shapeSubroutine);

    auto particleColors = new glm::vec4[totalParticles];
    auto modelMatrices = new glm::mat4[totalParticles];
    for(int i = 0; i < totalParticles; i++) {
        modelMatrices[i] = glm::mat4(1.0f);
    }

    // initializate buffers (VAO, VBO) for particle rendering
    // creating particle VAO/VBO, different from normal quads cause of instanced drawing
    GLuint particleVAO;
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
    GLuint modelMatrixBuffer;
    glGenBuffers(1, &modelMatrixBuffer);
    glBindBuffer(GL_ARRAY_BUFFER, modelMatrixBuffer);
    auto bufferSize = totalParticles * sizeof(glm::mat4);
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

    // buffer for storing color particles
    GLuint particleColorBuffer;
    glGenBuffers(1, &particleColorBuffer);
    glBindBuffer(GL_ARRAY_BUFFER, particleColorBuffer);
    auto colorBufferSize = totalParticles * sizeof(glm::vec4);
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


    /// create vehicle
    glm::vec3 chassisBox(1.f, .5f, 2.f);
    // default wheel infos
    WheelInfo wheelInfo;
    Vehicle vehicle(chassisBox, wheelInfo);

    // TODO: to remove, generalize to obstacle array
    // we create 25 rigid bodies for the cubes of the scene. In this case, we use BoxShape, with the same dimensions of the cubes, as collision shape of Bullet. For more complex cases, a Bounding Box of the model may have to be calculated, and its dimensions to be passed to the physics library
    GLint num_side = 0;
    // total number of the cubes
    GLint total_cubes = num_side*num_side;
    GLint i,j;
    // position of the cube
    glm::vec3 cube_pos;
    // dimension of the cube
    glm::vec3 cube_size = glm::vec3(.2f, .5f, .2f);
    // we set a small initial rotation for the cubes
    glm::vec3 cube_rot = glm::vec3(0.1f, 0.0f, 0.1f);
    // rigid body
    btRigidBody* cube;

    int cubes_start_i = bulletSimulation.dynamicsWorld->getCollisionObjectArray().size();
    // we create a 5x5 grid of rigid bodies
    for(i = 0; i < num_side; i++ )
    {
        for(j = 0; j < num_side; j++ )
        {
            // position of each cube in the grid (we add 3 to x to have a bigger displacement)
            cube_pos = glm::vec3((i - num_side)+3.0f, 1.0f, (num_side - j));
            // we create a rigid body (in this case, a dynamic body, with mass = 2)
            cube = bulletSimulation.createRigidBody(BOX, cube_pos, cube_size, cube_rot, 2.0f, 0.3f, 0.3f);
        }
    }

    // creating ramp
    bulletSimulation.createRigidBody(BOX, glm::vec3(-10.f, -2.f, 0.f), glm::vec3(3.f), glm::vec3(0.f, 0.f, glm::radians(60.f)), 0, 0.3f, 0.3f);

    // we set the maximum delta time for the update of the physical simulation
    GLfloat maxSecPerFrame = 1.0f / 60.0f;

    // Projection matrix: FOV angle, aspect ratio, near and far planes
    projection = glm::perspective(45.0f, (float)screenWidth/(float)screenHeight, 0.1f, 10000.0f);

    // Model and Normal transformation matrices for the objects in the scene: we set to identity
    glm::mat4 objModelMatrix = glm::mat4(1.0f);
    glm::mat4 planeModelMatrix = glm::mat4(1.0f);
    // textures for plane
    // Texture planeTexture("../textures/DirtFloor.jpg");
    // Texture planeNormalMap("../textures/DirtFloor_NormalMap.png");
    // Texture planeDisplacementMap("../textures/DirtFloor_DispMap.png");
    Texture planeTexture("../textures/Stone.jpg");
    Texture planeNormalMap("../textures/Stone_NormalMap.jpg");
    Texture planeDisplacementMap("../textures/Stone_DispMap.jpg");
    Texture grassTexture("../textures/Stone.jpg");
    Texture grassNormalMap("../textures/Stone_NormalMap.jpg");
    Texture grassDisplacementMap("../textures/Stone_DispMap.jpg");
    
    // Texture grassTexture("../textures/Grass.jpg");
    // Texture grassNormalMap("../textures/Grass_NormalMap.jpg");

    // constant distance from the camera and the vehicle
    glm::vec3 cameraOffset(0.f, 15.f, -25.f);

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

        glBindFramebuffer(GL_FRAMEBUFFER, fbo);
        glClearColor(0.1f, 0.1f, 0.1f, 1.0f);
        // we "clear" the frame and z buffer
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        glEnable(GL_DEPTH_TEST);

        // we set the rendering mode
        if (wireframe)
            // Draw in wireframe
            glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
        else
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

            /// Options for camera
            ImGui::Begin("Illuminance Model");
            auto &illumination = renderer.illumination;
            ImGui::SliderFloat3("Position", glm::value_ptr(illumination.lightPosition), -50.f, 50.f);
            // do not allow the light to go under the plane
            if(illumination.lightPosition.y < 0.f) illumination.lightPosition.y = -illumination.lightPosition.y;
            ImGui::SliderFloat("Diffusive component weight", &illumination.Kd, 0.f, 10.f);
            ImGui::SliderFloat("Roughness", &illumination.alpha, 0.01f, 1.f);
            ImGui::SliderFloat("Fresnel reflectance", &illumination.F0, 0.01f, 1.f);
            ImGui::End();

            /// Options for camera
            ImGui::Begin("Camera");
            ImGui::SliderFloat3("Offset", glm::value_ptr(cameraOffset), -40.f, 40.f);
            ImGui::End();

            // Render ImgGui
            ImGui::Render();
        }

        updateCameraPosition(vehicle, camera, cameraOffset, deltaTime);

        // View matrix (=camera): position, view direction, camera "up" vector
        // in this example, it has been defined as a global variable (we need it in the keyboard callback function)
        view = camera.GetViewMatrix();

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
            vehicle.SteerRight();
        } 
        if (keys[GLFW_KEY_LEFT]) {
            vehicle.SteerLeft();
        }

        vehicle.Update(deltaTime);
        
        // we update the physics simulation. We must pass the deltatime to be used for the update of the physical state of the scene.
        // Bullet works with a default timestep of 60 Hz (1/60 seconds). For smaller timesteps (i.e., if the current frame is computed faster than 1/60 seconds), Bullet applies interpolation rather than actual simulation.
        // In this example, we use deltatime from the last rendering: if it is < 1\60 sec, then we use it (thus "forcing" Bullet to apply simulation rather than interpolation), otherwise we use the default timestep (1/60 seconds) we have set above
        // We also set the max number of substeps to consider for the simulation (=10)
        // The "correct" values to set up the timestep depends on the characteristics and complexity of the physical simulation, the amount of time spent for the other computations (e.g., complex shading), etc.
        // For example, this example, with limited lighting, simple materials, no texturing, works correctly even setting:
        // bulletSimulation.dynamicsWorld->stepSimulation(1.0/60.0,10);
        bulletSimulation.dynamicsWorld->stepSimulation((deltaTime < maxSecPerFrame ? deltaTime : maxSecPerFrame), 10);

        /////////////////// OBJECTS ////////////////////////////////////////////////
        // We "install" the selected Shader Program as part of the current rendering process
        // we pass projection and view matrices to the Shader Program
        renderer.Activate(view, projection);

        // The plane is static, so its Collision Shape is not subject to forces, and it does not move. Thus, we do not need to use dynamicsWorld to acquire the rototraslations, but we can just use directly glm to manage the matrices
        // if, for some reason, the plane becomes a dynamic rigid body, the following code must be modified
        // we reset to identity at each frame
        planeModelMatrix = glm::mat4(1.0f);
        planeModelMatrix = glm::translate(planeModelMatrix, plane_pos);
        planeModelMatrix = glm::scale(planeModelMatrix, plane_size);
        renderer.SetModelTrasformation(planeModelMatrix);
        
        // set texture for the plane
        renderer.SetTexture(planeTexture);
        // set the normal map
        renderer.SetNormalMap(planeNormalMap);
        // set the displacement texture that is used as parallax map
        renderer.SetParallaxMap(planeDisplacementMap, .1f);

        // renderer.SetTerrainTexture(planeTexture, planeNormalMap, grassTexture, grassNormalMap);

        // we render the plane
        cubeModel->Draw();
        planeModelMatrix = glm::mat4(1.0f);

        // do not use parallax map anymore but restore uv coordinate interpolation
        renderer.SetTexCoordinateCalculation(UV);
        // reset normals calculation in vertex shader with normal matrix not anymore with normal map
        renderer.SetNormalCalculation(FROM_MATRIX);
        /// DYNAMIC OBJECTS (FALLING CUBES + BULLETS)
        // array of 16 floats = "native" matrix of OpenGL. We need it as an intermediate data structure to "convert" the Bullet matrix to a GLM matrix
        GLfloat matrix[16];
        btTransform transform;

        // we need two variables to manage the rendering of both cubes and bullets
        glm::vec3 obj_size;
  
        // we ask Bullet to provide the total number of Rigid Bodies in the scene
        int num_cobjs = bulletSimulation.dynamicsWorld->getNumCollisionObjects();

        // draw vehicle
        {
            auto bulletVehicle = vehicle.GetBulletVehicle();
            // drawing the chassis
            obj_size = chassisBox;
            renderer.SetColor(carColor);

            // we take the transformation matrix of the rigid boby, as calculated by the physics engine
            transform = bulletVehicle.getChassisWorldTransform();

            // we convert the Bullet matrix (transform) to an array of floats
            transform.getOpenGLMatrix(matrix);

            // we reset to identity at each frame
            objModelMatrix = glm::mat4(1.0f);

            // we create the GLM transformation matrix
            // 1) we convert the array of floats to a GLM mat4 (using make_mat4 method)
            // 2) Bullet matrix provides rotations and translations: it does not consider scale (usually the Collision Shape is generated using directly the scaled dimensions). If, like in our case, we have applied a scale to the original model, we need to multiply the scale to the rototranslation matrix created in 1). If we are working on an imported and not scaled model, we do not need to do this
            objModelMatrix = glm::make_mat4(matrix) * glm::scale(objModelMatrix, obj_size);
            // we create the normal matrix
            renderer.SetModelTrasformation(objModelMatrix);
            carModel->Draw();

            // we "reset" the matrix
            objModelMatrix = glm::mat4(1.0f);

            // draw wheels
            for(int wheel = 0; wheel < bulletVehicle.getNumWheels(); wheel++) {
                auto wheelWidth = wheelInfo.width;
                auto wheelRadius = wheelInfo.radius;
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

        // we cycle among all the Rigid Bodies (starting from 1 to avoid the plane)
        for (i=cubes_start_i; i<num_cobjs; i++)
        {
            // we take the Collision Object from the list
            btCollisionObject *obj = bulletSimulation.dynamicsWorld->getCollisionObjectArray()[i];
            // we upcast it in order to use the methods of the main class RigidBody
            btRigidBody *body = btRigidBody::upcast(obj);

            drawRigidBody(renderer, body);
        }


        // update all particles
        emitter->Update(deltaTime);

        /// draw particles 
        // initialize the particle shader
        particle_shader.Use();
        // we pass projection and view matrices to the particle Shader Program
        glUniformMatrix4fv(glGetUniformLocation(particle_shader.Program, "projectionMatrix"), 1, GL_FALSE, glm::value_ptr(projection));
        glUniformMatrix4fv(glGetUniformLocation(particle_shader.Program, "viewMatrix"), 1, GL_FALSE, glm::value_ptr(view));
        // calculate all colors andf model matrix for each particle
        for(int i = 0; i < emitter->size(); i++) {
            auto particle = emitter->particles[i];
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
        glDrawArraysInstanced(GL_TRIANGLES, 0, 6, totalParticles);
        glBindVertexArray(0);

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
    // we delete the Shader Programs
    renderer.Delete();
    particle_shader.Delete();
    postprocessing_shader.Delete();
    // we delete the data of the physical simulation
    bulletSimulation.Clear();

    // clear the particle emitter
    emitter->Delete();

    delete cubeModel;
    delete sphereModel;
    delete cylinderModel;
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
        wireframe=!wireframe;
    
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
