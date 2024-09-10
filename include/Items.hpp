#pragma once

class Entity;

#include <EntityStats.hpp>

class Entity;
typedef std::shared_ptr<Entity> EntityRef;

struct ItemInfos
{
    int price = 1.f;
    float damageMultiplier = 20.f;
    int dmgType = DamageType::Pure;
};

GENERATE_ENUM_FAST_REVERSE(EquipementSlots
    , WEAPON_SLOT
    , LEFT_FOOT_SLOT
    , UNNAMED_SLOT_3
    , UNNAMED_SLOT_4
    , UNNAMED_SLOT_5
    , UNNAMED_SLOT_6
    , UNNAMED_SLOT_7
    , UNNAMED_SLOT_8
    , UNNAMED_SLOT_9
    , UNNAMED_SLOT_10
    , UNNAMED_SLOT_11
    , UNNAMED_SLOT_12
    , UNNAMED_SLOT_13
    , UNNAMED_SLOT_14
    , UNNAMED_SLOT_15
    , UNNAMED_SLOT_16
)

enum BipedSkeletonID : int
{
    RIGHT_HAND = 24,
    LEFT_FOOT = 7
};

struct Items
{
    struct Equipement{int id = 0; EntityRef item;} equipped[16];

    static void equip(EntityRef usr, EntityRef item, EquipementSlots slot, int id);
    static void unequip(EntityRef usr, EquipementSlots slot);
};

struct ItemTransform{mat4 mat;};