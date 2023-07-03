#pragma once

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