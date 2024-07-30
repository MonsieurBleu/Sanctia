#pragma once

#include <ObjectGroup.hpp>

enum DamageType
{
    Pure, Blunt, Slash, Piercing, DamageType_Size
};

enum EffectType
{
    Damage
};


struct statBar
{
    float min = 0.f;
    float max = 100.f;
    float cur = 100.f;
};

struct EntityStats
{
    bool alive = true;
    statBar health;
    statBar stamina;

    float resistances[DamageType_Size] = {0.f};

    void damage(float val, DamageType type);
};






class Entity;

class Effect
{
    public :

        Effect()
        {}

        ~Effect()
        {}

        enum class TargetType : uint8
        {
            ALL, ENEMY, ALLIES
        } target = TargetType::ENEMY;


        EffectType type;

        int maxTriggerPerEntity = 1;
        int maxTrigger = 1e9;
        int curTrigger = 0;
        float duration = 0.f;

        struct EntityTrigger { Entity *e = nullptr; int cnt = 0;};
        std::vector<EntityTrigger> affectedEntities;

        Entity *usr = nullptr;

        float value;
        int valtype = (int)DamageType::Pure;

        /* TODO : remove ? */
        ObjectGroupRef attachement;

        void apply(EntityStats &s, float mult = 1.f);

        void clear(){
            affectedEntities.clear(); };
};
