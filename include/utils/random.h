#pragma once

#include <cmath>

inline float randf() {
    // returns a random float between 0 and 1
    return ((float)rand()) / (float) RAND_MAX;
}

float uniform_between(float low, float high) {
    // returns a random float between 0 and 1
    return randf() * (high - low) + low;
}
