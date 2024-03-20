#pragma once

#include <BluePhysics.hpp>

enum DamageType
{
    Pure, Blunt, Slash, Piercing, DamageType_Size
};

enum EffectType
{
    Damage
};

// enum Faction
// {

// }

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

class Effect
{
    public :

        EffectType type;

        int maxTrigger = 1e9;
        int curTrigger = 0;
        B_Collider zone;
        float duration;

        float value;
        int valtype;

        void apply(EntityStats &s);
};




