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

        if(globals._currentController == &spectator && GG::playerEntity->hasComp<RigidBody>())
        {
            auto body = GG::playerEntity->comp<RigidBody>();
            if(body)
                body->setIsActive(false);
        }

        physicsWorldUpdateTimer.start();
        PG::world->update(globals.simulationTime.speed / physicsTicks.freq);
        physicsWorldUpdateTimer.end();

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

                    /*
                        Fastest way possible to see if the player is grounded.
                            If the player comes into contact with a surface exerting an opposing force to 
                            gravity, their vertical velocity will be canceled out.

                            This method can theoricly leads to a rare double jump exploit.
                            Luna said this methode render bunny hopping impossible.
                    */
                    ds.grounded = abs(b->getLinearVelocity().y) < 5e-4;
                    ds.grounded = true; // TODO : fix grounded
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
                        b->applyWorldForceAtCenterOfMass(PG::torp3d(dir * pow(maxspeed, 0.4f) * 15.f * b->getMass()));            
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

        physicsSystemsTimer.end();
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

        ManageGarbage<RigidBody>();

        physicsMutex.unlock();

        physicsTicks.waitForEnd();
    }
}
