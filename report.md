% Car race
% Project for the course Realtime Graphics Programming
% Davide Cologni

\tableofcontents

# Introduction
The goal of this project is to implement an interactive car game with several graphical feature.
The application is developed in C++, because this language provide manual memory control.
I develop the project on Windows 11, using the Microsoft Visual Studio compiler.

## Used Libraries
To implement this project I used the following libraries:   
- **OpenGL** is an API to develop 3D computer graphics application, it provides a way to control and program the rendering pipeline   
- **Bullet** is a physics engine that simulates collision detection and soft and rigid body dynamics   
- **Assimp**   
- **Dear IMGui** a Immediate mode GUI library to tweak the parameters of the application at runtime.   
- **stb image** a header-only library to load images in memory (used for texturing and normal mapping)   

# Project description
## Vehicle
**TODO: check grammar**
I implemented a vehicle using the class `btRaycastVehicle` provided by Bullet.   
Usualy to model a vehicle in a physic engine you specify each component of the car (as wheel, suspension, chassis ecc...) as a rigid body with physic property.   
For example you can specify a spring for each suspension and a object with a specific restitution for the wheels.   
This techinique has the disavantages to be complex, very difficult to setup all the parameters of each rigid body to work well with the others and more expensive for the physics engine due to his number of objects and constrains.  
A more simple technique used in games is a Raycast Vehicle.
In this case only the chassis is modelled as a rigid body and the wheel are simulated using a ray that cast on the terrain and apply an opposite force to the car.
**TODO: Explanatory Image of RaycastVehicle**
In this way the physics engine need only to apply one force for each wheels and only resolve the collision with the chassis and the other object.
**TODO: Image of vehicle with all his feature like jumping colliding, rebalting ecc**

## Snow
Among the graphical effects I decided to implement, I also chose to simulate snow that is sensitive to the depth of the object stepping on it.

The snow effect is implemented in two step:   
First of all we create a texture with the height of the snow (heightmap).   
After that we draw a quad and we tesselate them displacing any created vertices by the value in the texture.   

To create the heightmap we use two texture:   
One used to remember the height of the snow in the previous frames, and the other one contains the depth of the object on top of the snow in the current frame.   
The two textures are then combined in the shader using an operation of min pixel per pixel.   
In order to know how much the object sink in the snow, I render the whole scene in a depth buffer, placing the camera on the floor with an orthogonal projection.  

![snow Difference](./img/snow-difference.png)
On the left the car in the snow with low suspension. On the right, the height of the suspension has been increased and the shape of the wheels is clearly distinguishable from the chassis.


### Tesselation
**TODO: explain what is tesselation**   

In the Hull shader I sample the texture obtained from the depth buffer, and calculate the displacement of the vertex multipling this value by the height of the snow.
Then I calculate the new position of the vertex, changing his y axis to the height just retrived.

I used the `sampler2DShadow` texture type provided by OpenGL to automatically apply a bilinear filter to the neighbouring pixels to get a softer effect and avoid sharp edges that would otherwise make the snow unrealistic.   

In the fragment shader I used an image of snow as a texture.   
I also interpolated the color of the fragment with a grey tonality based on the height to give a better sense of depth.   

## Particles

To simulate graphical effect like snow, turbo on acceleration I implemented a particle system.   
The particle system is responsible to create a large number of small objects (called particles) which together give the feeling of a more complete effect.   

![Turbo](./img/turbo.png)   
Image of a particle emitter attached on the vehicle to simulate fire when the car accelerate.   

![Snow](./img/falling-snow.png)
Here the effect of the snow falling is given placing a particle emitter on the sky and set a very low gravity force to the particles.   

Each emitter have a world position and all the elements position are relative to the one of his emitter.      
In this way you can attach particle emitter to other object in the scene (like some fire on a torch).   
You can choose to spawn the elements on a point or on an area.   
To customise the appearance of the effect it is possible to set a list of random parameters such as the particle's speed, direction, lifetime, colour, alpha parameter and size.   
The particle is not created with a value set by a user, what can be controlled is a range (minimum and maximum) in which the parameter can be randomly sampled.    

**TODO: video**   
![playground](./img/playground.png)
In the picture a playground used to test the particles.   
On the left a Dear ImGUI dialogue to manage all the random parameters of the particle system.    

Given the large number of objects required to give a credible effect, I rendered the particles using the instancing technique.   
Instancing allows you to render multiple objects with the same set of vertices, with only one render call to the GPU.   
In this way you can avoid comunicate the mesh data on each object you render, saturating the CPU to GPU bus.   
Each instance of the mesh can have uniforms with different values (like the position of the object or the color in our case).   
This values are stored in a buffer that is passed in one render call to the graphic card.   
Our particle emitter uses two buffer: one with a trasformation matrix that describe the rotation and the position of the particle in the world and another with a `vec4` color.   
On each frame, before sending it to the GPU, we update the values on the buffer calculating the new position of the particle based on his velocity, acceleration and his old position.    
Updating the particles on the CPU is not the best choice in term of performance: using compute shader would avoid comunication with the graphic card but also without this technique I'm able to instatiate a big number of particles more than enough to simulate the wanted effects.    
Note that even if the particles have a velocity, a position and an acceleration they don't interact with the bullet world: this bcause it would be too computationaly expensive given the large number of element.    
  
To avoid write a custom memory allocator, the number of particles per emitter is fixed.
When an element expire and consume all his lifetime another particle is created with random property and allocated on the memory location of the previous one.     


## Shadow Mapping
In the scene the objects have the ability to cast shadow, this feature is implemented using the shadow mapping technique.   
We first render the whole scene from the point of view of the light in a depth buffer.   
The object we can see from this perspective are the ones illuminated, and the other one are in shadow.   
After that when we render the scene, and we pass to the fragment shader the previously rendered depth buffer as a texture.   
We calculate the view position of each fragment from the point of view of the light.    
After that we use the view position x, y coordinate to sample the depth buffer, and we compare the z coordinate with the value in the shadow map: if the z is greater this means the light is occluded from another object.   
To simulate the shadow on the fragment we multiply all the channels of the color by a factor, in this way we decrease luminance.   
In my application I only use one source of directional light.    

### Poisson Sampling 
To avoid sharp, unrealistic edge on the shadows I use a technique called Poisson Sampling.   
Like Percentage Close Filtering it samples the shadow map multiple time and calculate the average.   
Poisson Sampling differs from PCF because it don't sample the 4 neighbour pixels but it choose from a presampled poisson disk distribution which other pixel to sample.    
The disc can have different dimension, in my application it contains 4 samples.    
I choose poisson sampling instead of standard PCF because I found the shadows more realistic and more soft.    

![Shadow](./img/shadow.png) 
An image with shadows casted on the snow and on the bridge.

## Obstacles
In the scene are present different type of obstacles.   
Each obstacle can interact with the others and the vehicle.    
To do so when we create an object in the scene we add it to the Bullet world object creating a solid object.
Among the other properties of the solid there is also his `BtCollisionShape`, that tell Bullet the shape of the object.   
Objects can have different `BtCollisionShape` than the shape rendered on the screen.   
Some of the obstacles have complex shape and others are designed as primitive like cubes or spheres.   
For primitive obstacles is easy to add it to the world, Bullet provide some classes that already handle the collision with other objects like Box, Sphere.   
For objects with more complex mesh is necessary distinguish between two cases: when the object is dynamic and the case when the object is static.   

For static obstacles, like the bridge in our scene, is necessary to create a `btTriangleMesh` coping all the vertices of the mesh and Bullet will create the shape for us.    
Usualy in real-life scenarios we create the collision from a simplified mesh, not the one with full resolution to not overload the physich engine.   

Physics engine handle only convex mesh dynamicly.   
So for dynamic obstacles with complex shape we create a convex hull of the mesh, fortunately Bullet provides us the `btConvexHullShape` object to do this from a set of vertices.   

## Minor graphical additions

### Skybox

### Texture

### Normal Mapping

### Light Model
The light model used from the application is the GGX model implemented during the lectures.   
The model is not been modified but for each object in the scene are used custom values for the parameters roughness, KD and the fresnel reflectance.   
For example for the car I decreased the roughness and incremented the fresnel reflectance in order to achive a more metallic look.   


# Performance
## Device 
## Performance related to particles

### Possible improvments
**TODO: check grammar**
Now each particle in the buffer is updated on the cpu and rendered separatly with istancing.
Since the particle don't interacth physicaly with the other object each element can be updated indipendently.
In addition to the lack of parallelization another drawback of updating the particle on the CPU is the memory transfert to the graphics card.
Is possible to use compute shader to create and update each particle on the GPU, without the need for the CPU to update and transfer the buffer at each frame. 
Despite this, as show, even with just the instancing technique, there are no frame drops up to --- (10.000 ???) particles.
Number that is more than enough to implement the effects used in the project. 

## Performance related to Meshes

**TODO: check grammar**
The main drawback of adding meshes to this project is that they should be rendered 3 times on each frame: One time for generic rendering, another time for the shadow map calculation and another time to calculate the depth over the snow section.
This is a list of meshes that I used for this project:

----

--- image of meshes used ---

Even with this mesh the performance is stable to --- fps

### Possible improvments
One improvment that can be made to each render pass is to use frustum culling.
Frustum culling allow us to render only the object that are inside the frustum of the camera.
This can be very beneficial especialy in the case of snow depth buffer: it is only a small rectangle on the plane and we only need to render a limited number of dynamic objects to know how much they sink into the snow.
In order to implement frustum culling you first need to implement a Bounding Volume hierarchy Data Structure to quickly determine if an object (or a group of objects) is in the frustum or not.
Both the implementation of Bounding Volume hierarchy and frustum culling is not trivial and due their complexity I consider it behind the scope of this project, and not necessary for the number of object in the scene in order to obtain a standard framerate.


# Conclusions
