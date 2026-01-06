#pragma once

#include <PhysicsGlobals.hpp>

struct staticEntityFlag 
{
    bool isDYnamic = false;
    // Stencil used to fill up empty space, because you can't bind anything less than 32 bit as shader uniform
    bool stencil32bit[3] = {false, false, false}; 
};


