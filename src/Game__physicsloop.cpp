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

#include <PhysicsGlobals.hpp>

#include <PhysicsEventListener.hpp>


void Game::physicsLoop()
{
    physicsTicks.freq = 100.f;
    physicsTicks.activate();

    PhysicsEventListener eventListener;
    PG::world->setEventListener((rp3d::EventListener*)&eventListener);

    while (state != quit)
    {   
        if(globals.simulationTime.isPaused())
        {
            PG::currentPhysicFreq = 0.f;
            continue;
        }
        else
            PG::currentPhysicFreq = physicsTicks.freq;

        physicsTicks.start();
        physicsTimer.start();
        physicsMutex.lock();

        if(globals._currentController == &spectator && GG::playerEntity->hasComp<rp3d::RigidBody*>())
        {
            auto body = GG::playerEntity->comp<rp3d::RigidBody*>();
            if(body)
                body->setIsActive(false);
        }

        PG::world->update(globals.simulationTime.speed / physicsTicks.freq);

    /***** UPDATING RIGID BODY AND ENTITY STATE RELATIVE TO THE BODY TYPE *****/
        System<rp3d::RigidBody*, EntityState3D>([](Entity &entity){
            auto &b = entity.comp<rp3d::RigidBody*>();
            auto &s = entity.comp<EntityState3D>();
            auto &ds = entity.comp<EntityDeplacementState>();

            #ifdef SANCTIA_DEBUG_PHYSIC_HELPER
            s.physicActivated = b->isActive();
            #endif

            switch (b->getType())
            {
            /* Entities with a dynamic body follows the physic. They can also exert a deplacement force*/
            case rp3d::BodyType::DYNAMIC : 
                {
                    auto &t = b->getTransform();
                    s.position = PG::toglm(t.getPosition());
                    s.quaternion = PG::toglm(t.getOrientation());
                    
                    if(!entity.hasComp<EntityDeplacementState>()) break;

                    /*
                        Fastest way possible to see if the player is grounded.
                            If the player comes into contact with a surface exerting an opposing force to 
                            gravity, their vertical velocity will be canceled out.

                            This method can theoricly leads to a rare double jump exploit.
                            Luna said this methode render bunny hopping impossible.
                    */
                    ds.grounded = abs(b->getLinearVelocity().y) < 5e-4;
                    ds.deplacementDirection = normalize(PG::toglm(b->getLinearVelocity()));

                    float maxspeed = ds.grounded ? ds.wantedSpeed : ds.airSpeed; 
                    auto vel = b->getLinearVelocity();
                    vec3 dir = ds.wantedDepDirection;
                    ds.speed = length(vec2(vel.x, vel.z));

                    if(entity.hasComp<ActionState>())
                    {
                        auto &as = entity.comp<ActionState>();

                        if(as.stun)
                            maxspeed = 0.f;
                        else switch (as.lockType)
                            {
                                case ActionState::DIRECTION : dir = as.lockedDirection;
                                case ActionState::SPEED_ONLY :maxspeed = as.lockedMaxSpeed; break;                
                                default: break;
                            }
                    }

                    if(ds.speed < maxspeed)
                        b->applyWorldForceAtCenterOfMass(PG::torp3d(dir * pow(maxspeed, 0.4f) * 15.f));            
                }
                break;
            
            /* Entities with a kinematic body are stucked :(. The body follows the entity state.
               The game assumes that another system is using the entity and dealing with his deplacement.*/
            case rp3d::BodyType::KINEMATIC :
                {
                    /* TODO : maybe fix one day for entities who are not using quaternion*/
                    rp3d::Transform t(PG::torp3d(s.position), s.usequat ? PG::torp3d(s.quaternion) : rp3d::Quaternion::identity());
                    b->setTransform(t);

                    // std::cout << entity.comp<EntityInfos>().name << " " << to_string(s.position) << "\n";
                }
                break;
            default: break;
            }

        });

    /***** CHECKING & APPLYING EFFECT TO ALL ENTITIES *****/
        // System<Effect>([](Entity &entity)
        // {
        //     Effect *e = &entity.comp<Effect>();
            
        //     if(!e->enable || e->curTrigger >= e->maxTrigger) entity.removeComp<Effect>();

        //     System<B_DynamicBodyRef, EntityStats, EntityState3D, Faction>([e](Entity &entity)
        //     {
        //         auto &f1 = e->usr ? e->usr->comp<Faction>() : entity.comp<Faction>();
        //         auto &f2 = entity.comp<Faction>();
        //         bool cancel = true;

        //         if(e->type == EffectType::Damage)
        //             for(auto &i : Faction::canDamage)
        //                 if(i.first == f1.type && i.second == f2.type)
        //                 {
        //                     cancel = false;
        //                     break;
        //                 }
                
        //         if(cancel) return;

        //         if(e->curTrigger < e->maxTrigger)
        //         {
        //             auto &b = entity.comp<B_DynamicBodyRef>();
        //             auto &s = entity.comp<EntityState3D>();

        //             CollisionInfo c = B_Collider::collide(b->boundingCollider, s.position, e->zone, vec3(0));
        //             if(c.penetration > 1e-6)
        //             {
        //                 for(auto i : e->affectedEntities)
        //                     if(i.e == &entity)
        //                         return;

        //                 e->affectedEntities.push_back({&entity, 0});
        //             }
        //         }
        //     });

        //     for(auto &i : e->affectedEntities) 
        //     if(i.e && i.cnt < e->maxTriggerPerEntity)
        //         switch (e->type)
        //         {
        //         case EffectType::Damage :
        //             {
        //                 bool blocked = false;
        //                 bool doStun = true;
        //                 float damageMult = 1.f;

        //                 auto &sd = i.e->comp<ActionState>();
        //                 auto &sd3D = i.e->comp<EntityState3D>();

        //                 ActionState *sa = e->usr ? &e->usr->comp<ActionState>() : &entity.comp<ActionState>();
        //                 EntityState3D *sa3D = e->usr ? &e->usr->comp<EntityState3D>() : &entity.comp<EntityState3D>();
                        
        //                 const int r = ActionState::Stance::RIGHT;
        //                 const int l = ActionState::Stance::LEFT;
        //                 const int special = ActionState::Stance::SPECIAL;

        //                 if(sd.blocking)
        //                 {
        //                     float a = dot(normalize(sa3D->position - sd3D.position), sd3D.lookDirection);
        //                     if(a > 0.5)
        //                     {

        //                         if((sd.stance() == r && sa->stance() == l) || (sd.stance() == l && sa->stance() == r))
        //                         {
        //                             sd.hasBlockedAttack = true;
        //                             blocked = true;
        //                         }
        //                         else if(sd.stance() == r || sd.stance() == l)
        //                         {
        //                             damageMult = 0.5;
        //                         }
        //                     } 
        //                 }
        //                 else if(sd.attacking)
        //                 {
        //                     if(sd.stance() == special && sa->stance() != special)
        //                         doStun = false;
        //                 }
                        
        //                 if(!blocked)
        //                 {
        //                     e->apply(i.e->comp<EntityStats>(), damageMult);
        //                     sd.stun = doStun;
        //                 }
        //                 i.cnt ++;
        //             }
        //             break;
                
        //         default :
        //             e->apply(i.e->comp<EntityStats>());
        //             i.cnt ++;
        //             break;
        //         }

        //     // if(e->curTrigger >= e->maxTrigger) entity.removeComp<Effect>();
        //     if(e->curTrigger >= e->maxTrigger) entity.comp<Effect>().enable = false;
        // });

        System<AgentState>([&, this](Entity &entity){
            entity.comp<AgentState>().timeSinceLastState += globals.simulationTime.getDelta();
        });

    /***** COMBAT DEMO AGENTS *****/
        static int i = 0;
        if((i++)%10 == 0)
        System<EntityState3D, EntityDeplacementState, AgentState, ActionState>([&, this](Entity &entity){

            auto &s = entity.comp<EntityState3D>();
            auto &ds = entity.comp<EntityDeplacementState>();
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
                            ds.wantedSpeed = 0;
                            return;
                        }

                        float rangeMin = 1.5f;
                        float rangeMax = 2.f;

                        auto &taction = target->comp<ActionState>();   

                        s.lookDirection = normalize(st.position-s.position);

                        if(action.attacking || action.blocking)
                        {
                            ds.wantedSpeed = 0; return;
                        }

                        if(d > rangeMax || d < rangeMin)  
                        {
                            ds.wantedDepDirection = normalize((st.position-s.position)*vec3(1, 0, 1)) * (d < rangeMin ? -1.f : 1.f);
                            ds.wantedSpeed = 1.5;
                        }
                        else
                        {
                            // s.wantedDepDirection = vec3(0);
                            // s.speed = 0;

                            float time = globals.simulationTime.getElapsedTime();
                            float angle = PI*2.f*random01Vec2(vec2(time - mod(time, 1.f) + 25.6984f*entity.ids[ENTITY_LIST]));
                            vec3 randDir(cos(angle), 0, sin(angle));

                            // s.wantedDepDirection = normalize(cross(st.position-s.position, vec3(0, 1, 0) + randDir));
                            ds.wantedDepDirection = randDir;
                            ds.wantedSpeed = dot(ds.wantedDepDirection, normalize((st.position-s.position)*vec3(1, 0, 1))) < 0.75 ? 0.5f : 0.f;
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

        physicsTimer.end();

        float maxFreq = 200.f;
        float minFreq = 25.f;
        if(physicsTimer.getDelta() > 1.f/physicsTicks.freq)
        {
            physicsTicks.freq = clamp(physicsTicks.freq/2.f, minFreq, maxFreq);
        }
        else if(physicsTimer.getDelta() < 0.4f/physicsTicks.freq)
        {
            physicsTicks.freq = clamp(physicsTicks.freq*2.f, minFreq, maxFreq);
        }

        ManageGarbage<rp3d::RigidBody *>();

        physicsMutex.unlock();

        physicsTicks.waitForEnd();
    }
}
