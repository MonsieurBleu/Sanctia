#pragma once

#include <MappedEnum.hpp>
#include <Graphics/ObjectGroup.hpp>
#include <array>

GENERATE_ENUM_FAST_REVERSE(DamageType, Pure, Blunt, Slash, Piercing, DamageType_Size);

GENERATE_ENUM_FAST_REVERSE(EffectType
    , UnknownType
    , Damage
    , Heal
)


struct statBar
{
    float min = 0.f;
    float max = 100.f;
    float cur = 100.f;

    statBar(){};
    statBar(float min, float max, float cur) : min(min), max(max), cur(cur){};
};

struct EntityStats
{
    bool alive = true;
    statBar health;
    statBar stamina;
    statBar adrenaline = statBar(-50, 50.f, 0.f);

    std::array<float, DamageType_Size> resistances = {0.f};

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


        EffectType type = EffectType::UnknownType;

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
