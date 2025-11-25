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

#include <Scripting/ScriptInstance.hpp>
#include <SanctiaLuaBindings.hpp>

void Game::physicsLoop()
{
    physicsTicks.freq = 100.f;
    physicsTicks.activate();

    threadStateName = "Physics Thread";
    threadState.open_libraries(
        sol::lib::base, 
        sol::lib::coroutine, 
        sol::lib::string, 
        sol::lib::io,
        sol::lib::math,
        sol::lib::jit
    );
    SanctiaLuaBindings::bindAll(threadState);

    PhysicsEventListener eventListener;
    PG::world->setEventListener((rp3d::EventListener*)&eventListener);

    while (state != quit)
    {   
        if(globals.simulationTime.isPaused() && globals.enablePhysics)
        {
            PG::currentPhysicFreq = 0.f;
            continue;
        }
        else
            PG::currentPhysicFreq = physicsTicks.freq;

        physicsTicks.start();
        physicsTimer.start();
        physicsMutex.lock();

        ManageGarbage<RigidBody>();

        if(GG::playerEntity && globals._currentController == &spectator && GG::playerEntity->hasComp<RigidBody>())
        {
            auto body = GG::playerEntity->comp<RigidBody>();
            if(body)
                body->setIsActive(false);
        }
        
        /*
            In some very specific conditions, invalid transforms can occurs. Those generally last for the time of a frame or even less.
            These conditions includes : 
                - Randommly the equiped items when respawning the player, even if the componment garbage is cleaned and the physic mutex locked
                - ...
            This simple system just check for a specific kind of bad transforms to prevent crashes.
            In the worst case scenerio, an object that get a bad transform will just be teleported in the center
            of the world, underneath the terrain. But normally those transforms are fixed the frame after that.

            Note ; isnan isn't working, the glm one AND the rp3d version. But the same check can be make by checking low infinity bound.
            This is probablly caused by the fastmath compiler argument.
        */
        System<RigidBody, EntityState3D, staticEntityFlag>([](Entity &entity){
            auto &b = entity.comp<RigidBody>();
            vec3 p = PG::toglm(b->getTransform().getPosition());
            const float lowinf = 1e6;
            if(p.x > lowinf || p.y > lowinf || p.z > lowinf || p.x < -lowinf || p.y < -lowinf || p.z < -lowinf)
                b->setTransform(rp3d::Transform::identity());
        });

        physicsWorldUpdateTimer.start();
        PG::world->update(globals.simulationTime.speed / physicsTicks.freq);
        physicsWorldUpdateTimer.stop();

        physicsSystemsTimer.start();

        PG::physicInterpolationMutex.lock();
        PG::physicInterpolationTick.tick();
        PG::physicInterpolationMutex.unlock();

    /***** UPDATING RIGID BODY AND ENTITY STATE RELATIVE TO THE BODY TYPE *****/
        System<RigidBody, EntityState3D, staticEntityFlag>([](Entity &entity){
            auto &b = entity.comp<RigidBody>();
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

                    if(!PG::doPhysicInterpolation || s._PhysicTmpPos.x == UNINITIALIZED_FLOAT)
                    {
                        s._PhysicTmpPos = PG::toglm(t.getPosition());
                        s._PhysicTmpQuat = PG::toglm(t.getOrientation());
                    }   
                    else
                    {
                        s._PhysicTmpPos  = s.position;
                        s._PhysicTmpQuat = s.quaternion;
                    }

                    s.position = PG::toglm(t.getPosition());
                    s.quaternion = PG::toglm(t.getOrientation());
                    
                    if(!entity.hasComp<EntityDeplacementState>()) break;

                    const float small_threshold = 0.05f;

                    vec3 velocity = PG::toglm(b->getLinearVelocity());

                    float dt = globals.simulationTime.getDelta();

                    ds.deplacementDirection = normalize(velocity);
                    ds.speed = length(velocity);
                    

                    // check grounded
                    PG::GroundedRayCastCallback callback;
                    float radius = 0.25f;
                    // here we try multiple raycasts in a grid to better detect ground when on slopes and stuff but idk if it really is necessary :/
                    for (int i = -1; i <= 1; i++)
                    {
                        for (int j = -1; j <= 1; j++)
                        {
                            vec3 offset = vec3(i * radius * 1.5, 0, j * radius * 1.5);
                            vec3 from_glm = s.position + offset + vec3(0, 1.0f, 0); // cursed magic value offset, should probably do something about that
                            rp3d::Vector3 from = PG::torp3d(from_glm);
                            rp3d::Vector3 to = PG::torp3d(s.position + offset + vec3(0, -0.15f, 0));
                            PG::GroundedRayCastCallback callback_offset(from_glm);
                            rp3d::Ray ray(from, to);
                            PG::world->raycast(ray, &callback_offset);
                            if (callback_offset.hit)
                            {
                                callback = callback_offset;
                                break;
                            }
                        }
                    }
                    if (!callback.hit)
                    {
                        ds.walking = false;
                        ds.grounded = false;
                    }
                    else if (dot(velocity, vec3(0, 1, 0)) > 0 && dot(velocity, callback.hitNormal) > small_threshold)
                    {
                        ds.walking = false;
                        ds.grounded = false;
                    }
                    else 
                    {
                        if (ds.grounded == false)
                        {
                            // we just landed
                            ds.landedTime = globals.simulationTime.getElapsedTime();
                        }
                        ds.grounded = true;
                        ds.walking = true;
                    }

                    // apply gravity
                    vec3 v = velocity + vec3(0, -ds.gravity, 0) * dt;

                    velocity = (velocity + v) * 0.5f; // smoothing the gravity effect, feels nicer imo


                    // get input from ActionState if any
                    if(entity.hasComp<ActionState>())
                    {
                        auto &as = entity.comp<ActionState>();

                        // if(as.stun)
                        //     maxspeed = 0.f;
                        // else switch (as.lockType)
                        //     {
                        //         case ActionState::DIRECTION : dir = as.lockedDirection;
                        //         case ActionState::SPEED_ONLY :maxspeed = as.lockedMaxSpeed; break;                
                        //         default: break;
                        //     }

                        if (as.stun)
                        {
                            ds.wantedSpeed = 0.f;
                            ds.isJumping = false;
                        }
                        else switch (as.lockType)
                        {
                            case ActionState::DIRECTION :
                                ds.wantedDepDirection = as.lockedDirection;
                                ds.isJumping = false;
                                break;
                            case ActionState::SPEED_ONLY :
                                ds.wantedSpeed = as.lockedMaxSpeed; 
                                ds.isJumping = false;
                                break;                
                            default: break;
                        }
                    }

                    // check jump
                    if (ds.isJumping)
                    {
                        ds.grounded = false;
                        ds.isJumping = false;
                        velocity.y = ds.jumpVelocity; // for nicer instantaneous jumps, try to change to += if it feels weird
                    }

                    // get acceleration depending on state
                    float accel;
                    if (ds.walking)
                    {
                        accel = ds.ground_accelerate;
                    }
                    else
                    {
                        accel = ds.air_accelerate;
                    }
                    
                    // apply friction
                    float speed = length(velocity);
                    if (speed < small_threshold) // if too slow, stop (to prevent sliding)
                    {
                        velocity.x = 0;
                        velocity.z = 0;
                    }
                    else
                    {
                        float drop = 0.0f;
                        if (ds.walking)
                        {
                            float control = speed < ds.stopspeed ? ds.stopspeed : speed;
                            drop = control * ds.friction * dt;
                        }

                        float new_speed = speed - drop;
                        new_speed = max(new_speed, 0.0f);
                        new_speed /= speed;
                        velocity *= new_speed;
                    }

                    // do movement
                    vec3 wishdir = ds.wantedDepDirection;
                    float wishspeed = ds.wantedSpeed;

                    float current_speed = dot(velocity, wishdir);
                    float addspeed = wishspeed - current_speed;
                    if (addspeed > 0)
                    {
                        float accelspeed = accel * dt * wishspeed;
                        accelspeed = min(accelspeed, addspeed);
                        velocity += wishdir * accelspeed;
                    }                    

                    


                    // float maxspeed = ds.grounded ? ds.wantedSpeed : ds.airSpeed; 
                    // vec3 vel = PG::toglm(b->getLinearVelocity());
                    // vec3 dir = ds.wantedDepDirection;
                    // ds.speed = length(vec2(vel.x, vel.z));
                    // ds.speed = length(vel);

                    

                    b->setLinearVelocity(PG::torp3d(velocity));

                    // std::cout << 
                    //     "pos: " << glm::to_string(s.position) << "\n" <<
                    //     "vel: " << glm::to_string(velocity) << "\n" <<
                    //     "wishdir: " << glm::to_string(wishdir) << "\n" <<
                    //     "wishspeed: " << wishspeed << "\n" <<
                    //     "grounded: " << ds.grounded << "\n" <<
                    //     "walking: " << ds.walking << "\n" <<
                    //     "velocity . ground normal: " << dot(velocity, callback.hitNormal) << "\n"
                    //     ;

                    // // if(ds.speed > 0.001)
                    // // {
                    // //     ds.speed *= abs(dot(dir, normalize(vel)));
                    // //     std::cout << abs(dot(dir, normalize(vel))) << "\n";
                    // // }

                    // if(ds.speed > 0.001)
                    // {
                    //     b->applyWorldForceAtCenterOfMass(PG::torp3d(-vec3(vel.x, 0, vel.z) * pow(maxspeed, 0.5f) * 5.f * b->getMass()));
                    // }

                    // // b->getCollider(0)->getMaterial().
                    

                    // if(ds.speed < maxspeed)
                    //     b->applyWorldForceAtCenterOfMass(PG::torp3d(dir * pow(maxspeed, 0.5f) * 50.f * b->getMass())); 
                    
                    // // b->applyWorldForceAtCenterOfMass(PG::torp3d(dir * pow(maxspeed, 0.5f) * 15.f * b->getMass())); 

                }
                break;
            
            /* Entities with a kinematic body are stucked :(. The body follows the entity state.
               The game assumes that another system is using the entity and dealing with his deplacement.*/
            case rp3d::BodyType::KINEMATIC :
                {
                    /* TODO : maybe fix one day for entities who are not using quaternion*/
                    rp3d::Transform t(PG::torp3d(s.position), s.usequat ? PG::torp3d(s.quaternion) : rp3d::Quaternion::identity());
                    b->setTransform(t);
                }
                break;

            default: break;
            }

        });

    // /***** SYNCHRONIZING MEG
    // *****/
    //     System<EntityGroupInfo>([&, this](Entity &entity)
    //     {
    //         ComponentModularity::synchronizeChildren(entity);
    //     });

        System<AgentState__old>([&, this](Entity &entity){
            entity.comp<AgentState__old>().timeSinceLastState += globals.simulationTime.getDelta();
        });

    /***** COMBAT DEMO AGENTS *****/
        System<AgentState, Script>([&, this](Entity &entity)
        {
            auto &as = entity.comp<AgentState>();
            float time = globals.simulationTime.getElapsedTime();

            time -= 0.05 * (entity.ids[ComponentCategory::AI]%128);
            
            if(time - as.lastUpdateTime > as.nextUpdateDelay)
            {
                entity.comp<Script>().run_OnAgentUpdate(entity);
            }
        });


        // static int i = 0;
        // if((i++)%10 == 0)
        // System<EntityState3D, EntityDeplacementState, AgentState__old, ActionState, Target>([&, this](Entity &entity){

        //     auto &s = entity.comp<EntityState3D>();
        //     auto &ds = entity.comp<EntityDeplacementState>();
        //     auto target = entity.comp<Target>();
        //     auto &as = entity.comp<AgentState__old>();
        //     auto &action = entity.comp<ActionState>();

        //     if(action.stun)
        //         return;
            
        //     if(entity.hasComp<EntityStats>() && !entity.comp<EntityStats>().alive)
        //         return;

        //     switch (as.state)
        //     {
        //         case AgentState__old::COMBAT_POSITIONING : if(!target.get()) return;
        //             {
        //                 auto &st = target->comp<EntityState3D>();   
        //                 float d = distance(s.position, st.position);

        //                 if(!target->comp<EntityStats>().alive)
        //                 {
        //                     ds.wantedSpeed = 0;
        //                     return;
        //                 }

        //                 float rangeMin = 1.5f;
        //                 float rangeMax = 2.f;

        //                 auto &taction = target->comp<ActionState>();   

        //                 s.lookDirection = normalize(st.position-s.position);

        //                 if(action.attacking || action.blocking)
        //                 {
        //                     ds.wantedSpeed = 0; return;
        //                 }

        //                 if(d > rangeMax || d < rangeMin)  
        //                 {
        //                     ds.wantedDepDirection = normalize((st.position-s.position)*vec3(1, 0, 1)) * (d < rangeMin ? -1.f : 1.f);
        //                     ds.wantedSpeed = 1.5;
        //                 }
        //                 else
        //                 {
        //                     // s.wantedDepDirection = vec3(0);
        //                     // s.speed = 0;

        //                     float time = globals.simulationTime.getElapsedTime();
        //                     float angle = PI*2.f*random01Vec2(vec2(time - mod(time, 1.f) + 25.6984f*entity.ids[ENTITY_LIST]));
        //                     vec3 randDir(cos(angle), 0, sin(angle));

        //                     // s.wantedDepDirection = normalize(cross(st.position-s.position, vec3(0, 1, 0) + randDir));
        //                     ds.wantedDepDirection = randDir;
        //                     ds.wantedSpeed = dot(ds.wantedDepDirection, normalize((st.position-s.position)*vec3(1, 0, 1))) < 0.75 ? 0.5f : 0.f;
        //                 }

        //                 if(d <= rangeMax)
        //                 {
        //                     if(taction.attacking)
        //                         as.TransitionTo(AgentState__old::COMBAT_BLOCKING, 0.5, 1.0);
        //                     else 
        //                     if(as.timeSinceLastState > as.randomTime)
        //                         as.TransitionTo(AgentState__old::COMBAT_ATTACKING);
        //                 }
        //             }
        //             break;
                
        //         case AgentState__old::COMBAT_ATTACKING : if(!target.get()) return;
        //             {
        //                 action.isTryingToAttack = true;
        //                 action.isTryingToBlock = false;

        //                 if(action.attacking) return;

        //                 auto &taction = target->comp<ActionState>();   

        //                 if(rand()%4 == 0)
        //                     action.setStance(ActionState::SPECIAL);
        //                 else if(taction.blocking)
        //                     action.setStance(taction.stance());
        //                 else
        //                     action.setStance(rand()%2 ? ActionState::LEFT : ActionState::RIGHT);

        //                 as.TransitionTo(AgentState__old::COMBAT_POSITIONING, 1.5, 3.0);
        //             }
        //             break;

        //         case AgentState__old::COMBAT_BLOCKING : if(!target.get()) return;
        //             {
        //                 auto &st = target->comp<EntityState3D>();  
        //                 auto &taction = target->comp<ActionState>();   
        //                 s.lookDirection = normalize(st.position-s.position);

        //                 if(taction.stun)
        //                 {
        //                     as.TransitionTo(AgentState__old::COMBAT_ATTACKING);
        //                     return;
        //                 }

        //                 action.isTryingToAttack = false;
        //                 action.isTryingToBlock = true;

        //                 if(as.timeSinceLastState > as.randomTime)
        //                 {
        //                     as.TransitionTo(AgentState__old::COMBAT_POSITIONING, 1.5, 3.0);
        //                     action.isTryingToBlock = false;
        //                     return;
        //                 }   

        //                 if(taction.attacking)
        //                     switch (taction.stance())
        //                     {
        //                         case ActionState::LEFT    : action.setStance(ActionState::RIGHT); break;
        //                         case ActionState::RIGHT   : action.setStance(ActionState::LEFT); break;
        //                         case ActionState::SPECIAL : action.setStance(ActionState::SPECIAL); break;
        //                         default: break;
        //                     }
        //             }
        //             break;

        //         default:
        //             break;
        //     }

        // });

        physicsSystemsTimer.stop();
        physicsTimer.stop();

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

        

        physicsMutex.unlock();

        physicsTicks.waitForEnd();
    }

    for(auto &i : Loader<ScriptInstance>::loadedAssets)
        if(i.second.lua_state() == threadState)
            i.second = ScriptInstance();
}
