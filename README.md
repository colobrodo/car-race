# Car race
This is my project for the courses of Real Time Graphics Programming at University of Milan.   
I developed a car based game, this is not inteded to be a playable game with a lot of mechanics but it's a sample application I used to implement some technique used in graphics programming.   
I think despite that you can still have fun trying it out :wink:   

Among the things I implemented there are:
- **Particle System**: A Particle System with istanced rendering.   
You can try it tweeking all the parameters for the emitter compiling the file `src/particle_playground`
- **Shadow Map**: Some shadows using [Shadow mapping technique](https://en.wikipedia.org/wiki/Shadow_mapping) and Poisson sampling PCF for softer effect
- **Sky box**: An image to simulate the environment using a cubemap
- A **Physichs-based car** that can interact with the environment 
- **Textures**
- **Normal Map**

## Compiling
I have developed and tested this project on windows but it should be compatible also with other platform.  

You can use the batch file   
```
.\MakeFileWin.bat
```   
To compile the main application.   
    
    
```
.\MakeParticle.bat
```   
To compile the particle playground.   



