#include <PhysicsEventListener.hpp>

void PhysicsEventListener::onTrigger(const rp3d::OverlapCallback::CallbackData& callbackData)
{
    uint32 nb = callbackData.getNbOverlappingPairs();

    for(uint32 i = 0; i < nb; i++)
    {
        _OverlapPair_ pair = callbackData.getOverlappingPair(i);

        rp3d::Collider *c1 = pair.getCollider1();
        rp3d::Collider *c2 = pair.getCollider2();

        
        Entity *e1 = (Entity*)c1->getUserData();
        Entity *e2 = (Entity*)c2->getUserData();

    if(pair.getEventType() == _OverlapPair_::EventType::OverlapStart)
        std::cout 
            << "hitzone between " 
            << e1->comp<EntityInfos>().name 
            << " and " 
            << e2->comp<EntityInfos>().name 
            << "\n";

    }
}