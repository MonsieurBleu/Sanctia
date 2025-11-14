#pragma once

#include <memory>

class Entity;
typedef std::shared_ptr<Entity> EntityRef;

struct Target
{
    EntityRef target;
    bool hasTarget(){return (bool)target;};
    Entity &getTarget(){return *target;};
};

