#pragma once

#include <BluePhysics.hpp>
#include <ObjectGroup.hpp>

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

        Effect()
        {
            zone.setCapsule(0.f, vec3(0), vec3(0));
        }

        bool enable = true;

        EffectType type;

        int maxTrigger = 1e9;
        int curTrigger = 0;
        B_Collider zone;
        float duration;

        float value;
        int valtype;

        ObjectGroupRef attachement;

        void apply(EntityStats &s);
};

struct EffectList
{
    Effect weapon;
};


