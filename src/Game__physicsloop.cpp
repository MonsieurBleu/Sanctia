#include <fstream>
#include <thread>

#include <Game.hpp>
#include <Globals.hpp>
#include <CompilingOptions.hpp>
#include <Constants.hpp>
#include <AssetManager.hpp>
#include <MathsUtils.hpp>

#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtx/string_cast.hpp>

void Game::physicsLoop()
{
    physicsTicks.freq = 100.f;
    physicsTicks.activate();

    while (state != quit)
    {
        if(globals.simulationTime.isPaused()) continue;

        physicsTicks.start();
        physicsTimer.start();

        physicsMutex.lock();
        
        // playerControl.body->boundingCollider.applyTranslation(playerControl.body->position, vec3(1, 0, 0));
        // GG::playerEntity->comp<EntityState3D>().position = playerControl.body->position;

        if(globals._currentController == &spectator)
        {
            playerControl.body->position = camera.getPosition() + vec3(0, 20, 0);
            playerControl.body->v = vec3(0);
        }

    /***** ATTACH ENTITY POSITION TO BODY POSITION *****/
        System<B_DynamicBodyRef, EntityState3D>([](Entity &entity)
        {
            auto &b = entity.comp<B_DynamicBodyRef>();
            auto &s = entity.comp<EntityState3D>();
            s.position = b->position;
            s.deplacementDirection = b->v;
            
            b->boundingCollider.applyTranslation(b->position, s.lookDirection);
        });

        if(!globals.simulationTime.isPaused())
            GG::physics.update(globals.simulationTime.speed / physicsTicks.freq);

    /***** APPLYING VELOCITY FROM DEPLACEMENT DIRECTION 
    *****/
        System<B_DynamicBodyRef, EntityState3D, DeplacementBehaviour>([](Entity &entity)
        {
            auto &s = entity.comp<EntityState3D>();
            auto &b = entity.comp<B_DynamicBodyRef>();

            auto &actionState = entity.comp<ActionState>();

            vec3 wdir = vec3(s.wantedDepDirection.x, 0, s.wantedDepDirection.z);

            /*
                TODO : debug
            */
            switch (actionState.lockDirection)
            {
                case ActionState::LockedDeplacement::DIRECTION :
                    b->v = actionState.lockedMaxSpeed * actionState.lockedDirection + vec3(0, b->v.y, 0);
                
                case ActionState::LockedDeplacement::SPEED_ONLY :
                    b->v = actionState.lockedMaxSpeed * wdir + vec3(0, b->v.y, 0);

                default: 
                    b->v = s.speed * wdir + vec3(0, b->v.y, 0);
                break;
            }
           
        });

    /***** ALL STUNED OR BLOCKING ENTITY ARE IMMOBILE *****/
        System<B_DynamicBodyRef, ActionState>([](Entity &entity){
            auto &s = entity.comp<ActionState>();

            if(s.stun || s.blocking)
                entity.comp<B_DynamicBodyRef>()->v *= vec3(0, 1, 0);
        });

    /***** ATTACH EFFECT TO ENTITY STATE *****/
        // System<Effect, EntityState3D>([](Entity &entity)
        // {
        //     auto &s = entity.comp<EntityState3D>();
        //     entity.comp<Effect>().zone.applyTranslation(s.position, s.direction);
        //     // entity.comp<Effect>().zone.applyTranslation(s.position, vec3(0));
        // });

    /***** CHECKING & APPLYING EFFECT TO ALL ENTITIES *****/
        System<Effect>([](Entity &entity)
        {
            Effect *e = &entity.comp<Effect>();
            
            if(!e->enable || e->curTrigger >= e->maxTrigger) entity.removeComp<Effect>();

            System<B_DynamicBodyRef, EntityStats, EntityState3D, Faction>([e](Entity &entity)
            {
                auto &f1 = e->usr ? e->usr->comp<Faction>() : entity.comp<Faction>();
                auto &f2 = entity.comp<Faction>();
                bool cancel = true;

                if(e->type == EffectType::Damage)
                    for(auto &i : Faction::canDamage)
                        if(i.first == f1.type && i.second == f2.type)
                        {
                            cancel = false;
                            break;
                        }
                
                if(cancel) return;

                if(e->curTrigger < e->maxTrigger)
                {
                    auto &b = entity.comp<B_DynamicBodyRef>();
                    auto &s = entity.comp<EntityState3D>();

                    CollisionInfo c = B_Collider::collide(b->boundingCollider, s.position, e->zone, vec3(0));
                    if(c.penetration > 1e-6)
                    {
                        for(auto i : e->affectedEntities)
                            if(i.e == &entity)
                                return;

                        e->affectedEntities.push_back({&entity, 0});
                    }
                }
            });

            for(auto &i : e->affectedEntities) 
            if(i.e && i.cnt < e->maxTriggerPerEntity)
                switch (e->type)
                {
                case EffectType::Damage :
                    {
                        bool blocked = false;
                        bool doStun = true;
                        float damageMult = 1.f;

                        auto &sd = i.e->comp<ActionState>();
                        auto &sd3D = i.e->comp<EntityState3D>();

                        ActionState *sa = e->usr ? &e->usr->comp<ActionState>() : &entity.comp<ActionState>();
                        EntityState3D *sa3D = e->usr ? &e->usr->comp<EntityState3D>() : &entity.comp<EntityState3D>();
                        
                        const int r = ActionState::Stance::RIGHT;
                        const int l = ActionState::Stance::LEFT;
                        const int special = ActionState::Stance::SPECIAL;

                        if(sd.blocking)
                        {
                            float a = dot(normalize(sa3D->position - sd3D.position), sd3D.lookDirection);
                            if(a > 0.5)
                            {

                                if((sd.stance() == r && sa->stance() == l) || (sd.stance() == l && sa->stance() == r))
                                {
                                    sd.hasBlockedAttack = true;
                                    blocked = true;
                                }
                                else if(sd.stance() == r || sd.stance() == l)
                                {
                                    damageMult = 0.5;
                                }
                            } 
                        }
                        else if(sd.attacking)
                        {
                            if(sd.stance() == special && sa->stance() != special)
                                doStun = false;
                        }
                        
                        if(!blocked)
                        {
                            e->apply(i.e->comp<EntityStats>(), damageMult);
                            sd.stun = doStun;
                        }
                        i.cnt ++;
                    }
                    break;
                
                default :
                    e->apply(i.e->comp<EntityStats>());
                    i.cnt ++;
                    break;
                }

            // if(e->curTrigger >= e->maxTrigger) entity.removeComp<Effect>();
            if(e->curTrigger >= e->maxTrigger) entity.comp<Effect>().enable = false;
        });

        System<AgentState>([&, this](Entity &entity){
            entity.comp<AgentState>().timeSinceLastState += globals.simulationTime.getDelta();
        });

    /***** COMBAT DEMO AGENTS *****/
        static int i = 0;
        if((i++)%10 == 0)
        System<EntityState3D, AgentState, ActionState>([&, this](Entity &entity){

            auto &s = entity.comp<EntityState3D>();
            auto target = entity.comp<Target>();
            auto &as = entity.comp<AgentState>();
            auto &action = entity.comp<ActionState>();

            if(action.stun)
                return;
            
            if(entity.hasComp<EntityStats>() && !entity.comp<EntityStats>().alive)
                return;

            switch (as.state)
            {
                case AgentState::COMBAT_POSITIONING : if(!target.get()) return;
                    {
                        auto &st = target->comp<EntityState3D>();   
                        float d = distance(s.position, st.position);

                        if(!target->comp<EntityStats>().alive)
                        {
                            s.speed = 0;
                            return;
                        }

                        float rangeMin = 1.5f;
                        float rangeMax = 2.f;

                        auto &taction = target->comp<ActionState>();   

                        s.lookDirection = normalize(st.position-s.position);

                        if(action.attacking || action.blocking)
                        {
                            s.speed = 0; return;
                        }

                        if(d > rangeMax || d < rangeMin)  
                        {
                            s.wantedDepDirection = normalize((st.position-s.position)*vec3(1, 0, 1)) * (d < rangeMin ? -1.f : 1.f);
                            s.speed = 1;
                        }
                        else
                        {
                            // s.wantedDepDirection = vec3(0);
                            // s.speed = 0;

                            float time = globals.simulationTime.getElapsedTime();
                            float angle = PI*2.f*random01Vec2(vec2(time - mod(time, 1.f) + 25.6984f*entity.ids[ENTITY_LIST]));
                            vec3 randDir(cos(angle), 0, sin(angle));

                            // s.wantedDepDirection = normalize(cross(st.position-s.position, vec3(0, 1, 0) + randDir));
                            s.wantedDepDirection = randDir;
                            s.speed = dot(s.wantedDepDirection, normalize((st.position-s.position)*vec3(1, 0, 1))) < 0.75 ? 0.5f : 0.f;
                        }

                        if(d <= rangeMax)
                        {
                            if(taction.attacking)
                                as.TransitionTo(AgentState::COMBAT_BLOCKING, 0.5, 1.0);
                            else 
                            if(as.timeSinceLastState > as.randomTime)
                                as.TransitionTo(AgentState::COMBAT_ATTACKING);
                        }
                    }
                    break;
                
                case AgentState::COMBAT_ATTACKING : if(!target.get()) return;
                    {
                        action.isTryingToAttack = true;
                        action.isTryingToBlock = false;

                        if(action.attacking) return;

                        auto &taction = target->comp<ActionState>();   

                        if(rand()%4 == 0)
                            action.setStance(ActionState::SPECIAL);
                        else if(taction.blocking)
                            action.setStance(taction.stance());
                        else
                            action.setStance(rand()%2 ? ActionState::LEFT : ActionState::RIGHT);

                        as.TransitionTo(AgentState::COMBAT_POSITIONING, 1.5, 3.0);
                    }
                    break;

                case AgentState::COMBAT_BLOCKING : if(!target.get()) return;
                    {
                        auto &st = target->comp<EntityState3D>();  
                        auto &taction = target->comp<ActionState>();   
                        s.lookDirection = normalize(st.position-s.position);

                        if(taction.stun)
                        {
                            as.TransitionTo(AgentState::COMBAT_ATTACKING);
                            return;
                        }

                        action.isTryingToAttack = false;
                        action.isTryingToBlock = true;

                        if(as.timeSinceLastState > as.randomTime)
                        {
                            as.TransitionTo(AgentState::COMBAT_POSITIONING, 1.5, 3.0);
                            action.isTryingToBlock = false;
                            return;
                        }   

                        if(taction.attacking)
                            switch (taction.stance())
                            {
                                case ActionState::LEFT    : action.setStance(ActionState::RIGHT); break;
                                case ActionState::RIGHT   : action.setStance(ActionState::LEFT); break;
                                case ActionState::SPECIAL : action.setStance(ActionState::SPECIAL); break;
                                default: break;
                            }
                    }
                    break;

                default:
                    break;
            }

        });

        // ManageGarbage<Effect>();
        ManageGarbage<B_DynamicBodyRef>();

        physicsMutex.unlock();

        physicsTimer.end();
        physicsTicks.waitForEnd();
    }
}
