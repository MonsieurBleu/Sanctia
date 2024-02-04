#pragma once

#include <GameObject.hpp>

class Entity;

enum EntityDeathType
{
    command,
    meleeAttack
};

struct EntityKillInfos
{
    Entity *killer = nullptr;
    EntityDeathType death = EntityDeathType::command;
};

class Entity : public GameObject
{
    protected : 
        std::u32string name;
        std::vector<Collider> hitcollider;

    public : 
        virtual void kill(EntityKillInfos&){};
        virtual void updateAgent(){GameObject::update();};
};
