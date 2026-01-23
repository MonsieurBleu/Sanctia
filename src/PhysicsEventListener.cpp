#include <PhysicsEventListener.hpp>
#include <GameConstants.hpp>

// #define DO_EVENT_LISTENER_DEBUG_PRINT

void applyEffect(Entity *weapon, Entity *target)
{
    assert(weapon && target && weapon->has<Effect>());

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
        #ifdef DO_EVENT_LISTENER_DEBUG_PRINT
        WARNING_MESSAGE("Empty Effect::usr on weapon '" 
            << weapon->comp<EntityInfos>().name << "' in collision with '" 
            << target->comp<EntityInfos>().name << "'");
        #endif
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

    auto &tState = target->comp<state3D>();
    auto &wState = effect.usr->comp<state3D>();

    float angle = dot(normalize(wState.position - tState.position), wState.lookDirection);

    #ifdef DO_EVENT_LISTENER_DEBUG_PRINT
        std::cout << TERMINAL_INFO << TERMINAL_UNDERLINE
            << "Effect hitzone of type " 
            << TERMINAL_FILENAME << EffectTypeReverseMap[effect.type] << TERMINAL_INFO
            << " between object " << weapon->comp<EntityInfos>().name 
            << " on target " << target->comp<EntityInfos>().name << "\n"
            << TERMINAL_RESET;
        
        std::cout << TERMINAL_INFO
            << "\tFactions are "
            << TERMINAL_FILENAME << Faction::TypeReverseMap[wFaction.type] << TERMINAL_INFO
            << " and "
            << TERMINAL_FILENAME << Faction::TypeReverseMap[tFaction.type] << TERMINAL_INFO;
        
        if(Faction::areEnemy(wFaction, tFaction))
            std::cout << ". They are enemy.\n" << TERMINAL_RESET;
        else 
            std::cout << ". They are not enemies.\n" << TERMINAL_RESET;
    #endif

    /* Applying the effect */
    switch (effect.type)
    {
        case EffectType::Damage :
            {
                float damageMult = 1.f;
                bool stun = true;

                // std::cout << tAction.blocking << "\t" << angle << "\t" << SANCTIA_COMBAT_BLOCK_ANGLE << "\n";

                if(tAction.blocking && angle < SANCTIA_COMBAT_BLOCK_ANGLE)
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
                
                #ifdef DO_EVENT_LISTENER_DEBUG_PRINT
                    std::cout << TERMINAL_INFO;
                    std::cout << "\tCalculated damageMult : " << damageMult << "\n";
                    std::cout << "\tCalculated stun : " << stun << "\n" << TERMINAL_RESET;
                #endif

                if(damageMult > 0.f)
                {
                    effect.apply(target->comp<EntityStats>(), damageMult);
                    tAction.stun = stun;
                }
            }
            break;
        
        default : effect.apply(target->comp<EntityStats>()); break;
    }
}

void PhysicsEventListener::onContact(const rp3d::CollisionCallback::CallbackData& callbackData) 
{
    uint32 nb = callbackData.getNbContactPairs();

    for(uint32 i = 0; i < nb; i++) 
    {
        _ContactPair pair = callbackData.getContactPair(i);

        rp3d::Collider *c1 = pair.getCollider1();
        rp3d::Collider *c2 = pair.getCollider2();

        
        Entity *e1 = (Entity*)c1->getUserData();
        Entity *e2 = (Entity*)c2->getUserData();

        if(!e1 || !e2)
        {
            #ifdef DO_EVENT_LISTENER_DEBUG_PRINT
            WARNING_MESSAGE("Physic collide event with null user data will be ignored :(");
            #endif
            continue;
        }

        // bool isExit = pair.getEventType() == _ContactPair::EventType::ContactExit;
        bool isStart = pair.getEventType() == _ContactPair::EventType::ContactStart;
        bool isStay = pair.getEventType() == _ContactPair::EventType::ContactStay;

        bool e1Deplacement = e1->has<DeplacementState>();
        bool e2Deplacement = e2->has<DeplacementState>();
        if (
               ((e1Deplacement || e2Deplacement) && !(e1Deplacement && e2Deplacement))  // e1 or e2 are dynamic but not both
        && (isStart || isStay)                                                          // we are entering or staying in collision
        )
        {
            Entity* dynamicEntity = e1Deplacement ? e1 : e2;
            DeplacementState& ds = dynamicEntity->comp<DeplacementState>();
            
            // check if ground is flat enough to walk
            const float Y_THRESHOLD = 0.80; // threshold for grounded
            uint32 nbContacts = pair.getNbContactPoints();
            // std::cout << "nbContacts: " << nbContacts << std::endl;
            float maxY = -1.0f;
            bool found_grounded = false;
            for (int j = 0; j < nbContacts; j++)
            {
                rp3d::CollisionCallback::ContactPoint contactPoint = pair.getContactPoint(j);
                rp3d::Vector3 normal = contactPoint.getWorldNormal();

                // the normal direction depends on which entity is the dynamic one so we may need to invert it 
                float n = e1Deplacement ? -normal.y : normal.y; 

                // std::cout << "[" << std::fixed << std::setprecision(4) << globals.simulationTime.getElapsedTime() << "] Debug :3 normal.y is : " << n << std::endl;
                maxY = max(n, maxY);
                if (n > Y_THRESHOLD) 
                {
                    ds._grounded = true;
                    found_grounded = true;
                    // break;
                }
            }
            if (found_grounded)
                ds._groundNormalY = maxY;
        }
    }
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

        if(!e1 || !e2)
        {
            #ifdef DO_EVENT_LISTENER_DEBUG_PRINT
            WARNING_MESSAGE("Physic trigger event with null user data will be ignored :(");
            #endif
            continue;
        }

        bool isExit = pair.getEventType() == _OverlapPair_::EventType::OverlapExit;
        bool isEnter = pair.getEventType() == _OverlapPair_::EventType::OverlapStart;
        // bool isStay = pair.getEventType() == _OverlapPair_::EventType::OverlapStay;

        // if(pair.getEventType() != _OverlapPair_::EventType::OverlapStart)
        {
            // bool isWearer = false; // True if e2 is equipped with e1 as an Item
            // if(e2->has<Items>())
            //     for(auto &i : e2->comp<Items>().equipped)
            //         if(i.item.get() == e1)
            //         {
            //             isWearer = true;
            //             continue;
            //         }
            // bool hasWearer = isWearer;

            Entity *wearer = e1;
            Entity *parent = e1->comp<EntityGroupInfo>().parent;

            if(parent && parent->has<Items>())
                for(auto &i : parent->comp<Items>().equipped)
                    if(i.item.get() == e1)
                    {
                        wearer = parent;
                        break;
                    }

            // TODO: maybe see if we should move this to the collide event also 
            // (since it's called onCollision and all that and that for now it will only apply to trigger colliders)
            // (maybe separate onCollide And onTrigger ? that's what they do in unity)
            if(e1->has<Script>())
            {
                if(isExit)
                    e1->comp<Script>().run_OnCollisionExit(*e1, *e2, *wearer);
                else if(isEnter)
                    e1->comp<Script>().run_OnCollisionEnter(*e1, *e2, *wearer);
            }
        }

        // if(pair.getEventType() == _OverlapPair_::EventType::OverlapStart)
        // {
        //     bool e1IsValidWeapon = e1->has<Effect>();
        //     bool e1IsValidTarget = e1->has<EntityStats>();

        //     bool e2IsValidWeapon = e2->has<Effect>();
        //     bool e2IsValidTarget = e2->has<EntityStats>();
            
        //     if(e1IsValidWeapon && e2IsValidTarget)
        //         applyEffect(e1, e2);

        //     if(e2IsValidWeapon && e1IsValidTarget)
        //         applyEffect(e2, e1);        
        // }
        
    }
}