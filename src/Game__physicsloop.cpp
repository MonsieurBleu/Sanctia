#include <fstream>
#include <thread>

#include <Game.hpp>
#include <Globals.hpp>
#include <CompilingOptions.hpp>
#include <Constants.hpp>
#include <AssetManager.hpp>
#include <MathsUtils.hpp>
#include <EntityBlueprint.hpp>

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
        ManageGarbage<Target>();

        if(GG::playerEntity && globals._currentController == &spectator && GG::playerEntity->has<RigidBody>())
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
        System<RigidBody, state3D, staticEntityFlag>([](Entity &entity){
            auto &b = entity.comp<RigidBody>();
            vec3 p = PG::toglm(b->getTransform().getPosition());
            const float lowinf = 1e6;
            if(p.x > lowinf || p.y > lowinf || p.z > lowinf || p.x < -lowinf || p.y < -lowinf || p.z < -lowinf)
                b->setTransform(rp3d::Transform::identity());
        });

        System<DeplacementState>([](Entity& entity)
        {
            entity.comp<DeplacementState>()._grounded = false;
        });

        physicsWorldUpdateTimer.start();
        PG::world->update(globals.simulationTime.speed / physicsTicks.freq);
        physicsWorldUpdateTimer.stop();

        System<DeplacementState>([](Entity& entity)
        {
            DeplacementState& ds = entity.comp<DeplacementState>();
            if (!ds.grounded) // just landed
            {
                ds.landedTime = globals.simulationTime.getElapsedTime();
            }
            if (ds._grounded)
            {
                ds.grounded = true;
                ds.walking  = true;
            }
            else {
                ds.grounded = false;
                ds.walking  = false;
            }
            ds.groundNormalY = ds._groundNormalY;
        });

        physicsSystemsTimer.start();

        PG::physicInterpolationMutex.lock();
        PG::physicInterpolationTick.tick();
        PG::physicInterpolationMutex.unlock();

        int nearbyCells = 0;
        System<RigidBody, state3D, HeightFieldDummyFlag>([&nearbyCells](Entity &entity){
            RigidBody b = entity.comp<RigidBody>();
            
            vec3 centerPos = PG::toglm(b->getTransform().getPosition());
            centerPos.y = 0;

            if (!GG::playerEntity || !GG::playerEntity->has<state3D>())
            {
                return;
            }
            vec3 playerPos = GG::playerEntity->comp<state3D>().position;
            playerPos.y = 0;

            float d = distance(playerPos, centerPos);

            if (d < Blueprint::cellSize)
            {
                b->getCollider(0)->setIsWorldQueryCollider(true);
                nearbyCells++;
            }
            else {
                b->getCollider(0)->setIsWorldQueryCollider(false);
            }
        });

        // std::cout << "nearbyCells: " << nearbyCells << std::endl;

    /***** UPDATING RIGID BODY AND ENTITY STATE RELATIVE TO THE BODY TYPE *****/
        System<RigidBody, state3D, staticEntityFlag>([](Entity &entity){
            auto &b = entity.comp<RigidBody>();
            auto &s = entity.comp<state3D>();
            auto &ds = entity.comp<DeplacementState>();

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
                    
                    if(!entity.has<DeplacementState>()) break;

                    const float small_threshold = 0.05f;

                    vec3 velocity = PG::toglm(b->getLinearVelocity());

                    float dt = globals.simulationTime.getDelta();

                    ds.deplacementDirection = normalize(velocity);
                    ds.speed = length(velocity);

                    // apply gravity
                    vec3 v = velocity + vec3(0, -ds.gravity, 0) * dt;

                    velocity = (velocity + v) * 0.5f; // smoothing the gravity effect, feels nicer imo


                    // get input from ActionState if any
                    if(entity.has<ActionState>())
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

                    vec3 angleVector_forward, angleVector_right, angleVector_up;
                    angleVectors(globals.currentCamera->getDirection(), angleVector_forward, angleVector_right, angleVector_up);
                    angleVector_forward.y = 0.00001;
                    angleVector_forward = normalize(angleVector_forward);
                    vec3 vaultGroundCheck = angleVector_forward * 1.5f + s.position;

                    vec3 verticalOffset = vec3(0, 1.0, 0);
                    vec3 p1 = s.position       + verticalOffset;
                    p1.x = p1.x == 0.0f ? 0.0001f : p1.x;
                    p1.y = p1.y == 0.0f ? 0.0001f : p1.y;
                    p1.z = p1.z == 0.0f ? 0.0001f : p1.z;
                    vec3 p2 = vaultGroundCheck + verticalOffset;
                    rp3d::Ray ray(PG::torp3d(p1), PG::torp3d(p2));
                    PG::RayCastCallback cb(p1, p2);
                    PG::world->raycast(ray, &cb, 1<<CollideCategory::ENVIRONEMENT);

                    // std::cout << "p1: " << p1 << std::endl;
                    if (cb.hit)
                        std::cout << "[" << globals.simulationTime.getElapsedTime() << "] Hit: distance: " << std::to_string(cb.minDistance) << std::endl;

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
                        // accel = ds.ground_accelerate;

                        // try to slow the player down if they're on a slope
                        // kinda magic values, they give a nicer feel imo
                        const float scaling = 4.0f;
                        const float accelZeroAtThisNormal = 0.6f;
                        // normalize t to be between accelZeroAtThisNormal and 1
                        float t = (ds.groundNormalY - accelZeroAtThisNormal) / (1.0f - accelZeroAtThisNormal);
                        // function from https://easings.net/#easeInExpo with modified scaling
                        float normalDistanceFromUp = t == 0.0f ? 0 : pow(2.0f, t * scaling - scaling);

                        normalDistanceFromUp = clamp(normalDistanceFromUp, 0.0f, 1.0f);

                        // interpolate between air accelerate and ground accelerate depending on the slope's angle
                        accel = mix(ds.air_accelerate, ds.ground_accelerate, normalDistanceFromUp); 
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
                    // std::cout << "ds.wantedSpeed: " << ds.wantedSpeed << "\n";
                    if (addspeed > 0)
                    {
                        float accelspeed = accel * dt * wishspeed;
                        accelspeed = min(accelspeed, addspeed);
                        velocity += wishdir * accelspeed;
                    }
                    

                    b->setLinearVelocity(PG::torp3d(velocity));
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

            time -= 0.05 * (entity.ids[ComponentCategory::AI]%16);
            
            if(time - as.lastUpdateTime > as.nextUpdateDelay)
            {
                entity.comp<Script>().run_OnAgentUpdate(entity);
            }
        });


        // static int i = 0;
        // if((i++)%10 == 0)
        // System<state3D, DeplacementState, AgentState__old, ActionState, Target>([&, this](Entity &entity){

        //     auto &s = entity.comp<state3D>();
        //     auto &ds = entity.comp<DeplacementState>();
        //     auto target = entity.comp<Target>();
        //     auto &as = entity.comp<AgentState__old>();
        //     auto &action = entity.comp<ActionState>();

        //     if(action.stun)
        //         return;
            
        //     if(entity.has<EntityStats>() && !entity.comp<EntityStats>().alive)
        //         return;

        //     switch (as.state)
        //     {
        //         case AgentState__old::COMBAT_POSITIONING : if(!target.get()) return;
        //             {
        //                 auto &st = target->comp<state3D>();   
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
        //                 auto &st = target->comp<state3D>();  
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
