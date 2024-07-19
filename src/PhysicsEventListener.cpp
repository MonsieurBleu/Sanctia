#include <PhysicsEventListener.hpp>


void applyEffect(Entity *weapon, Entity *target)
{
    assert(weapon && target && weapon->hasComp<Effect>());

    Effect &effect = weapon->comp<Effect>();

    Effect::EntityTrigger *triggerInfos = nullptr;

    for(auto i : effect.affectedEntities)
        if(i.e == target)
        {
            if(i.cnt >= effect.maxTriggerPerEntity)
                return;
            triggerInfos = &i;
        }

    if(!effect.usr)
    {
        WARNING_MESSAGE("Empty Effect::usr on weapon '" 
            << weapon->comp<EntityInfos>().name << "' in collision with '" 
            << target->comp<EntityInfos>().name << "'");
        return;
    }

    const Faction wFaction = effect.usr->comp<Faction>();
    const Faction tFaction = target->comp<Faction>();

    switch (effect.target)
    {
        case Effect::TargetType::ENEMY : if(Faction::areEnemy(wFaction, tFaction)) return; break;
        case Effect::TargetType::ALLIES : if(Faction::areEnemy(wFaction, tFaction)) return; break;
        default: break;
    }

    if(triggerInfos) 
        triggerInfos->cnt ++;
    else
        effect.affectedEntities.push_back({target, 1});

    std::cout 
        << "hitzone between weapon " 
        << weapon->comp<EntityInfos>().name 
        << " on target " 
        << target->comp<EntityInfos>().name 
        << "\n";
}


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
        {
            bool e1IsValidWeapon = e1->hasComp<Effect>();
            bool e1IsValidTarget = e1->hasComp<EntityStats>();

            bool e2IsValidWeapon = e2->hasComp<Effect>();
            bool e2IsValidTarget = e2->hasComp<EntityStats>();
            
            if(e1IsValidWeapon && e2IsValidTarget)
                applyEffect(e1, e2);

            if(e2IsValidWeapon && e1IsValidTarget)
                applyEffect(e2, e1);        
        }
        
    }
}