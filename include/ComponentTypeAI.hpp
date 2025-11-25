#pragma once

#include <memory>
#include <MappedEnum.hpp>

#ifndef GLM_VERSION_MAJOR 
#include <glm/glm.hpp>
#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtx/euler_angles.hpp>
#endif

using namespace glm;

#include <Globals.hpp>

class Entity;
typedef std::shared_ptr<Entity> EntityRef;

struct Target
{
    Entity *target;
    bool hasTarget(){return (bool)target;};
    Entity &getTarget(){return *target;};
    void setTarget(int i);
};

struct AgentState__old
{
    GENERATE_ENUM_FAST_REVERSE(State
        , COMBAT_POSITIONING
        , COMBAT_ATTACKING
        , COMBAT_BLOCKING
    ) 
    
    State state;

    float timeSinceLastState = 0.f;
    float randomTime = 0.f;

    void TransitionTo(State newState, float minTime = 0.f, float maxTime = 0.f)
    {
        state = newState; 
        timeSinceLastState = 0.f;
        float r = (float)(std::rand()%1024)/1024.f;
        randomTime = minTime + r*(maxTime-minTime);
    }
};

struct AgentState 
{
    uint state = 0;
    std::string stateName;

    float lastUpdateTime = 0.f;
    float nextUpdateDelay = 0.1f;
    float lastStateChangeTime = 0.f;

    void Transition(int newState, const std::string& newStateName, float delay)
    {
        lastUpdateTime = globals.simulationTime.getElapsedTime();
        if(newState != state) lastStateChangeTime = lastUpdateTime;
        state = newState;
        stateName = newStateName;
        nextUpdateDelay = delay;
    }
};
