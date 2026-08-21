#include <Game.hpp>
#include <Globals.hpp>
#include <CompilingOptions.hpp>
#include <Constants.hpp>
#include <AssetManager.hpp>
#include <MathsUtils.hpp>
#include <EntityBlueprint.hpp>
#include <Scripting/ScriptInstance.hpp>
#include <SanctiaLuaBindings.hpp>

#include <JoltIntegration/PhysicsDebugRenderer.hpp>

#include <JoltIntegration/PhysicsCommons.hpp>

#include <Jolt/Core/Factory.h>
#include <Jolt/Core/TempAllocator.h>
#include <Jolt/Core/JobSystemThreadPool.h>


#include <Jolt/Physics/Collision/Shape/BoxShape.h>
#include <Jolt/Physics/Collision/Shape/SphereShape.h>
#include <Jolt/Physics/Collision/Shape/CylinderShape.h>
#include <Jolt/Physics/Collision/Shape/CapsuleShape.h>
#include <Jolt/Physics/Collision/Shape/RotatedTranslatedShape.h>
#include <Jolt/Physics/Body/BodyCreationSettings.h>
#include <Jolt/Physics/Body/BodyActivationListener.h>

#include <Jolt/Physics/Collision/NarrowPhaseQuery.h>
#include <Jolt/Physics/Collision/RayCast.h>
#include <Jolt/Physics/Collision/CastResult.h>

#include <Jolt/Physics/Collision/ShapeCast.h>

// JPH::AssertFailedFunction JPH::AssertFailed;

void Game::physicsLoop2()
{
    currentThreadID = JOLT_MAIN_THREAD_ID;
    JoltVulpine::physicsMutex.lock();
    JoltVulpine::physicsTicks.freq = 60.f;
    JoltVulpine::physicsTicks.activate();

    threadStateName = "Jolt Thread";
    threadState.open_libraries(
        sol::lib::base, 
        sol::lib::coroutine, 
        sol::lib::string, 
        sol::lib::io,
        sol::lib::math,
        sol::lib::jit
    );

    SanctiaLuaBindings::bindAll(threadState);

    /****** 
              JOLT CONFIGURATION   0x7fffac937ab8
    ******/

    JPH::RegisterDefaultAllocator();

	JPH::Trace = JoltVulpine::TraceCallback;
	JPH_IF_ENABLE_ASSERTS(JPH::AssertFailed = JoltVulpine::AssertFailedCallback);

	JPH::Factory::sInstance = new JPH::Factory();
	JPH::RegisterTypes();

    /* Allocating x bytes per possible physics body */
    constexpr uint bytePerMaxBody = 128;
    constexpr uint joltPreAllocatedBytes = bytePerMaxBody  * Component<JoltBody>::elements.size();
    JPH::TempAllocatorImpl temp_allocator(joltPreAllocatedBytes);
    NOTIF_MESSAGE("Physics Thread pre-allocated ", joltPreAllocatedBytes/(1024*1024), " MB for physics world.")


    JPH::JobSystemThreadPool job_system(JPH::cMaxPhysicsJobs, JPH::cMaxPhysicsBarriers, std::thread::hardware_concurrency() - 1);

	// constexpr uint cMaxBodies = Component<Body>::elements.size();
    constexpr uint cMaxBodies = 1<<16;
	constexpr uint cNumBodyMutexes = 0;
	constexpr uint cMaxBodyPairs = 1<<18;
	constexpr uint cMaxContactConstraints = 1<<15;
    constexpr uint cNumBroadPhaseLayers = 8;

    JPH::BroadPhaseLayerInterfaceMask broadPhaseLayerInterface(cNumBroadPhaseLayers);
    JPH::ObjectVsBroadPhaseLayerFilterMask objectVsBroadPhaseLayerFilter(broadPhaseLayerInterface);
    JPH::ObjectLayerPairFilterMask objectLayerPairFilter;

    JoltVulpine::jPhysicsSystem = new JPH::PhysicsSystem();
    // JoltVulpine::debugRenderer = new JoltVulpine::DebugRenderer();

    JoltVulpine::jPhysicsSystem->Init(
        cMaxBodies, 
        cNumBodyMutexes, 
        cMaxBodyPairs, 
        cMaxContactConstraints, 
        broadPhaseLayerInterface,
        objectVsBroadPhaseLayerFilter,
        objectLayerPairFilter
    );

    JoltVulpine::jPhysicsSystem->SetCombineFriction([](const JPH::Body &inBody1, const JPH::SubShapeID &, const JPH::Body &inBody2, const JPH::SubShapeID &)
    {
        Entity *e1 = (Entity*)inBody1.GetUserData();
        Entity *e2 = (Entity*)inBody1.GetUserData();

        Entity *e = e1 && e1->has<Movement>() ? e1 : (e2 && e2->has<Movement>() ? e2 : nullptr);

        if(e and e->comp<Movement>().grounded.get())
        {
            if(JoltVulpine::useVelocityBasedMovement)
            {
                // return 0.25f;
                // return max(inBody1.GetFriction(), inBody2.GetFriction());
                return 0.f;
            }
            else
            {
                // return 1.f - dot(e->comp<Movement>().direction.current, e->comp<Movement>().direction.goal);
    
                auto &depl = e->comp<Movement>();
    
                float d = max(0.f, dot(depl.direction.current, depl.direction.goal));
                d = smoothstep(0.5f, 1.f, d);
                float f = 1.f - d;
    
                f *= smoothstep(0.f, 0.5f,depl.grounded.timeSinceChange());
    
                f *= 4.0;
                
                // NOTIF_MESSAGE(
                //     "\n\tcurrent:", depl.direction.current,
                //     "\n\tgoal   :", depl.direction.goal,
                //     "\n\td:", d,
                //      "\n\tf:", f
                //     )
    
                return f;
            }
        }
        else
        {
            return (inBody1.GetFriction(), inBody2.GetFriction())*0.5f;
        }
    });

    JoltVulpine::physicsMutex.unlock();

    // WARNING_MESSAGE(
    //     objectLayerPairFilter.ShouldCollide(
    //         JPH::ObjectLayerPairFilterMask::sGetObjectLayer(
    //             1<<JoltVulpine::Layers::Categories::ENVIRONEMENT, 
    //             1<<JoltVulpine::Layers::Categories::ENVIRONEMENT
    //         ),
    //         JPH::ObjectLayerPairFilterMask::sGetObjectLayer(
    //             1<<JoltVulpine::Layers::Categories::ENVIRONEMENT, 
    //             1<<JoltVulpine::Layers::Categories::ENVIRONEMENT | 1<<JoltVulpine::Layers::Categories::HIT_ZONE
    //         )
    //     )
    // )
        
    // WARNING_MESSAGE(
    //     objectLayerPairFilter.ShouldCollide(
    //         JPH::ObjectLayerPairFilterMask::sGetObjectLayer(
    //             1<<JoltVulpine::Layers::Categories::HIT_ZONE, 
    //             1<<JoltVulpine::Layers::Categories::ENVIRONEMENT
    //         ),
    //         JPH::ObjectLayerPairFilterMask::sGetObjectLayer(
    //             1<<JoltVulpine::Layers::Categories::ENVIRONEMENT, 
    //             1<<JoltVulpine::Layers::Categories::HIT_ZONE
    //         )
    //     )
    // )
        

    // WARNING_MESSAGE(
    //     objectLayerPairFilter.ShouldCollide(
    //         JPH::ObjectLayerPairFilterMask::sGetObjectLayer(
    //             1<<JoltVulpine::Layers::Categories::PICK_UP, 
    //             1<<JoltVulpine::Layers::Categories::ENVIRONEMENT
    //         ),
    //         JPH::ObjectLayerPairFilterMask::sGetObjectLayer(
    //             1<<JoltVulpine::Layers::Categories::ENVIRONEMENT, 
    //             1<<JoltVulpine::Layers::Categories::ENVIRONEMENT
    //         )
    //     )
    // )

    // WARNING_MESSAGE(
    //     objectLayerPairFilter.ShouldCollide(
    //         JPH::ObjectLayerPairFilterMask::sGetObjectLayer(
    //             1<<JoltVulpine::Layers::Categories::ENVIRONEMENT, 
    //             1<<JoltVulpine::Layers::Categories::ENVIRONEMENT
    //         ),
    //         JPH::ObjectLayerPairFilterMask::sGetObjectLayer(
    //             1<<JoltVulpine::Layers::Categories::PICK_UP, 
    //             1<<JoltVulpine::Layers::Categories::ENVIRONEMENT
    //         )
    //     )
    // )


    


    while (state != quit)
    {   
        // WARNING_MESSAGE(&JoltVulpine::jPhysicsSystem)
        
        if(globals.simulationTime.isPaused() && JoltVulpine::enablePhysics)
        {
            JoltVulpine::currentPhysicFreq = 0.f;
            continue;
        }
        else
            JoltVulpine::currentPhysicFreq = physicsTicks.freq;
    
        JoltVulpine::physicsTicks.start();
        JoltVulpine::physicsTimer.start();
        JoltVulpine::physicsMutex.lock();

        /* ........ ADD NEWLY CREATED BODIES TO PHYSICS SCENE  ........ */
		JoltVulpine::bodiesToAddMutex.lock();
        if(JoltVulpine::bodiesToAdd.size())
        {
            // NOTIF_MESSAGE("Adding ", JoltVulpine::bodiesToAdd.size(), " new bodies to the simulation.")
            auto btaInfo = JoltVulpine::jPhysicsSystem->GetBodyInterface().AddBodiesPrepare(
                JoltVulpine::bodiesToAdd.data(),
                JoltVulpine::bodiesToAdd.size()
            );
            JoltVulpine::jPhysicsSystem->GetBodyInterface().AddBodiesFinalize(
                JoltVulpine::bodiesToAdd.data(),
                JoltVulpine::bodiesToAdd.size(),
                btaInfo,
                JPH::EActivation::Activate
            );
            JoltVulpine::bodiesToAdd.clear();
        }
		JoltVulpine::bodiesToAddMutex.unlock();

        /* ........ PHYSIC WORLD UPDATE  ........ */

		const int cCollisionSteps = 2;
		JoltVulpine::physicsWorldUpdateTimer.start();
        float accSpeedFactor = 1.f/(60.f * globals.simulationTime.speed/JoltVulpine::physicsTicks.freq);
        JoltVulpine::jPhysicsSystem->Update(globals.simulationTime.speed/JoltVulpine::physicsTicks.freq, cCollisionSteps, &temp_allocator, &job_system);
        JoltVulpine::physicsWorldUpdateTimer.stop();
        
        /* ........ POST PHYSIC UPDATE SYSTEMS ........ */

        JoltVulpine::physicsSystemsTimer.start();
        System<KynematicFlag, JoltBody, State3D>([](Entity &e, KynematicFlag &k, JoltBody &b, State3D &s)
        {
            /*
                Entities with kynematic bodies follow State3D
            */
            if(k)
            {
                // NOTIF_MESSAGE(e.toStr(), "\n\t", s.position)
                s.position = s.position + vec3(0.0, 0, 0);
                JoltVulpine::jPhysicsSystem->GetBodyInterface().SetPositionAndRotation(
                    b, Vvec3(s.position), Vquat(s.rotation), JPH::EActivation::Activate
                );
            }

            /*
                Attach sensor to main body
            */
            for(auto &sensor : b.sensors)
            {
                if(sensor.IsInvalid()) continue;

                JoltVulpine::jPhysicsSystem->GetBodyInterface().SetPositionAndRotation(
                    sensor, Vvec3(s.position), Vquat(s.rotation), JPH::EActivation::Activate
                );
            }
        });


        /*
            Entities with dyamic main bodies updates State3D with physics data.
            They also use physic interpolation.
        */
        JoltVulpine::physicInterpolationMutex.lock();
        JoltVulpine::physicInterpolationTick.tick();
        System<DynamicState3D, State3D, JoltBody>([](Entity &e, DynamicState3D &ds, State3D &s, JoltBody &body)
        {
            if(!body.IsInvalid())
            {
                JPH::Vec3 position;
                JPH::Quat rotation;

                JoltVulpine::jPhysicsSystem->GetBodyInterface().GetPositionAndRotation(
                    body, 
                    position, 
                    rotation
                );

                ds.last = ds.next;
                ds.next = {Vvec3(position), Vquat(rotation)};

                if(!e.has<EntityModel>())
                    s = ds.next;
            }
        });
        JoltVulpine::physicInterpolationMutex.unlock();

        /*
            Manage complex movement like climbing
        */
        System<ComplexMovements, Movement, JoltBody, State3D>([&](Entity &e, ComplexMovements &move, Movement &depl, JoltBody &body, State3D &state)
        {
            auto &interface = JoltVulpine::jPhysicsSystem->GetBodyInterface();

            /*
                initialazing jump
            */
            if(move.jump.getGoal() and depl.grounded.get())
            {
                move.jump.acceptGoal();
                move.jump.setGoal(false);
                interface.AddForce(body, Vvec3(0, 1e6f * accSpeedFactor, 0));
            }
            /*
                reseting jump
            */
            if(move.jump.getCurrent() and depl.grounded.get() and move.jump.timeSinceGoalMet() > 0.25f)
            {
                move.jump.overrideCurrent(false);
            }

            /*
                initialazing wall jump
            */
            if(move.wallJump.getGoal() and not depl.grounded.get())
            {
                Jvec3 dir = vec3(0, 1, 0) + 0.5f*normalize(vec3(1, 0, 1)*(state.position-move.closestWall));

                // NOTIF_MESSAGE(dot(!dir, move.lastWallJumpDirection), length(move.lastWallJumpDirection))

                if(dot(!dir, move.lastWallJumpDirection) < 0.5f or length(move.lastWallJumpDirection) < 0.1f)
                {
                    move.wallJump.acceptGoal();
                    move.wallJump.setGoal(false);
                    interface.AddForce(body, 1e6f*dir*accSpeedFactor);
                    move.lastWallJumpDirection = dir;
                }
            }
            /*
                reseting wall jump
            */
            if(move.wallJump.getCurrent() and move.wallJump.timeSinceGoalMet() > 0.25f)
            {
                move.wallJump.overrideCurrent(false);
            }
            if(depl.grounded.get())
            {
                move.lastWallJumpDirection = vec3(0);
            }


            /*
                initialazing climn
            */
            if(!move.climb.getCurrent() and move.climb.getGoal() and move.climb.timeSinceGoalChange() < 0.1f)
            {
                move.climb.acceptGoal();
                move.climb.setGoal(false);
                move.animationInitialPosition = state.position;
                JoltVulpine::jPhysicsSystem->GetBodyInterface().SetMotionType(body, JPH::EMotionType::Kinematic, JPH::EActivation::Activate);
            }

            /*
                climb animation
                TODO : change for something else later
            */
            if(move.climb.getCurrent())
            {
                float animationTime = move.climbAnimSpeed;
                switch(move.climbType)
                {
                    case ComplexMovements::ClimbTypeEnum::High :
                    case ComplexMovements::ClimbTypeEnum::Platform :
                        animationTime *= 2.5;
                        break;
                    default : break;
                }

                float a = linearstep(0.f, animationTime, move.climb.timeSinceGoalMet());
                JoltVulpine::jPhysicsSystem->GetBodyInterface().SetPosition(
                    body, 
                    Vvec3(mix(move.animationInitialPosition, move.closestSurface, a)), 
                    JPH::EActivation::Activate
                );

                if(a >= 1.f)
                {
                    move.climb.overrideCurrent(false);
                    JoltVulpine::jPhysicsSystem->GetBodyInterface().SetMotionType(body, JPH::EMotionType::Dynamic, JPH::EActivation::Activate);
                }

                return;
            }


            /*
                Testing for closest edge
            */
            class ShapeCollector : public JPH::CastShapeCollector
            {
                public :
                vec3 dir2D;
                vec3 playerPos;
                vec3 farestPoint;
                float highestDistance = 0.f;
                ComplexMovements *move;
                JPH::BodyID playerBody;

                virtual void AddHit(const ResultType &inResult) override
                {
                    if(playerBody == inResult.mBodyID2) return;

                    JPH::Body *bodyptr = nullptr;
                    while (!bodyptr) 
                        bodyptr = JoltVulpine::jPhysicsSystem->GetBodyLockInterface().TryGetBody(inResult.mBodyID2);
                    
                    Vvec3 point = Vvec3(inResult.mContactPointOn2) + dir2D*0.25f + vec3(0, 0.0, 0);
                    Vvec3 normal = bodyptr->GetWorldSpaceSurfaceNormal(inResult.mSubShapeID2, point);

                    if(normal.y > 0.75)
                    {
                        float d = distance(playerPos, point);
                        // if(d > highestDistance)
                        if(point.y > farestPoint.y)
                        {
                            highestDistance = d;
                            farestPoint = point;
                        }
                    }
                };
            };

            ShapeCollector collector;
            vec3 dir2D = normalize(depl.look.current*vec3(1, 0, 1));
            collector.dir2D = dir2D;
            collector.playerPos = state.position;
            collector.farestPoint = state.position;
            collector.move = &move;
            collector.playerBody = body;
            Vvec3 direction = Vvec3(0, -3, 0);

            Vvec3 extents = Vvec3(1.0, 0.25, 0.25);
            static auto box = JPH::RotatedTranslatedShapeSettings(
                Vvec3(-extents*vec3(1, 1, 0)),

                JPH::QuatArg::sIdentity(), 
                JPH::BoxShapeSettings(extents).Create().Get()

                // Vquat(glm::eulerAngleXYZ(0.f, 0.f, radians(90.f))),
                // JPH::CapsuleShapeSettings(extents.x, extents.z).Create().Get()
            ).Create().Get();

            Vvec3 pos = state.position + dir2D*extents.x;
            ModelState3D s;
            s.setPosition(pos - direction + dir2D*0.25f)
             .setRotation(vec3(0, -atan2(dir2D.z, dir2D.x), 0))
             .forceUpdate();

            Jmat4 origin = JPH::Mat44(Vvec4(s.modelMatrix[0]), Vvec4(s.modelMatrix[1]), Vvec4(s.modelMatrix[2]), Vvec4(s.modelMatrix[3]));
            
            JPH::RShapeCast cast(
                // interface.GetShape(body),
                box,
                Vvec3(1.0, 1, 1.0),
                origin,
                direction
            );

            JPH::ShapeCastSettings settings;
            settings.SetBackFaceMode(JPH::EBackFaceMode::IgnoreBackFaces);

            /// Comment from Jolt source code VVVVV
            // Indicates if we want to shrink the shape by the convex radius and then expand it again. 
            // This speeds up collision detection and gives a more accurate normal at the cost of a 
            // more 'rounded' shape.
            settings.mUseShrunkenShapeAndConvexRadius = true;

            JoltVulpine::EnvironementCollideFilter layerFilter;
            JoltVulpine::BodyExcludeFilter bodyFilter(body);

            JoltVulpine::jPhysicsSystem->GetNarrowPhaseQuery().CastShape(
                cast, 
                settings, 
                Vvec3(0), //RVec3Arg inBaseOffset, 
                collector,
                {}, layerFilter, bodyFilter
            );

            /* Checking if path between edge & player is unobstructed */
            if(collector.farestPoint.y > state.position.y)
            {
                auto aabb = JoltVulpine::jPhysicsSystem->GetBodyInterface().GetShape(body)->GetWorldSpaceBounds(Vmat4(1), Vvec3(1));
                float r = aabb.GetSize().GetX()*0.125f;
                float h = aabb.GetSize().GetY();
                float d = distance(state.position*vec3(1, 0, 1), collector.farestPoint*vec3(1, 0, 1));

                static auto box2 = JPH::RotatedTranslatedShapeSettings(
                    // Vvec3(vec3(-r-d/2.f, h/2.f, 0.f)),
                    Vvec3(0),
                    JPH::Quat::sIdentity(),
                    JPH::BoxShapeSettings(Vvec3(0.5f*vec3(d + 4.f*r, h, r))).Create().Get()
                ).Create().Get();

                ModelState3D s2;
                s2  .setPosition(collector.farestPoint - dir2D*(r+d/2.f) + vec3(0,  h/2.f + 0.25f, 0))
                    .setRotation(vec3(0, -atan2(dir2D.z, dir2D.x), 0))
                    .forceUpdate();
                Jmat4 t = JPH::Mat44(Vvec4(s2.modelMatrix[0]), Vvec4(s2.modelMatrix[1]), Vvec4(s2.modelMatrix[2]), Vvec4(s2.modelMatrix[3]));

                // GG::draw->drawBox(
                //     Vvec3(box2->GetWorldSpaceBounds(t, Vvec3(1)).mMin), 
                //     Vvec3(box2->GetWorldSpaceBounds(t, Vvec3(1)).mMax)
                //     , 0.01f, 
                //     // s2, 
                //     ModelState3D(),
                //     vec3(0, 1, 1)
                // );
                
                JPH::CollideShapeSettings settings2;

                class AnyCollide : public JPH::CollideShapeCollector
                {
                    public :
                    bool anyHit = false;
                    virtual void AddHit(const ResultType &inResult) override 
                    {
                        anyHit = true;

                        // GG::draw->drawBox(
                        //     Vvec3(inResult.mContactPointOn2)-0.25f,
                        //     Vvec3(inResult.mContactPointOn2)+0.25f
                        // );
                    };
                };

                AnyCollide collector2;
                JoltVulpine::jPhysicsSystem->GetNarrowPhaseQuery().CollideShape(
                    box2, Vvec3(1), t, settings2, Vvec3(0), collector2, {}, layerFilter, bodyFilter
                );

                if(collector2.anyHit)
                    move.closestSurface = state.position;
                else
                    move.closestSurface = collector.farestPoint;

                // NOTIF_MESSAGE(PRINTVAR(r), PRINTVAR(h), PRINTVAR(d), PRINTVAR(collector2.anyHit))
            }
            else
            {
                move.closestSurface = state.position;
            }

            /*
                Testing for closest wall
            */
            Vvec3 right = cross(dir2D, vec3(0, 1, 0));
            Vvec3 diag  = normalize(right+dir2D);
            Vvec3 directions[] = 
            {
                dir2D, right, -dir2D, -right,
                diag, diag*Vvec3(-1, 0, 1), diag*Vvec3(1, 0, -1), diag*Vvec3(-1, 0, -1)  
            };
            Vvec3 closestWall = state.position;
            Vvec3 closestEdge = move.closestSurface;
            float closestWallDistance = 1e6;
            // bool frontFacing = true;

            for(auto &i : directions)
            {
                JPH::RRayCast ray;
                JPH::RayCastResult result;

                ray.mOrigin = Vvec3(state.position + vec3(0, 1, 0));
                ray.mDirection = Vvec3(i*1.5f);

                bool hit = JoltVulpine::jPhysicsSystem->GetNarrowPhaseQuery().CastRay(
                    ray, 
                    result,
                    JPH::BroadPhaseLayerFilter(),
                    JoltVulpine::EnvironementCollideFilter(),
                    JoltVulpine::BodyExcludeFilter(body)
                );

                if(hit && !result.mBodyID.IsInvalid())
                {
                    JPH::Body *bodyptr = nullptr;
                    while (!bodyptr) 
                        bodyptr = JoltVulpine::jPhysicsSystem->GetBodyLockInterface().TryGetBody(result.mBodyID);
                    
                    const Vvec3 normal = bodyptr->GetWorldSpaceSurfaceNormal(result.mSubShapeID2, ray.GetPointOnRay(result.mFraction));

                    // WARNING_MESSAGE(normal.y)

                    if(normal.y < 0.70 && result.mFraction < closestWallDistance)
                    {
                        closestWallDistance = result.mFraction;
                        closestWall = ray.mOrigin + result.mFraction*ray.mDirection;

                        // if(frontFacing) closestEdge = closestWall;
                    }
                }

                // frontFacing = false;
            }

            /*
                Determining precise climbing positions, normal and type
            */
            move.closestWall = state.position;

            Vvec3 edgeNormal = vec3(0);
            float cnt = 0.f;
            float plateformThickness = 0.f;

            if(move.closestSurface != state.position)
            {
                const uint wallEdgeSteps = 8;
                const float wallEdgeMaxOffset = 0.5f;
                for(uint i = 0; i < wallEdgeSteps; i++)
                {
                    float a = wallEdgeMaxOffset*(1.f - (float)i/(float)(wallEdgeSteps-1));
                    JPH::RRayCast ray;
                    JPH::RayCastResult result;
    
                    ray.mOrigin = Vvec3(move.closestSurface - dir2D - vec3(0, a, 0));
                    ray.mDirection = Vvec3(dir2D);

                    bool hit = JoltVulpine::jPhysicsSystem->GetNarrowPhaseQuery().CastRay(
                        ray, 
                        result,
                        JPH::BroadPhaseLayerFilter(),
                        JoltVulpine::EnvironementCollideFilter(),
                        JoltVulpine::BodyExcludeFilter(body)
                    );

                    if(hit && !result.mBodyID.IsInvalid())
                    {
                        JPH::Body *bodyptr = nullptr;
                        while (!bodyptr) 
                            bodyptr = JoltVulpine::jPhysicsSystem->GetBodyLockInterface().TryGetBody(result.mBodyID);

                        const Vvec3 normal = bodyptr->GetWorldSpaceSurfaceNormal(result.mSubShapeID2, ray.GetPointOnRay(result.mFraction));
                        const Vvec3 position = ray.mOrigin + result.mFraction*ray.mDirection;

                        plateformThickness = max(plateformThickness, move.closestSurface.y - position.y);

                        if(normal.y < cos(radians(35.f)))
                        {
                            const Vvec3 color = hsv2rgb(vec3(0.1 + a*0.5, 1, 1));
                            // GG::draw->drawSphere(position, 0.05, 0.1f, ModelState3D(), color);
                            // GG::draw->drawLine(position, position+normal*0.4f, 0.1f, ModelState3D(), color);

                            closestEdge = position;
                            cnt ++;
                            edgeNormal = edgeNormal + normal;
                        }

                        if(!i and cnt == 0.f)
                        {
                            closestEdge = position;
                            edgeNormal = normal;
                            cnt ++;
                        }
                    }
                }

                move.closestEdge = closestEdge;
                float edgeHeight = distance(move.closestSurface.y, state.position.y);
                if(depl.grounded.get() and edgeHeight <= 1.5f)
                {
                    move.climbType = ComplexMovements::ClimbTypeEnum::Low;
                    move.closestEdgeNormal = normalize(vec3(1,0,1)*(state.position-move.closestEdge));
                }
                else if(plateformThickness >= 0.4f)
                {
                    move.climbType = ComplexMovements::ClimbTypeEnum::High;
                    move.closestEdgeNormal = cnt > 0.f ? normalize(edgeNormal/cnt) : normalize(vec3(1,0,1)*(state.position-move.closestEdge));
                }
                else
                {
                    move.climbType = ComplexMovements::ClimbTypeEnum::Platform;
                    move.closestEdgeNormal = cnt > 0.f ? normalize(edgeNormal/cnt) : normalize(vec3(1,0,1)*(state.position-move.closestEdge));
                }
            }


            // NOTIF_MESSAGE(closestWall)

            // static auto cylinder = JPH::CylinderShapeSettings(0.5, 2.f).Create().Get();
            // JPH::CollideShapeSettings settings2;
            // settings2.mPenetrationTolerance = 0.f;
            // settings2.mCollisionTolerance = 2.f;
            
            // class ClosestWallCollide : public JPH::CollideShapeCollector
            // {
            //     public :
            //     bool anyHit = false;
            //     vec3 closestHit = vec3(0);
            //     vec3 playerPos = vec3(0);
            //     virtual void AddHit(const ResultType &inResult) override 
            //     {
            //         anyHit = true;

            //         GG::draw->drawSphere(
            //             Vvec3(inResult.mContactPointOn2)-0.25f,
            //             0.25f, 0.1f
            //         );

            //         NOTIF_MESSAGE(inResult.mContactPointOn2)
            //     };
            // };

            // ClosestWallCollide collector2;
            // collector2.closestHit = state.position;
            // collector2.playerPos = state.position;      

            // Jmat4 t;
            // t.SetTranslation(Vvec3(state.position));

            // GG::draw->drawBox(
            //     Vvec3(cylinder->GetWorldSpaceBounds(t, Vvec3(1)).mMin), 
            //     Vvec3(cylinder->GetWorldSpaceBounds(t, Vvec3(1)).mMax)
            //     , 0.01f, 
            //     // s2, 
            //     ModelState3D(),
            //     vec3(0, 1, 1)
            // );

            // JoltVulpine::jPhysicsSystem->GetNarrowPhaseQuery().CollideShape(
            //     cylinder, Vvec3(1), 
            //     t, 
            //     settings2, 
            //     Vvec3(0), 
            //     collector2, 
            //     {}, layerFilter, bodyFilter
            // );
        });

        System<Movement, JoltBody>([&](Entity &e, Movement &depl, JoltBody &body)
        {
            if(!body.IsInvalid())
            {
                Vvec3 vel = JoltVulpine::jPhysicsSystem->GetBodyInterface().GetLinearVelocity(body);
                
                auto &depl = e.comp<Movement>();

                depl.speed.current = glm::length(vec3(vel));

                if(depl.speed.current > 0.01f)
                {
                    depl.direction.current = normalize(vel);
                }
                else
                {
                    depl.direction.current = vec3(0, 0, 0);
                }

                JPH::RRayCast ray;
                
                ray.mOrigin = 
                    e.has<State3D>() ? 
                    Vvec3(e.comp<State3D>().position) : 
                    Vvec3(JoltVulpine::jPhysicsSystem->GetBodyInterface().GetPosition(body))
                ;

                float offset = 0.25;
                ray.mOrigin += Vvec3(0, offset, 0);

                ray.mDirection = Vvec3(0, -1, 0);

                JPH::RayCastResult result;

                bool hit = JoltVulpine::jPhysicsSystem->GetNarrowPhaseQuery().CastRay(
                    ray, 
                    result,
                    JPH::BroadPhaseLayerFilter(),
                    JoltVulpine::EnvironementCollideFilter(),
                    JoltVulpine::BodyExcludeFilter(body)
                );

                float staminaSlowness = 1.f;

                float depDotNormal = 0.f;
                float slopeDir = 0.f;

                // depl.grounded.set(true);

                if(!hit || result.mBodyID.IsInvalid())
                {
                    depl.grounded.set(false);
                    // ERROR_MESSAGE("NO GROUND ", JoltVulpine::physicsTimer.getUpdateCounter())
                }
                else
                {
                    JPH::Body *bodyptr = nullptr;
                    while (!bodyptr) 
                        bodyptr = JoltVulpine::jPhysicsSystem->GetBodyLockInterface().TryGetBody(result.mBodyID);
                    
                    const Vvec3 normal = bodyptr->GetWorldSpaceSurfaceNormal(result.mSubShapeID2, ray.GetPointOnRay(result.mFraction));
                    slopeDir = normal.y;

                    const float tresholdAirborn = 0.60;
                    const float tresholdSlowed = 0.99;

                    depl.grounded.set(result.mFraction < offset+0.25f);

                    if(e.has<Gauges>())
                    {
                        staminaSlowness = smoothstep(0.f, 0.1f, e.comp<Gauges>().stamina.getFraction());
                    }

                    constexpr float bias = 1e-4;
                    if((abs(normal.x) > bias or abs(normal.z) > bias) and (abs(depl.direction.current.x) > bias or abs(depl.direction.current.z) > bias))
                    {
                        vec2 normal2D = normalize(vec2(normal.x, normal.z));
                        vec2 depl2D = normalize(vec2(depl.direction.current.x, depl.direction.current.z));
                        depDotNormal = dot(normal2D, depl2D);
                    }

                    // NOTIF_MESSAGE(
                    //     PRINTVAR(tresholdAirborn),
                    //     PRINTVAR(tresholdSlowed),
                    //     PRINTVAR(slopeSlowness),
                    //     PRINTVAR(normal.y)
                    // )
                }

                auto &interface = JoltVulpine::jPhysicsSystem->GetBodyInterface();

                float speed = mix(depl.walkSpeed, depl.speed.goal, staminaSlowness);

                bool inJump = e.has<ComplexMovements>() and e.comp<ComplexMovements>().jump.getCurrent();

                // NOTIF_MESSAGE(PRINTVAR(e.has<ComplexMovements>()), PRINTVAR(e.comp<ComplexMovements>().jump.getCurrent()))

                if(depl.grounded.get() and not inJump)
                {
                    if(JoltVulpine::useVelocityBasedMovement)
                    {   
                        vec3 dir = depl.direction.goal;

                        // Prevent "bouncing" when descending on a slope
                        dir.y = -smoothstep(0.98f, 0.95f, slopeDir)*max(depDotNormal, 0.f);

                        interface.SetLinearVelocity(body, Vvec3(dir*speed));
                        interface.SetMaxLinearVelocity(body, speed*length(depl.direction.goal));

                        // NOTIF_MESSAGE(dir)
                    }
                    else
                    {
                        interface.AddForce(body, Vvec3(accSpeedFactor * depl.direction.goal * 1.f * 5e5f * smoothstep(0.f, 0.5f, depl.grounded.timeSinceChange())));
                        interface.SetMaxLinearVelocity(body, speed);
                    }
                }
                else
                {
                    // Vvec3 dir = depl.direction.goal*depl.walkSpeed;
                    // dir.y = depl.direction.current.y;
                    // interface.SetLinearVelocity(body, dir);

                    interface.AddForce(body, Vvec3(accSpeedFactor * depl.direction.goal * 1e6f * smoothstep(depl.walkSpeed, 0.f, depl.speed.current)));

                    interface.SetMaxLinearVelocity(body, 1e3f);
                }

                interface.SetGravityFactor(body, 3.f);
            }
        });

        JoltVulpine::physicsSystemsTimer.stop();
        JoltVulpine::physicsTimer.stop();

        float maxFreq = 60.f;
        float minFreq = 7.5f;

        if(JoltVulpine::physicsTimer.getDelta() > 1.0/JoltVulpine::physicsTicks.freq)
        {
            JoltVulpine::physicsTicks.freq = clamp(JoltVulpine::physicsTicks.freq/2.f, minFreq, maxFreq);
        }
        else if(JoltVulpine::physicsTimer.getDelta() < 0.5f/JoltVulpine::physicsTicks.freq)
        {
            JoltVulpine::physicsTicks.freq = clamp(JoltVulpine::physicsTicks.freq*2.f, minFreq, maxFreq);
        }

        JoltVulpine::physicsMutex.unlock();
        JoltVulpine::physicsTicks.waitForEnd();
    }
    
    for(auto &i : Loader<ScriptInstance>::loadedAssets)
        if(i.second.lua_state() == threadState)
            i.second = ScriptInstance();
    
    JPH::UnregisterTypes();
    delete JPH::Factory::sInstance;
    JPH::Factory::sInstance = nullptr;

    delete JoltVulpine::jPhysicsSystem;
    JoltVulpine::jPhysicsSystem = nullptr;

    delete JoltVulpine::debugRenderer;
    JoltVulpine::debugRenderer = nullptr;
}
