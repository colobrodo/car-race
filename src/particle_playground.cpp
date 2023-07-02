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
#include <utils/particle.h>

#include <imgui/imgui.h>
#include <imgui/imgui_impl_glfw.h>
#include <imgui/imgui_impl_opengl3.h>

// we load the GLM classes used in the application
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/matrix_inverse.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtx/intersect.hpp>

// we include the library for images loading
#define STB_IMAGE_IMPLEMENTATION
#include <stb_image/stb_image.h>

// Dimensioni della finestra dell'applicazione
GLuint screenWidth = 1400, screenHeight = 1100;

// callback functions for keyboard and mouse events
void key_callback(GLFWwindow* window, int key, int scancode, int action, int mode);
void mouse_callback(GLFWwindow* window, double xpos, double ypos);

// we initialize an array of booleans for each keyboard key
bool keys[1024];

// we need to store the previous mouse position to calculate the offset with the current frame
GLfloat lastX, lastY;

// when rendering the first frame, we do not have a "previous state" for the mouse, so we need to manage this situation
bool firstMouse = true;

// parameters for time calculation (for animations)
GLfloat deltaTime = 0.0f;
GLfloat lastFrame = 0.0f;

// view and projection matrices (global because we need to use them in the keyboard callback)
glm::mat4 view, projection;

// we create a camera. We pass the initial position as a parameter to the constructor. In this case, we use a "floating" camera (we pass false as last parameter)
Camera camera(glm::vec3(0.0f, 2.0f, 9.0f), GL_FALSE);

/// Uniforms to be passed to shaders
// point light position
glm::vec3 lightPos0 = glm::vec3(5.0f, 20.0f, 20.0f);

// weight for the diffusive component
GLfloat Kd = 3.0f;
// roughness index for GGX shader
GLfloat alpha = 0.2f;
// Fresnel reflectance at 0 degree (Schlik's approximation)
GLfloat F0 = 0.9f;

// color of the falling objects
GLfloat diffuseColor[] = {1.0f,0.0f,0.0f};
GLfloat carColor[] = {.8f, .8f, .8f};
// color of the plane
GLfloat planeMaterial1[] = {.6f, .6f, .7f};
GLfloat planeMaterial2[] = {.3f, .3f, .3f};
// color of the bullets
GLfloat shootColor[] = {1.0f,1.0f,0.0f};
// dimension of the bullets (global because we need it also in the keyboard callback)
glm::vec3 sphereSize = glm::vec3(0.2f, 0.2f, 0.2f);

unsigned int quadVAO, quadVBO;

// particle emitter
ParticleEmitter *emitter;

// models 
Model *cubeModel;
Model *sphereModel;
Model *cylinderModel;


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


    // the Shader Program for rendering the particles
    Shader particle_shader = Shader("particle.vert", "particle.frag");
    // the Shader Program used on the framebuffer, for effect on the whole screen
    Shader postprocessing_shader = Shader("postprocessing.vert", "postprocessing.frag");

    // we load the model(s) (code of Model class is in include/utils/model.h)
    // unused for particle system
    cubeModel = new Model("../models/cube.obj");
    sphereModel = new Model("../models/sphere.obj");
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
    //glBindVertexArray(0);


    /// create particles
    auto size = 10000;
    emitter = new ParticleEmitter(size);
    {
        
        // initialize the random variables for the emitter
        emitter->Size0     = 0.025f;
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
        // spawn all the particle in the same point as default
        emitter->spawnShape = POINT;
    }

    auto particleColors = new glm::vec4[size];
    auto modelMatrices = new glm::mat4[size];
    for(int i = 0; i < size; i++) {
        modelMatrices[i] = glm::mat4(1.0f);
    }

    // creating particle VAO/VBO, different from normal quads cause of instancing
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
    auto bufferSize = size * sizeof(glm::mat4);
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
    auto colorBufferSize = size * sizeof(glm::vec4);
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


    GLint i,j;


    // we set the maximum delta time for the update of the physical simulation
    GLfloat maxSecPerFrame = 1.0f / 60.0f;

    // Projection matrix: FOV angle, aspect ratio, near and far planes
    projection = glm::perspective(45.0f, (float)screenWidth/(float)screenHeight, 0.1f, 10000.0f);

    // Model transformation matrix for the objects in the scene: we set to identity
    glm::mat4 objModelMatrix = glm::mat4(1.0f);

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

    GLuint circleShapeIndex = glGetSubroutineIndex(particle_shader.Program, GL_FRAGMENT_SHADER, "Circle");
    GLuint squareShapeIndex = glGetSubroutineIndex(particle_shader.Program, GL_FRAGMENT_SHADER, "Square");
    // shape names used in combobox
    const char *shapes[] = {"Circle", "Square"};
    // map array used to map combo indexes to subroutine indexes  
    const GLuint subroutineIndexes[] = {circleShapeIndex, squareShapeIndex};
    // imgui combobox choice index (different from opengl index)
    int comboIndex = 0;

    const char *spawnShapeNames[] = {"Point", "Disc", "Rectangle"};
    const SpawnShape spawnShapes[] = {POINT, DISC, RECTANGLE};
    int spawnShapeCombo = 0;

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

        glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);

        {
            /// ImGui Dialog
            // Start the Dear ImGui frame
            ImGui_ImplOpenGL3_NewFrame();
            ImGui_ImplGlfw_NewFrame();
            ImGui::NewFrame();

            // Actual ImgGui Dialogs drawing

            /// Options for particle system
            ImGui::Begin("Particle");
            // spawn shape 
            ImGui::Combo("Spawn Shape", &spawnShapeCombo, spawnShapeNames, IM_ARRAYSIZE(spawnShapeNames));
            // we check if we have updated the spawn shape
            auto choosenSpawnShape = spawnShapes[spawnShapeCombo];
            if(emitter->spawnShape != choosenSpawnShape) {
                // we have updated the spawn shape
                // we set the default parameters read later by the widgets
                switch (choosenSpawnShape) {
                    case DISC:
                        emitter->spawnRadius = 1.0f;
                        break;
                    case RECTANGLE:
                        emitter->spawnRectSize.height = 0.5f;
                        emitter->spawnRectSize.width = 2.0f;
                        break;
                }
                // update the type of spawn shape with the choosen one
                emitter->spawnShape = choosenSpawnShape;
            }
            // create widgets (sliders) for each parameter of the choosen spawn shape
            // the shape is centered around the position
            switch (choosenSpawnShape) {
                // disc spawn radius
                case DISC:
                    ImGui::SliderFloat("Spawn Radius", &emitter->spawnRadius, 0.01f, 3.f);
                    break;
                // for rectangle height and width
                case RECTANGLE:
                    ImGui::SliderFloat("Spawn Height", &emitter->spawnRectSize.height, 0.01f, 3.f);
                    ImGui::SliderFloat("Spawn Width", &emitter->spawnRectSize.width, 0.01f, 3.f);
                    break;
                // point no parameters, handler by ctrl + mouse drag
            }
            // get the rest of the parameters, mostly random variables bounds
            ImGui::Combo("Shape", &comboIndex, shapes, IM_ARRAYSIZE(shapes));
            ImGui::SeparatorText("Directions");
            ImGui::SliderFloat3("Velocity", glm::value_ptr(emitter->Velocity), -3.f, 3.f);
            ImGui::SliderFloat3("DeltaVelocity0", glm::value_ptr(emitter->DeltaVelocity0), -3.f, 3.f);
            ImGui::SliderFloat3("DeltaVelocity1", glm::value_ptr(emitter->DeltaVelocity1), -3.f, 3.f);
            ImGui::SliderFloat3("Gravity", glm::value_ptr(emitter->Gravity), -3.f, 3.f);
            ImGui::SeparatorText("Colors");
            ImGui::ColorEdit3("Color0", glm::value_ptr(emitter->Color0));
            ImGui::ColorEdit3("Color1", glm::value_ptr(emitter->Color1));
            ImGui::SliderFloat("Alpha0", &emitter->Alpha0, 0.01f, 1.f);
            ImGui::SliderFloat("Alpha1", &emitter->Alpha1, 0.01f, 1.f);
            ImGui::Separator();
            ImGui::SliderFloat3("Scale0", glm::value_ptr(emitter->Scale0), .5f, 2.f);
            ImGui::SliderFloat3("Scale1", glm::value_ptr(emitter->Scale1), .5f, 2.f);
            ImGui::SliderFloat("Size0", &emitter->Size0, 0.005f, 1.f);
            ImGui::SliderFloat("Size1", &emitter->Size1, 0.01f, 1.f);
            ImGui::SliderFloat("Rotation0", &emitter->Rotation0, 0.1f, 5.f);
            ImGui::SliderFloat("Rotation1", &emitter->Rotation1, 0.1f, 5.f);
            ImGui::SliderFloat("Lifespan0", &emitter->Lifespan0, 0.1f, 5.f);
            ImGui::SliderFloat("Lifespan1", &emitter->Lifespan1, 0.1f, 5.f);
            ImGui::End();

            // Render ImgGui
            ImGui::Render();
        }

        // View matrix (=camera): position, view direction, camera "up" vector
        // in this example, it has been defined as a global variable (we need it in the keyboard callback function)
        view = camera.GetViewMatrix();

        // update all particles
        emitter->Update(deltaTime);

        /// draw particles 
        // initialize the particle shader
        particle_shader.Use();
        // set the subroutine to choose the particle shape based on combobox choice
        glUniformSubroutinesuiv(GL_FRAGMENT_SHADER, 1, &subroutineIndexes[comboIndex]);

        // we pass projection and view matrices to the particle Shader Program
        glUniformMatrix4fv(glGetUniformLocation(particle_shader.Program, "projectionMatrix"), 1, GL_FALSE, glm::value_ptr(projection));
        glUniformMatrix4fv(glGetUniformLocation(particle_shader.Program, "viewMatrix"), 1, GL_FALSE, glm::value_ptr(view));
        
        // for now draw all the particles in a fixed point, later read the spawn point from the particle emitter
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
        glDrawArraysInstanced(GL_TRIANGLES, 0, 6, size);
        glBindVertexArray(0);


        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

        glBindVertexArray(quadVAO);
        // back to default framebuffer
        glBindFramebuffer(GL_FRAMEBUFFER, 0); 
        glClearColor(1.0f, 1.0f, 1.0f, 1.0f); 
        glClear(GL_COLOR_BUFFER_BIT);
        // render the framebuffer to the screen
        postprocessing_shader.Use();  
        glDisable(GL_DEPTH_TEST);
        glBindTexture(GL_TEXTURE_2D, texture);
        drawQuad();
        glBindVertexArray(0);

        // Faccio lo swap tra back e front buffer
        glfwSwapBuffers(window);
    }

    // ImGui Cleanup
    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();

    // when I exit from the graphics loop, it is because the application is closing
    // we delete the Shader Programs
    particle_shader.Delete();
    postprocessing_shader.Delete();

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
void mouse_callback(GLFWwindow *window, double cursorX, double cursorY)
{
    // used fixed camera for now

    // pickling: get a point more close to the camera as possible on the cursor coordinate
    // trasform this point to world coordinate
    // trace a ray from the camera
    // find the intersection between this ray and the plane z=0

    int state = glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_LEFT);
    // check also for shift key on move to avoid unwanted drag while tweeking imgui parameters
    if(keys[GLFW_KEY_LEFT_CONTROL] && state == GLFW_PRESS) {
        glm::vec4 position;
        // we must retro-project the coordinates of the mouse pointer, in order to have a point in world coordinate to be used to determine a vector from the camera (= direction and orientation of the bullet)
        // we convert the cursor position (taken from the mouse callback) from Viewport Coordinates to Normalized Device Coordinate (= [-1,1] in both coordinates)
        position.x = (cursorX / screenWidth) * 2.0f - 1.0f;
        position.y = -(cursorY / screenHeight) * 2.0f + 1.0f; // Viewport Y coordinates are from top-left corner to the bottom
        // we need a 3D point, so we set the min value to the depth with respect to camera position
        position.z = 1.0f;
        // w = 1.0 because we are using homogeneous coordinates
        position.w = 1.0f;

        // we determine the inverse matrix for the projection and view transformations
        auto unproject = glm::inverse(projection * view);
        position = unproject * position;

        // create the ray from the closest position to the camera
        glm::vec3 picklingPoint(position.x / position.w, position.y / position.w, position.z / position.w);
        glm::vec3 picklingDir = picklingPoint - camera.Position;
        // find the ray intersection with the z=0 plane
        float t;
        glm::intersectRayPlane(camera.Position, picklingDir, 
                               glm::vec3(0.f, 0.f, 0.f), glm::vec3(0.f, 0.f, 1.f),
                               t);
        auto intersectionPoint = camera.Position + picklingDir * t;
        emitter->Position = intersectionPoint;
    }
}