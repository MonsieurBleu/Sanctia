#include <SanctiaEntity.hpp>
#include <MathsUtils.hpp>

#include <Subapps.hpp>

void Items::equip(EntityRef usr, EntityRef item, EquipementSlots slot, int id)
{
    usr->comp<Items>().equipped[slot] = {id, item};

    ComponentModularity::addChild(*usr, item);

    if(item->has<Effect>())
    {
        item->comp<Effect>().usr = usr.get();
    }
    if(item->has<RigidBody>() and item->has<staticEntityFlag>())
    {
        auto &b = item->comp<RigidBody>();

        b->setIsActive(false);
        item->comp<staticEntityFlag>().shoudBeActive = false;
        b->setType(rp3d::BodyType::KINEMATIC);
        
        int size = b->getNbColliders();
        
        for(int i = 0; i < size; i++)
        {
            auto c = b->getCollider(i);
            
            if(c->getCollisionCategoryBits() == 1<<CollideCategory::ENVIRONEMENT)
                c->setCollideWithMaskBits(0);
        }
        // b->setIsActive(true);
    }
}

void Items::unequip(Entity &usr, EquipementSlots slot)
{
    EntityRef item = usr.comp<Items>().equipped[slot].item;

    if(!item) return;

    auto is = item->comp<state3D>();
    // ComponentModularity::addChild(usr, item);

    is.initPosition = is.position;
    is.initQuat = is.quaternion;

    
    

    if(item->has<Effect>())
    {
        item->comp<Effect>().usr = nullptr;
    }
    if(item->has<RigidBody>() and item->has<staticEntityFlag>())
    {
        physicsMutex.lock();

        auto &b = item->comp<RigidBody>();
        
        ComponentModularity::addChild(*SubApps::getCurrentRoot(), item);
        b->setTransform(rp3d::Transform(PG::torp3d(is.position), PG::torp3d(is.usequat ? is.quaternion : directionToQuat(is.lookDirection))));
 
        // auto tmp = b->getTransform();
        // std::cout << item->toStr() << "\n";
        // NOTIF_MESSAGE(is.position << "\t" << is.quaternion)
        // NOTIF_MESSAGE(PG::toglm(tmp.getPosition()) << "\t" << PG::toglm(tmp.getOrientation()))

        b->setType(rp3d::BodyType::DYNAMIC);


        int size = b->getNbColliders();

        for(int i = 0; i < size; i++)
        {
            auto c = b->getCollider(i);

            if(c->getCollisionCategoryBits() == 1<<CollideCategory::ENVIRONEMENT)
                c->setCollideWithMaskBits(1<<CollideCategory::ENVIRONEMENT);
        }

        b->setIsActive(true);
        item->comp<staticEntityFlag>().shoudBeActive = true;

        physicsMutex.unlock();
    }

    

    usr.comp<Items>().equipped[slot] = { };
}
