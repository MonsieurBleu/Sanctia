#include <SanctiaEntity.hpp>

void Items::equip(EntityRef usr, EntityRef item, EquipementSlots slot, int id)
{
    usr->comp<Items>().equipped[slot] = {id, item};

    if(item->hasComp<Effect>())
    {
        item->comp<Effect>().usr = usr.get();
    }
    if(item->hasComp<RigidBody>())
    {
        item->comp<RigidBody>()->setIsActive(false);
    }
}

void Items::unequip(EntityRef usr, EquipementSlots slot)
{
    EntityRef item = usr->comp<Items>().equipped[slot].item;

    usr->comp<Items>().equipped[slot] = { };

    if(item->hasComp<Effect>())
    {
        item->comp<Effect>().usr = nullptr;
    }
    if(item->hasComp<RigidBody>())
    {
        item->comp<RigidBody>()->setIsActive(true);
    }
}