#pragma once

#include <MappedEnum.hpp>
#include <Graphics/ObjectGroup.hpp>

GENERATE_ENUM_FAST_REVERSE(DamageType, Pure, Blunt, Slash, Piercing, DamageType_Size);

GENERATE_ENUM_FAST_REVERSE(EffectType
    , Unknown
    , Damage
    , Heal
)


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


        EffectType type = EffectType::Unknown;

        int maxTriggerPerEntity = 1;
        int maxTrigger = 1e9;
        int curTrigger = 0;
        float duration = 0.f;

        struct EntityTrigger { Entity *e = nullptr; int cnt = 0;};
        std::vector<EntityTrigger> affectedEntities;

        Entity *usr = nullptr;

        float value;
        int valtype = (int)DamageType::Pure;

        void apply(EntityStats &s, float mult = 1.f);

        void clear(){
            affectedEntities.clear(); };
};
