#pragma once

#include <PhysicsGlobals.hpp>

struct staticEntityFlag 
{
    bool isDYnamic = false;
    // Stencil used to fill up empty space, because you can't bind anything less than 32 bit as shader uniform
    bool stencil32bit[3] = {false, false, false}; 
    bool shoudBeActive = true;
    bool isActive = true;
    int activeIterationCnt = 0;
};



// dummy component to apply system only to height fields
struct HeightFieldDummyFlag {
};