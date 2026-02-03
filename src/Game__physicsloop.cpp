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

#include "PlayerUtils.hpp"

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
        
        PG::physicInterpolationMutex.lock();
        PG::physicInterpolationTick.tick();
        PG::physicInterpolationMutex.unlock();

        ManageGarbage<RigidBody>();
        ManageGarbage<Target>();

        if(GG::playerEntity && globals._currentController == &spectator && GG::playerEntity->has<RigidBody>())
        {
            auto body = GG::playerEntity->comp<RigidBody>();
            if(body)
            {
                body->setIsActive(false);

                if(GG::playerEntity->has<staticEntityFlag>())
                    GG::playerEntity->comp<staticEntityFlag>().shoudBeActive = false;
            }
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

        System<MovementState>([](Entity& entity)
        {
            entity.comp<MovementState>()._grounded = false;
        });

        physicsWorldUpdateTimer.start();
        PG::world->update(globals.simulationTime.speed / physicsTicks.freq);
        physicsWorldUpdateTimer.stop();

        System<MovementState>([](Entity& entity)
        {
            MovementState& ds = entity.comp<MovementState>();
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



        System<RigidBody, state3D, staticEntityFlag>([](Entity &entity){
            auto &b = entity.comp<RigidBody>();
            auto &f = entity.comp<staticEntityFlag>();

            f.isActive = b->isActive();
            
            if(f.isActive) f.activeIterationCnt ++;
            else f.activeIterationCnt = 0;

            if(f.shoudBeActive != f.isActive
            
                and b->isAllowedToSleep()
            )
            {
                // WARNING_MESSAGE(f.shoudBeActive << "\t" <<  entity.toStr())
                b->setIsActive(f.shoudBeActive);
                f.isActive = f.shoudBeActive;

                // b->setIsAllowedToSleep(bool isAllowedToSleep)
            }


        });

        // static vec3 minPos = vec3(1e6);
        // WARNING_MESSAGE(minPos.y);

        // int nearbyCells = 0;
        // System<RigidBody, state3D, HeightFieldDummyFlag>([&nearbyCells](Entity &entity){
        //     RigidBody b = entity.comp<RigidBody>();
            
        //     vec3 centerPos = PG::toglm(b->getTransform().getPosition());
        //     centerPos.y = 0;

        //     if (!GG::playerEntity || !GG::playerEntity->has<state3D>())
        //     {
        //         return;
        //     }
        //     vec3 playerPos = GG::playerEntity->comp<state3D>().position;
        //     playerPos.y = 0;

        //     float d = distance(playerPos, centerPos);

        //     if (d < Blueprint::cellSize)
        //     {
        //         b->getCollider(0)->setIsWorldQueryCollider(true);
        //         nearbyCells++;
        //     }
        //     else {
        //         b->getCollider(0)->setIsWorldQueryCollider(false);
        //     }
        // });

        // std::cout << "nearbyCells: " << nearbyCells << std::endl;

    /***** UPDATING RIGID BODY AND ENTITY STATE RELATIVE TO THE BODY TYPE *****/
        System<RigidBody, state3D, staticEntityFlag>([](Entity &entity){
            auto &b = entity.comp<RigidBody>();
            auto &s = entity.comp<state3D>();
            

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



                    
                    // minPos = min(minPos, s.position);
                    


                    
                    if(!entity.has<MovementState>()) break;

                    auto &ms = entity.comp<MovementState>();

                    const float small_threshold = 0.05f;

                    vec3 velocity = PG::toglm(b->getLinearVelocity());

                    float dt = globals.simulationTime.getDelta();

                    ms.deplacementDirection = normalize(velocity);
                    ms.speed = length(velocity);

                    // apply gravity
                    vec3 v = velocity + vec3(0, -ms.gravity, 0) * dt;

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

                        if (as.climbing)
                        {
                            const float climbSpeedSlow = 1.0f; // m/s 
                            const float climbSpeedFast = 4.0f; // m/s 
                            float climbSpeed = climbSpeedSlow;
                            const float vaultOffset = 0.2f;


                            const float minSpeedForFastVault = 6.0f;
                            const float maxHeightForFastVault = 1.0;
                            if (ms.climbStartSpeed >= minSpeedForFastVault && ms.climbHeight <= maxHeightForFastVault)
                            {
                                climbSpeed = climbSpeedFast;
                            }

                            // 0 < t < ms.climbHeight
                            float t = (globals.simulationTime.getElapsedTime() - as.climbingTime) * climbSpeed;
                            
                            // climb anim 
                            if (t <= ms.climbHeight)
                            {
                                if (ms.climbHeight == vaultOffset) ms.climbHeight += 0.0001; // prevent division by 0 error

                                float t_vault = max(t - vaultOffset, 0.0f) / (ms.climbHeight - vaultOffset);
                                
                                // Logger::debug("t: ", t, " t vault: ", t_vault);

                                vec3 pos = ms.climbStartPos + vec3(0, 1, 0) * t;
                            
                                pos = mix(pos, ms.climbVaultPos, t_vault);

                                // Logger::debug("pos'd: ", pos);

                                rp3d::Transform transform(PG::torp3d(pos), b->getTransform().getOrientation());
                                b->setTransform(transform);

                                vec3 forward = ms.climbVaultPos - ms.climbStartPos;
                                forward.y = 0;
                                PlayerViewController::setTargetDir(normalize(forward));
                            }
                            else // climb anim end
                            {
                                as.climbing = false;
                                ms.isAllowedToClimb = true;

                                size_t collider_n = b->getNbColliders();
                                for (size_t i = 0; i < collider_n; i++)
                                {
                                    b->getCollider(i)->setIsSimulationCollider(true);
                                }

                                PlayerViewController::setEnabled(false);
                            }

                            b->setLinearVelocity(rp3d::Vector3(0, 0, 0));
                            ms.speed = 0.0f;
                            ms.isTryingToJump = false;
                            ms.wantedSpeed = 0;
                        }

                        if (as.stun)
                        {
                            ms.wantedSpeed = 0.f;
                            ms.isTryingToJump = false;
                        }
                        else switch (as.lockType)
                        {
                            case ActionState::DIRECTION :
                                ms.wantedMoveDirection = as.lockedDirection;
                                ms.isTryingToJump = false;
                                break;
                            case ActionState::SPEED_ONLY :
                                ms.wantedSpeed = as.lockedMaxSpeed; 
                                ms.isTryingToJump = false;
                                break;
                            default: break;
                        }
                    }

                    bool canClimb = false;
                    vec3 handPosLeft, handPosRight;
                    if (entity.is(*GG::playerEntity) && ms.isAllowedToClimb)
                    {
                        vec3 angleVector_forward, angleVector_right, angleVector_up;
                        angleVectors(globals.currentCamera->getDirection(), angleVector_forward, angleVector_right, angleVector_up);
                        angleVector_forward.y = 0.00001;
                        angleVector_forward = normalize(angleVector_forward);
                        vec3 vaultWallCheckRoot = angleVector_forward * (length(velocity) < 6.0f ? 0.75f : 1.25f);

                        vec3 verticalOffset = vec3(0, 0.75, 0);

                        float distanceFromCenter = 0.3;
                        
                        vec3 p1 = s.position + verticalOffset;
                        vec3 p2 = s.position + verticalOffset + vaultWallCheckRoot;

                        vec3 p1_1 = p1 + angleVector_right * distanceFromCenter;
                        vec3 p1_2 = p1 - angleVector_right * distanceFromCenter;

                        vec3 p2_1 = p2 + angleVector_right * distanceFromCenter;
                        vec3 p2_2 = p2 - angleVector_right * distanceFromCenter;

                        ensureNonZeroVectorComponents(p1_1);
                        ensureNonZeroVectorComponents(p2_1);
                        rp3d::Ray ray_1(PG::torp3d(p1_1), PG::torp3d(p2_1));
                        PG::RayCastCallback cb_1(p1_1, p2_1);
                        PG::world->raycast(ray_1, &cb_1, 1<<CollideCategory::ENVIRONEMENT);

                        ensureNonZeroVectorComponents(p1_2);
                        ensureNonZeroVectorComponents(p2_2);
                        rp3d::Ray ray_2(PG::torp3d(p1_2), PG::torp3d(p2_2));
                        PG::RayCastCallback cb_2(p1_2, p2_2);
                        PG::world->raycast(ray_2, &cb_2, 1<<CollideCategory::ENVIRONEMENT);

                        if (cb_1.hit && cb_2.hit)
                        {
                            vec3 hit1 = cb_1.hitPoint;
                            vec3 hit2 = cb_2.hitPoint;
                            vec3 hitVector = normalize(hit1 - hit2);
                            float d = dot(hitVector, angleVector_forward);
                            if (abs(d) < 0.15f)
                            {
                                float offset_in = 0.1;
                                vec3 hitTest_1 = hit1 + normalize(p2_1 - p1_1) * offset_in;
                                vec3 hitTest_2 = hit2 + normalize(p2_2 - p1_2) * offset_in;
                                float testHeightMax = 2.5;
                                float testHeightMin = 0.5;
                                
                                vec3 heightTest_1_start = vec3(hitTest_1.x, testHeightMax, hitTest_1.z);
                                vec3 heightTest_1_end   = vec3(hitTest_1.x, testHeightMin, hitTest_1.z);
                                ensureNonZeroVectorComponents(heightTest_1_start);
                                ensureNonZeroVectorComponents(heightTest_1_end);

                                vec3 heightTest_2_start = vec3(hitTest_2.x, testHeightMax, hitTest_2.z);
                                vec3 heightTest_2_end   = vec3(hitTest_2.x, testHeightMin, hitTest_2.z);
                                ensureNonZeroVectorComponents(heightTest_2_start);
                                ensureNonZeroVectorComponents(heightTest_2_end);

                                rp3d::Ray ray_height_1(PG::torp3d(heightTest_1_start), PG::torp3d(heightTest_1_end));
                                PG::RayCastCallback cb_height_1(heightTest_1_start, heightTest_1_end);
                                PG::world->raycast(ray_height_1, &cb_height_1, 1<<CollideCategory::ENVIRONEMENT);

                                rp3d::Ray ray_height_2(PG::torp3d(heightTest_2_start), PG::torp3d(heightTest_2_end));
                                PG::RayCastCallback cb_height_2(heightTest_2_start, heightTest_2_end);
                                PG::world->raycast(ray_height_2, &cb_height_2, 1<<CollideCategory::ENVIRONEMENT);

                                float maxHeightDifference = 0.05;
                                if (
                                    cb_height_1.hit && cb_height_2.hit
                                    && cb_height_1.minDistance - cb_height_2.minDistance < maxHeightDifference
                                )
                                {
                                    float avgDist = (cb_height_1.minDistance + cb_height_2.minDistance) / 2.0f;
                                    float height = testHeightMax - avgDist;
                                    canClimb = true;
                                    ms.climbStartPos = s.position;
                                    ms.climbEndPos = s.position;
                                    ms.climbEndPos.y += height;
                                    ms.climbVaultPos =    ms.climbEndPos 
                                                        + angleVector_forward * cb_1.minDistance 
                                                        + angleVector_forward * offset_in * 2.0f;
                                    ms.climbHeight = height;
                                    ms.climbStartSpeed = length(velocity);

                                    // Logger::debug(cb_1.minDistance, " ", angleVector_forward * cb_1.minDistance, " ", angleVector_forward * offset_in * 2.0f);

                                    handPosLeft = cb_height_1.hitPoint;
                                    handPosRight = cb_height_2.hitPoint;
                                    // Logger::debug("[",  globals.simulationTime.getElapsedTime(), "] can climb :) height: ", height);
                                }
                                
                            }
                        }
                    }

                    // check jump
                    if (ms.isTryingToJump)
                    {
                        ms.grounded = false;
                        ms.isTryingToJump = false;
                        
                        if (canClimb)
                        {
                            // assuming the entity has an action state cause like it's the player
                            auto &as = entity.comp<ActionState>();
                            as.climbing = true;
                            as.climbingTime = globals.simulationTime.getElapsedTime();
                            // TODO check if the entity is the player 
                            PlayerViewController::setEnabled(true);
                            ms.isAllowedToClimb = false;
                            
                            size_t collider_n = b->getNbColliders();
                            for (size_t i = 0; i < collider_n; i++)
                            {
                                b->getCollider(i)->setIsSimulationCollider(false);
                            }

                            // Logger::debug("[", globals.appTime.getElapsedTime(), "] climb :)");
                            // Logger::debug("start pos: ", ms.climbStartPos);
                            // Logger::debug("end pos: ", ms.climbEndPos);
                            // Logger::debug("vault pos: ", ms.climbVaultPos);
                        }
                        else {
                            // Logger::debug("[", globals.appTime.getElapsedTime(), "] jump");
                            velocity.y = ms.jumpVelocity; // for nicer instantaneous jumps, try to change to += if it feels weird
                        }
                    }

                    // get acceleration depending on state
                    float accel;
                    if (ms.walking)
                    {
                        // accel = ds.ground_accelerate;

                        // try to slow the player down if they're on a slope
                        // kinda magic values, they give a nicer feel imo
                        const float scaling = 4.0f;
                        const float accelZeroAtThisNormal = 0.6f;
                        // normalize t to be between accelZeroAtThisNormal and 1
                        float t = (ms.groundNormalY - accelZeroAtThisNormal) / (1.0f - accelZeroAtThisNormal);
                        // function from https://easings.net/#easeInExpo with modified scaling
                        float normalDistanceFromUp = t == 0.0f ? 0 : pow(2.0f, t * scaling - scaling);

                        normalDistanceFromUp = clamp(normalDistanceFromUp, 0.0f, 1.0f);

                        // interpolate between air accelerate and ground accelerate depending on the slope's angle
                        accel = mix(ms.air_accelerate, ms.ground_accelerate, normalDistanceFromUp); 
                    }
                    else
                    {
                        accel = ms.air_accelerate;
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
                        if (ms.walking)
                        {
                            float control = speed < ms.stopspeed ? ms.stopspeed : speed;
                            drop = control * ms.friction * dt;
                        }

                        float new_speed = speed - drop;
                        new_speed = max(new_speed, 0.0f);
                        new_speed /= speed;
                        velocity *= new_speed;
                    }

                    // do movement
                    vec3 wishdir = ms.wantedMoveDirection;
                    float wishspeed = ms.wantedSpeed;

                    if(entity.has<EntityStats>())
                        wishspeed = mix(
                            ms.wantedSpeed,
                            min(ms.wantedSpeed, ms.walkSpeed),
                            smoothstep(
                                25.f,
                                10.f,
                                entity.comp<EntityStats>().stamina.cur
                            )
                        );

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
        // System<state3D, MovementState, AgentState__old, ActionState, Target>([&, this](Entity &entity){

        //     auto &s = entity.comp<state3D>();
        //     auto &ds = entity.comp<MovementState>();
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
        //                     ds.wantedMoveDirection = normalize((st.position-s.position)*vec3(1, 0, 1)) * (d < rangeMin ? -1.f : 1.f);
        //                     ds.wantedSpeed = 1.5;
        //                 }
        //                 else
        //                 {
        //                     // s.wantedMoveDirection = vec3(0);
        //                     // s.speed = 0;

        //                     float time = globals.simulationTime.getElapsedTime();
        //                     float angle = PI*2.f*random01Vec2(vec2(time - mod(time, 1.f) + 25.6984f*entity.ids[ENTITY_LIST]));
        //                     vec3 randDir(cos(angle), 0, sin(angle));

        //                     // s.wantedMoveDirection = normalize(cross(st.position-s.position, vec3(0, 1, 0) + randDir));
        //                     ds.wantedMoveDirection = randDir;
        //                     ds.wantedSpeed = dot(ds.wantedMoveDirection, normalize((st.position-s.position)*vec3(1, 0, 1))) < 0.75 ? 0.5f : 0.f;
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
        float minFreq = 5.f;

        if(physicsTimer.getDelta() > 1.0/physicsTicks.freq)
        {
            physicsTicks.freq = clamp(physicsTicks.freq/2.f, minFreq, maxFreq);
        }
        else if(physicsTimer.getDelta() < 0.5f/physicsTicks.freq)
        {
            physicsTicks.freq = clamp(physicsTicks.freq*2.f, minFreq, maxFreq);
        }

        physicsMutex.unlock();

        physicsTicks.waitForEnd();

        // const float stepSize = 5.f / 1000.f;
        // // physicsTicks.freq = ceil(1.0/(physicsTimer.getDelta()*stepSize))*stepSize;
        
        // float delta = ceil(physicsTimer.getDelta()/stepSize)*stepSize;
        // physicsTicks.freq = 1.0/delta;
        
        // physicsTicks.freq = clamp(physicsTicks.freq, minFreq, maxFreq);



    }

    for(auto &i : Loader<ScriptInstance>::loadedAssets)
        if(i.second.lua_state() == threadState)
            i.second = ScriptInstance();
}
