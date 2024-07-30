#pragma once

class Entity;

#include <EntityStats.hpp>

class Entity;
typedef std::shared_ptr<Entity> EntityRef;

struct ItemInfos
{
    int price = 1.f;
    float dmgMult = 20.f;
    int dmgType = DamageType::Pure;
};

enum EquipementSlots : uint8
{
    WEAPON_SLOT,
    LEFT_FOOT_SLOT
};

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

struct ItemTransform{mat4 t;};