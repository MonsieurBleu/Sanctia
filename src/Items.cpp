#include <SanctiaEntity.hpp>

void Items::equip(EntityRef usr, EntityRef item, EquipementSlots slot, int id)
{
    usr->comp<Items>().equipped[slot] = {id, item};

    ComponentModularity::addChild(*usr, item);

    if(item->hasComp<Effect>())
    {
        item->comp<Effect>().usr = usr.get();
    }
    if(item->hasComp<RigidBody>())
    {
        auto &b = item->comp<RigidBody>();

        b->setIsActive(false);

        int size = b->getNbColliders();

        for(int i = 0; i < size; i++)
        {
            auto c = b->getCollider(i);

            if(c->getCollisionCategoryBits() == 1<<CollideCategory::ENVIRONEMENT)
                c->setCollideWithMaskBits(0);
        }
    }
}

void Items::unequip(EntityRef usr, EquipementSlots slot)
{
    EntityRef item = usr->comp<Items>().equipped[slot].item;

    ComponentModularity::addChild(*usr, item);

    usr->comp<Items>().equipped[slot] = { };

    if(item->hasComp<Effect>())
    {
        item->comp<Effect>().usr = nullptr;
    }
    if(item->hasComp<RigidBody>())
    {
        auto &b = item->comp<RigidBody>();

        b->setIsActive(true);

        int size = b->getNbColliders();

        for(int i = 0; i < size; i++)
        {
            auto c = b->getCollider(i);

            if(c->getCollisionCategoryBits() == 1<<CollideCategory::ENVIRONEMENT)
                c->setCollideWithMaskBits(1<<CollideCategory::ENVIRONEMENT);
        }
    }
}