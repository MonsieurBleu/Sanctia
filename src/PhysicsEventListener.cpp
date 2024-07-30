#include <PhysicsEventListener.hpp>
#include <GameConstants.hpp>

// #define DO_EVENT_LISTENER_DEBUG_PRINT

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
        case Effect::TargetType::ENEMY : if(!Faction::areEnemy(wFaction, tFaction)) return; break;
        case Effect::TargetType::ALLIES : if(Faction::areEnemy(wFaction, tFaction)) return; break;
        default: break;
    }

    if(triggerInfos) 
        triggerInfos->cnt ++;
    else
        effect.affectedEntities.push_back({target, 1});

    /* Compiling final informations needed for applying effect */
    auto &tAction = target->comp<ActionState>();
    auto &wAction = effect.usr->comp<ActionState>();

    auto &tState = target->comp<EntityState3D>();
    auto &wState = effect.usr->comp<EntityState3D>();

    float angle = dot(normalize(wState.position - tState.position), wState.lookDirection);

    /* Applying the effect */
    switch (effect.type)
    {
        case EffectType::Damage :
            {
                float damageMult = 1.f;
                bool stun = true;

                if(tAction.blocking && angle > SANCTIA_COMBAT_BLOCK_ANGLE)
                {
                    if(wAction.stance() != tAction.stance())
                    {
                        tAction.hasBlockedAttack = true;
                        damageMult = 0.f;
                    }
                    else
                        damageMult = 0.5f;
                }
                if(tAction.attacking && tAction.stance() == ActionState::SPECIAL && wAction.stance() != ActionState::SPECIAL)
                    stun = false;
                
                if(damageMult > 0.f)
                {
                    effect.apply(target->comp<EntityStats>(), damageMult);
                    tAction.stun = stun;
                }
            }
            break;
        
        default : effect.apply(target->comp<EntityStats>()); break;
    }


    #ifdef DO_EVENT_LISTENER_DEBUG_PRINT
        std::cout 
            << "hitzone between weapon " 
            << weapon->comp<EntityInfos>().name 
            << " on target " 
            << target->comp<EntityInfos>().name 
            << "\n";
        
        std::cout 
            << "factions are "
            << (int)wFaction.type
            << " and "
            << (int)tFaction.type;
        
        if(Faction::areEnemy(wFaction, tFaction))
            std::cout << ". They are enemy.\n";
        else 
            std::cout << ". They are not enemies.\n";
    #endif
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