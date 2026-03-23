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

// generate a timestamp for logging
#define TIMESTAMP_PHYSICS "[", globals.simulationTime.getElapsedTime(), "] "

// Draw the final raycast against the world for climbing as well as a sphere if there is a valid hit
#define CLIMB_DEBUG_DRAW_LINE 0
// Draw the world quantization boxes that detect if a specific box has a collider inside
#define CLIMB_DEBUG_DRAW_BOX 0
// Draw the debug info for stepping (the boxes and ray)
#define STEP_DEBUG_DRAW 0

// callback that is called for the box overlap tests for climbing and stepping.
// mostly just checks that whatever we collided with has userdata and doesn't have a MovementState component (to avoid colliding with characters and stuff)
struct ClimbOverlapCallback : public rp3d::OverlapCallback
{
    bool hit = false;

    void onOverlap(CallbackData &callbackData) override
    {
        size_t pairs = callbackData.getNbOverlappingPairs();
        for (size_t i = 0; i < pairs; i++)
        {
            rp3d::OverlapCallback::OverlapPair pair = callbackData.getOverlappingPair(i);
            rp3d::Collider *c1 = pair.getCollider1();
            rp3d::Collider *c2 = pair.getCollider2();

            Entity *other = (Entity*)c2->getUserData();

            if (
                    other == nullptr
                || other->has<MovementState>()
            ) continue;

            hit = true;
            return;
        }
    }
};

// Callback that is called for the final raycast test for climbing and stepping.
// like above this one filters stuff that doesn't have any entities attached or a movement state,
// but also filters the terrain colliders since their raycast is broken for some reason 
// inherits from the base vulpine raycast callback class that provides info such as hit point, hit distance and normal
class ClimbRaycastCallback : public PG::RayCastCallback
{
private:
    rp3d::decimal notifyRaycastHit(const rp3d::RaycastInfo& info) override
    {
// #if CLIMB_DEBUG_DRAW_LINE
//         if (GG::draw != nullptr)
//             GG::draw->drawSphere(PG::toglm(info.worldPoint), 0.05f, 0.0f, ModelState3D(), "#00ffff"_rgb);
// #endif

        // DEBUG_MESSAGE(TIMESTAMP_PHYSICS, "hit! ", PG::toglm(info.worldPoint));

        Entity* e = (Entity*)info.body->getUserData();
        if (
            (e == nullptr) ||
            (e->has<MovementState>()) ||
            (e->has<HeightFieldDummyFlag>()) 
        )  
        {
            return -1.0;
        }

        return PG::RayCastCallback::notifyRaycastHit(info);
    }
public:
    ClimbRaycastCallback(vec3 p1, vec3 p2) : PG::RayCastCallback(p1, p2) {};
};

// The function that does all the work for testing if we can climb or step.
// Very roughly, it uses box querries to get a rough point where there is no terrain and then uses a raycast to find the final point.
// This function assumes that GG::playerEntity is a valid entity pointer.
void TryPlayerClimb()
{
    // since this depends on the player's position and body we return if it doesn't have the proper components
    if(!GG::playerEntity->has<state3D>() or !GG::playerEntity->has<RigidBody>() or !GG::playerEntity->has<MovementState>())
        return;

    // get the player components
    state3D& s = GG::playerEntity->comp<state3D>();
    rp3d::RigidBody* playerBody = GG::playerEntity->comp<RigidBody>();
    MovementState& ms = GG::playerEntity->comp<MovementState>();

    // this flag is usually true when we're already climbing but could also be true if we're in an animation for instance.
    if (!ms.isAllowedToClimb) return;

    // this is the flag that communicates to the rest of the code if the climb test was valid or not, 
    // we try to find if we can find a valid climb spot so it starts out false
    ms.canClimb = false;

    // === climbing constants ===
    // almost every single one of those is eyeballed

    // the max distance to check for the climbing check relative to the player's position
    const float distanceCheck = 1.5;

    // when we find a hit, we want to move the point a little forward so we're not right on a ledge and fall right back down.
    // this float is that offset in world units
    const float offset_in = 0.5;

    // the max height we can climb
    const float testHeightMax = 2.5;
    // the min height we can climb
    const float testHeightMin = 0.75;
    // the height of the player's collider
    const float playerHeight = 2.0f;

    // resolution of the box querries, they're quite expensive because of the engine we're using so we can't go crazy with the resolution.
    // we only run height tests if we got a hit when doing a box the height of the total climbing volume for the depth
    constexpr int N_DEPTH = 8;
    constexpr int N_HEIGHT = 8;

    const float epsilon = 0.01;

    // half the height of the total climbing volume, used for computing the extents of boxes that are the height of the climbing volume
    const float halfHeight = (testHeightMax - testHeightMin) / 2;

    // the body that will be used to do the queries.
    // we set it to kinematic but it actually doesn't matter since it will be deleted at the end
    rp3d::RigidBody* testBody = PG::world->createRigidBody(rp3d::Transform(rp3d::Vector3(.001, .001, .001), DEFQUAT));
    testBody->setType(rp3d::BodyType::KINEMATIC);

    // first we test for obstructions to climbing. Basically just if we can fit the player right above the climbing volume, 
    // since we test the queries later from the top-down this allows us to quickly determine if a climb is blocked by a ceiling.
    // we also test if there is a ceiling just above the player, we don't want them phasing through the terrain

    // extent 1 is for testing if there's a ceiling above the player that blocks climbing
    vec3 ObstructionBoxHalfExtents_1 = vec3(0.25f, halfHeight, .25);
    // extent 2 is for testing if there's a ceiling blocking the climbing test, 
    // this isn't ideal since that means we can't climb a small ledge if we're too close to the wall but it's probably fine idk
    vec3 ObstructionBoxHalfExtents_2 = vec3(0.25f, playerHeight / 2.0f, distanceCheck / 2.0f);
    static rp3d::BoxShape* ObstructionBox_1 = PG::common.createBoxShape(PG::torp3d(ObstructionBoxHalfExtents_1));
    static rp3d::BoxShape* ObstructionBox_2 = PG::common.createBoxShape(PG::torp3d(ObstructionBoxHalfExtents_2));
    rp3d::Transform obstructionTransform_1(rp3d::Vector3(0, playerHeight + halfHeight, 0), DEFQUAT);
    rp3d::Transform obstructionTransform_2(rp3d::Vector3(0, testHeightMax + halfHeight, -distanceCheck / 2), DEFQUAT);
    rp3d::Collider *obstructionCollider_1 = testBody->addCollider(ObstructionBox_1, obstructionTransform_1);
    rp3d::Collider *obstructionCollider_2 = testBody->addCollider(ObstructionBox_2, obstructionTransform_2);
    // we explicitely say that the colliders aren't to be simulated cause otherwise it can fuck with the physics even if we delete them later
    obstructionCollider_1->setIsSimulationCollider(false);
    obstructionCollider_2->setIsSimulationCollider(false);
    obstructionCollider_1->setCollisionCategoryBits(1<<CollideCategory::ENVIRONEMENT);
    obstructionCollider_1->setCollideWithMaskBits(1<<CollideCategory::ENVIRONEMENT);
    obstructionCollider_2->setCollisionCategoryBits(1<<CollideCategory::ENVIRONEMENT);
    obstructionCollider_2->setCollideWithMaskBits(1<<CollideCategory::ENVIRONEMENT);

    testBody->setTransform(playerBody->getTransform());

#if CLIMB_DEBUG_DRAW_BOX
    if (GG::draw != nullptr and playerBody)
    {
        GG::draw->drawBox(ObstructionBox_1, playerBody->getTransform() * obstructionTransform_1, 0.0f, ModelState3D(), "#ffffff"_rgb);
        GG::draw->drawBox(ObstructionBox_2, playerBody->getTransform() * obstructionTransform_2, 0.0f, ModelState3D(), "#ffffff"_rgb);
    }
#endif
    
    ClimbOverlapCallback testObstruction;
    PG::world->testOverlap(testBody, testObstruction);

    // if we're obstructed we just exit
    if (testObstruction.hit) 
    {
        PG::world->destroyRigidBody(testBody);
        return;
    }
    
    testBody->removeCollider(obstructionCollider_1);
    testBody->removeCollider(obstructionCollider_2);

    // now we define the extents for the box queries
    // this one has the shape of (width of player, total height of the query volume, size of the depth test box)
    vec3 halfExtents = vec3(.25f, halfHeight, distanceCheck / N_DEPTH);
    // this one has the shape of (width of player, player height, player width)
    vec3 halfPlayerBoxExtents = vec3(.25, playerHeight / 2, .25);

    static rp3d::BoxShape* playerbox = PG::common.createBoxShape(PG::torp3d(halfPlayerBoxExtents));
    static rp3d::BoxShape* box = PG::common.createBoxShape(PG::torp3d(halfExtents));

    
    rp3d::Collider* depthCollider = nullptr;
    for (int i = N_DEPTH - 1; i > 0 and playerBody; i--)
    {
        depthCollider = testBody->addCollider(box, rp3d::Transform());
        depthCollider->setIsSimulationCollider(false);
        depthCollider->setCollisionCategoryBits(1<<CollideCategory::ENVIRONEMENT);
        depthCollider->setCollideWithMaskBits(1<<CollideCategory::ENVIRONEMENT);

        const float step_size_depth = distanceCheck / N_DEPTH;
        vec3 offset = vec3(0, halfHeight + testHeightMin + epsilon, -step_size_depth * i);
        rp3d::Transform transformOffset(PG::torp3d(offset), DEFQUAT);
        rp3d::Transform transformWorld = playerBody->getTransform() * transformOffset ;
        testBody->setTransform(transformWorld);

#if CLIMB_DEBUG_DRAW_BOX
        if (GG::draw != nullptr)
        {
            GG::draw->drawBox(box, transformWorld);
        }
#endif
        
        ClimbOverlapCallback test;
        PG::world->testOverlap(testBody, test);

        bool found = false;
        testBody->removeCollider(depthCollider);
        depthCollider = nullptr;
        if (!test.hit) continue;

        // === Climb Test ===
        
        rp3d::Collider* playerHeightCollider = testBody->addCollider(playerbox, rp3d::Transform());
        playerHeightCollider->setIsSimulationCollider(false);
        playerHeightCollider->setCollisionCategoryBits(1<<CollideCategory::ENVIRONEMENT);
        playerHeightCollider->setCollideWithMaskBits(1<<CollideCategory::ENVIRONEMENT);

        for (int j = 0; j < N_HEIGHT; j++)
        {
            // iterate top-down
            vec3 offset2 = vec3(0, mix(testHeightMax, 0.0f, (float)j / (float)(N_HEIGHT - 1)) + playerHeight / 2.0f, 0);

            rp3d::Transform transformOffsetHeight(PG::torp3d(offset * vec3(1, 0, 1) + offset2), DEFQUAT);
            rp3d::Transform transformWorldHeight = playerBody->getTransform() * transformOffsetHeight;
            testBody->setTransform(transformWorldHeight);

#if CLIMB_DEBUG_DRAW_BOX
            if (GG::draw != nullptr)
            {
                GG::draw->drawBox(playerbox, transformWorldHeight, 0.0f, ModelState3D(), "#0000ff"_rgb);
            }
#endif

            ClimbOverlapCallback testHeight;
            PG::world->testOverlap(testBody, testHeight);

            if (!testHeight.hit) continue;

// #if CLIMB_DEBUG_DRAW_LINE
//             if (GG::draw != nullptr)
//             {
//                 GG::draw->drawSphere(PG::toglm(transformWorldHeight.getPosition()), 0.05f, 0.0f, ModelState3D(), "#ff00ff"_rgb);
//             }
// #endif
            // the actual world point we want to raycast at
            rp3d::Transform pointTransform = transformWorldHeight * rp3d::Transform(
                rp3d::Vector3(
                    0, 
                    // -1.0f + testHeightMax / N_HEIGHT, 
                    // -offset_in

                     -1.0f + testHeightMax / N_HEIGHT,
                     -offset_in

                ), DEFQUAT);
            vec3 newPlayerPos = PG::toglm(pointTransform.getPosition());

            // define the ray that will determine the final climb position and if it's a valid position
            vec3 p1 = newPlayerPos + vec3(0, 2.0, 0);
            vec3 p2 = newPlayerPos - vec3(0, 0.25, 0);
            rp3d::Vector3 _p1 = PG::torp3d(p1);
            rp3d::Vector3 _p2 = PG::torp3d(p2);
            // ray against the world but not the terrain
            ClimbRaycastCallback cb(p1, p2);
            rp3d::Ray ray(_p1, _p2);
            PG::world->raycast(ray, &cb, 1<<CollideCategory::ENVIRONEMENT);

            // fix the raycast not working against the terrain coliders
            bool terrainHit = false;
            rp3d::RaycastInfo info;
            vec3 TerrainNormal = vec3(0, -1, 0);
            System<HeightFieldDummyFlag>([&](Entity &entity)
            {
                if (terrainHit) return;

                RigidBody b = entity.comp<RigidBody>();
                
                // define x and z offset rays to compute the partial derivative of the position to get the normal
                const float offset = 0.05f;
                rp3d::Vector3 _p1_dx = _p1 + rp3d::Vector3(offset, 0, 0);
                rp3d::Vector3 _p2_dx = _p2 + rp3d::Vector3(offset, 0, 0);

                rp3d::Vector3 _p1_dy = _p1 + rp3d::Vector3(0, 0, offset);
                rp3d::Vector3 _p2_dy = _p2 + rp3d::Vector3(0, 0, offset);

                const size_t n = b->getNbColliders();
                for (size_t i = 0; i < n; i++)
                {
                    Collider c = b->getCollider(i);

                    rp3d::Ray ray(_p1, _p2);
                    rp3d::Ray ray_dx(_p1_dx, _p2_dx);
                    rp3d::Ray ray_dy(_p1_dy, _p2_dy);

                    bool _hit = c->raycast(ray, info);
                    rp3d::RaycastInfo info_dx, info_dy;
                    bool hit_dx = c->raycast(ray_dx, info_dx);
                    bool hit_dy = c->raycast(ray_dy, info_dy);

                    // all 3 rays need to hit
                    if (_hit && hit_dx && hit_dy)
                    {
                        terrainHit = true;
                        vec3 p    = PG::toglm(   info.worldPoint);
                        vec3 p_dx = PG::toglm(info_dx.worldPoint);
                        vec3 p_dy = PG::toglm(info_dy.worldPoint);

                        // compute the normal
                        vec3 n = cross(p - p_dy, p - p_dx);
                        if (length2(n) != 0.0)
                            TerrainNormal = normalize(n);

                        // DEBUG_MESSAGE("normal: ", querryNormal);
                        break;
                    } 
                }
            });

            // try to get the final hit info based on the terrain raycast and world raycast
            vec3 hitPoint = vec3(0);
            vec3 hitNormal = vec3(0, -1, 0);
            bool hit = terrainHit || cb.hit;
            if (terrainHit && cb.hit)
            {
                if (cb.hitPoint.y > info.worldPoint.y)
                {
                    hitPoint = cb.hitPoint;
                    hitNormal = cb.hitNormal;
                }
                else 
                {
                    hitPoint = PG::toglm(info.worldPoint);
                    hitNormal = TerrainNormal;
                }
            }
            else if (terrainHit)
            {
                hitPoint = PG::toglm(info.worldPoint);
                hitNormal = TerrainNormal;
            }
            else if (cb.hit)
            {
                hitPoint = cb.hitPoint;
                hitNormal = cb.hitNormal;
            }

            // update the final position with the hit point, offset a tiny bit upward to avoid clipping
            newPlayerPos.y = hitPoint.y + 0.1f;

#if CLIMB_DEBUG_DRAW_LINE
            if (GG::draw != nullptr)
            {
                GG::draw->drawLine(p1, p2);
            }
#endif

            // to avoid false positive, especially when trying to climb lower surfaces, we can have a normal limit that varies based on the height of the new position.
            
            // the normal limit for when the height is low
            const float normalDotLow = 0.99;
            // the normal limit for when the height is high
            const float normalDotHigh = 0.7;

            // we also add a little offset so it's starts a little higher
            const float heightOffset = 0.1f;

            float playerHeight = s.position.y + heightOffset;
            float height = newPlayerPos.y - playerHeight;

            const float normalHeightInterpolateMin = testHeightMin;
            const float normalHeightInterpolateMax = testHeightMax;

            float t = height / (normalHeightInterpolateMax - normalHeightInterpolateMin);
            float normalDotMax = mix(normalDotLow, normalDotHigh, t); 

            float boundsMissEpsilon = 0.1;

            // fail raycast if: 
            if (
                   !hit // raycast missed
                || hitNormal.y < normalDotMax // normal of hit is too steep
                || hitPoint.y < transformWorldHeight.getPosition().y - playerbox->getHalfExtents().y - boundsMissEpsilon // hit is outside the bounds of the test (essentially also a miss)
            ) break;

            found = true;
#if CLIMB_DEBUG_DRAW_LINE
            if (GG::draw != nullptr)
            {
                GG::draw->drawSphere(hitPoint, 0.1, 0.0f, ModelState3D(), "#00ff00"_rgb);
            }
#endif
            // if we do have a valid raycast then we're good and we can just set the component values 
            ms.climbStartPos = s.position;
            ms.climbEndPos = newPlayerPos;
            
            ms.climbHeight = height;
            ms.climbStartSpeed = length(PG::toglm(playerBody->getLinearVelocity()));
            ms.canClimb = true;

            // DEBUG_MESSAGE(
            //     "ms.climbStartPos: ", ms.climbStartPos,
            //     "ms.climbEndPos: ", ms.climbEndPos,
            //     "ms.climbHeight: ", ms.climbHeight,
            //     "ms.climbStartSpeed: ", ms.climbStartSpeed
            // );

            break;
        }
        testBody->removeCollider(playerHeightCollider);

        if (found)
            break;
    }

    // === Step test ===

    // only step if we both are pressing an input and are currently moving
    if (length(ms.wantedMoveDirection) < 0.001 || length(ms.inputVector) < 0.001)
    {
        PG::world->destroyRigidBody(testBody);
        return;
    }

    // clear the colliders that might have been left dangling after the climb test
    size_t colliders_nb = testBody->getNbColliders();
    for (size_t i = 0; i < colliders_nb; i++)
    {
        testBody->removeCollider(testBody->getCollider(0));
    }

    static rp3d::BoxShape* stepBox = PG::common.createBoxShape(rp3d::Vector3(0.25, 0.15, 0.2));
    static rp3d::BoxShape* stepBoxObstruct = PG::common.createBoxShape(rp3d::Vector3(0.5, 1.0, 0.5)); // essentially a box the size of the player
    rp3d::Transform stepTransform(rp3d::Vector3(0, -0.8 + playerHeight / 2.0f, -0.25), DEFQUAT);
    rp3d::Transform stepTransformObstruct(rp3d::Vector3(0, .35 + playerHeight / 2.0f, -.4), DEFQUAT);

    // transform the direction vector into a rotation vector
    vec3 wantedRotation = vec3(0, atan(ms.inputVector.x, ms.inputVector.z), 0);
    rp3d::Transform stepRotationTransform(rp3d::Vector3(0, 0, 0), PG::torp3d(quat(wantedRotation)));

    stepTransform = stepRotationTransform * stepTransform;
    stepTransformObstruct = stepRotationTransform * stepTransformObstruct;

    rp3d::Collider* stepColliderObstruct = testBody->addCollider(stepBoxObstruct, stepTransformObstruct);
    stepColliderObstruct->setIsSimulationCollider(false);
    stepColliderObstruct->setCollisionCategoryBits(1<<CollideCategory::ENVIRONEMENT);
    stepColliderObstruct->setCollideWithMaskBits(1<<CollideCategory::ENVIRONEMENT);

    
    // testBody->setTransform(stepTransformObstructWorld);
    testBody->setTransform(playerBody->getTransform());
    
    ClimbOverlapCallback testStepObstruct;
    PG::world->testOverlap(testBody, testStepObstruct);
    
#if STEP_DEBUG_DRAW
    if (GG::draw != nullptr)
    {
        rp3d::Transform stepTransformObstructWorld = playerBody->getTransform() * stepTransformObstruct;
        GG::draw->drawBox(stepBoxObstruct, stepTransformObstructWorld, 0.0f, ModelState3D(), "#ff00ff"_rgb);
    }
#endif

    if (!testStepObstruct.hit)
    {
        rp3d::Transform stepTransformWorld = playerBody->getTransform() * stepTransform;
        // testBody->setTransform(stepTransformWorld);

        testBody->removeCollider(stepColliderObstruct);
        rp3d::Collider* stepCollider = testBody->addCollider(stepBox, stepTransform);
        stepCollider->setIsSimulationCollider(false);
        stepCollider->setCollisionCategoryBits(1<<CollideCategory::ENVIRONEMENT);
        stepCollider->setCollideWithMaskBits(1<<CollideCategory::ENVIRONEMENT);

#if STEP_DEBUG_DRAW
        if (GG::draw != nullptr)
        {
            GG::draw->drawBox(stepBox, stepTransformWorld, 0.0f, ModelState3D(), "#ff00ff"_rgb);
        }
#endif
        
        ClimbOverlapCallback testStep;
        PG::world->testOverlap(testBody, testStep);
        
        if (testStep.hit)
        {
            testBody->removeCollider(stepCollider);

            rp3d::Transform pointTransform = stepTransformWorld * rp3d::Transform(
                rp3d::Vector3(
                    0, 
                    +0.35f, 
                    -0.1
                ), DEFQUAT);
            vec3 newPlayerPos = PG::toglm(pointTransform.getPosition());

            vec3 p1 = newPlayerPos + vec3(0, 0.5, 0);
            vec3 p2 = newPlayerPos - vec3(0, 0.5, 0);
            ClimbRaycastCallback cb(p1, p2);
            rp3d::Ray ray(PG::torp3d(p1), PG::torp3d(p2));
            PG::world->raycast(ray, &cb, 1<<CollideCategory::ENVIRONEMENT);

#if STEP_DEBUG_DRAW
            if (GG::draw != nullptr)
            {
                GG::draw->drawLine(p1, p2);
            }
#endif

            // to step we need the surface to be almost completely flat
            const float normalDotMax = 0.99;

            if (cb.hit && dot(cb.hitNormal, vec3(0, 1, 0)) > normalDotMax)
            {   
#if STEP_DEBUG_DRAW
                if (GG::draw != nullptr)
                {
                    GG::draw->drawSphere(cb.hitPoint, 0.1);
                }
#endif
                // move the player directly to the step point
                playerBody->setTransform(rp3d::Transform(PG::torp3d(cb.hitPoint), DEFQUAT));
            }
        }
    }
    
    PG::world->destroyRigidBody(testBody);
}

void Game::physicsLoop()
{
    currentThreadID = 1;
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

                if(GG::playerEntity->has<PhysicsInfos>())
                    GG::playerEntity->comp<PhysicsInfos>().shoudBeActive = false;
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
        System<NonStaticBodyDummyFlag, RigidBody, state3D, PhysicsInfos>([](Entity &entity){
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

        // System<MovementState>([](Entity& entity)
        // {
        //     MovementState& ms = entity.comp<MovementState>();
        //     if (ms.walking)
        //     {
        //         if (!ms.grounded) // just landed
        //         {
        //             ms.justLanded = true;
        //             ms.landedTime = globals.simulationTime.getElapsedTime();
        //         }
                
                
        //         ms.grounded = true;
        //         ms.walking  = true;
        //     }
        //     else
        //     {
        //         ms.grounded = false;
        //         ms.walking  = false;
        //     }
        //     ms.groundNormalY = ms._groundNormalY;
        // });

        physicsSystemsTimer.start();



        System<NonStaticBodyDummyFlag, RigidBody, state3D, PhysicsInfos>([](Entity &entity){
            auto &b = entity.comp<RigidBody>();
            auto &f = entity.comp<PhysicsInfos>();

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



        
        if (GG::playerEntity != nullptr)
        {
            TryPlayerClimb();
        }
        

    /***** UPDATING RIGID BODY AND ENTITY STATE RELATIVE TO THE BODY TYPE *****/
        System<NonStaticBodyDummyFlag, RigidBody, state3D, PhysicsInfos>([](Entity &entity){
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
            vec3 velocity = PG::toglm(b->getLinearVelocity());

            float heightAboveTerrain;
            vec3 terrainNormal;
            // probably should make this into a standalone function cause it's useful and also it would free up this insanely long system
            // also if you leave the terrain the game will segfault
            {
                vec2 field_point = vec2(entity.comp<state3D>().position.x, entity.comp<state3D>().position.z);
                field_point += vec2(Blueprint::terrainSize.x, Blueprint::terrainSize.z) * 0.5f;

                Texture2D &HeightMap = Loader<Texture2D>::get(Blueprint::mapFileName);

                float *src = ((float *)HeightMap.getPixelSource());
                ivec2 textureSize = HeightMap.getResolution();

                auto texelFetch = [&src, &textureSize](const ivec2& uv){return src[uv.y * textureSize.x + uv.x] * Blueprint::terrainSize.y;};

                vec2 terrain_uv = field_point / vec2(Blueprint::terrainSize.x, Blueprint::terrainSize.z);
                vec2 texel_pos = terrain_uv * vec2(textureSize) - 0.5f;
                // bilinear interpolation
                ivec2 base = ivec2(floor(texel_pos));
                vec2 frac  = fract(texel_pos); 

                ivec2 p00 = base + ivec2(0, 0);
                ivec2 p10 = base + ivec2(1, 0);
                ivec2 p01 = base + ivec2(0, 1);
                ivec2 p11 = base + ivec2(1, 1);

                float c00 = texelFetch(p00);
                float c10 = texelFetch(p10);
                float c01 = texelFetch(p01);
                float c11 = texelFetch(p11);

                float wx = frac.x;
                float wy = frac.y;

                float h =
                    c00 * (1 - wx) * (1 - wy) +
                    c10 * wx       * (1 - wy) +
                    c01 * (1 - wx) * wy +
                    c11 * wx       * wy;

                heightAboveTerrain = entity.comp<state3D>().position.y - h;

                float dhdx = c00 - c10;
                float dhdy = c00 - c01;

                terrainNormal = normalize(vec3(dhdx, 1.0, dhdy));

                // DEBUG_MESSAGE("terrainNormal: ", terrainNormal);

                // vec3 p = vec3(entity.comp<state3D>().position.x, h, entity.comp<state3D>().position.z);
                // GG::draw->drawLine(p, p + terrainNormal, 10.0f);
            }
            
            vec3 p1 = entity.comp<state3D>().position + vec3(0, 0.05, 0);
            vec3 p2 = entity.comp<state3D>().position - vec3(0, 0.20, 0);
            rp3d::Vector3 _p1 = PG::torp3d(p1);
            rp3d::Vector3 _p2 = PG::torp3d(p2);
            // ray against the world but not the terrain
            ClimbRaycastCallback cb(p1, p2);
            rp3d::Ray ray(_p1, _p2);
            PG::world->raycast(ray, &cb, 1<<CollideCategory::ENVIRONEMENT);

            float floordist = abs(heightAboveTerrain);
            vec3 floorNormal = terrainNormal;
            if (cb.hit && cb.minDistance < heightAboveTerrain)
            {
                floordist = cb.minDistance;
                floorNormal = cb.hitNormal;
            }

            constexpr float groundedCheckRange = 0.25f;
            if (
                floordist > groundedCheckRange ||
                (velocity.y > 0.0f && dot(velocity, floorNormal) > 0.4)
            )
            {
                // if (floordist > groundedCheckRange)
                // {
                //     DEBUG_MESSAGE(TIMESTAMP_PHYSICS, "Distance check failed... floordist was: ", floordist);

                //     vec3 p0 = entity.comp<state3D>().position;
                //     vec3 p1 = p0 + vec3(0, -groundedCheckRange, 0);
                //     GG::draw->drawSphere(p0, 0.01f, 1000.0, ModelState3D(), "#00ff00"_rgb);
                //     GG::draw->drawSphere(p1, 0.01f, 1000.0, ModelState3D(), "#00ff00"_rgb);
                //     GG::draw->drawLine(p0, p1, 1000.0);
                // }
                if (ms.grounded)
                    ms.lastGroundedTime = globals.simulationTime.getElapsedTime();

                ms.grounded = false;
                ms.walking  = false;
            }
            else if (floorNormal.y < 0.7)
            {
                if (!ms.grounded)
                {
                    ms.justLanded = true;
                    ms.landedTime = globals.simulationTime.getElapsedTime();
                }

                ms.grounded = true;
                ms.walking  = false;
            }
            else
            {
                if (!ms.grounded)
                {
                    ms.justLanded = true;
                    ms.landedTime = globals.simulationTime.getElapsedTime();
                }

                ms.grounded = true;
                ms.walking = true;
            }

            ms.groundNormalY = floorNormal.y;

            vec3 lookDir = s.usequat ? eulerAngles(s.quaternion) : s.lookDirection;
            vec3 eulerY = vec3(
                0.0f, 
                atan(-lookDir.x, -lookDir.z), 
                0.0f
            );

            rp3d::Transform transform(b->getTransform().getPosition(), PG::torp3d(quat(eulerY)));
            b->setTransform(transform);

            

            const float small_threshold = 0.05f;

            float dt = globals.simulationTime.getDelta();

            ms.deplacementDirection = normalize(velocity);
            ms.speed = length(velocity);

            if (ms.walking)
            {
                for (int i = 0; i < b->getNbColliders(); i++)
                {
                    b->getCollider(i)->getMaterial().setFrictionCoefficient(1.0f);
                }
            }
            else
            {
                for (int i = 0; i < b->getNbColliders(); i++)
                {
                    b->getCollider(i)->getMaterial().setFrictionCoefficient(0.0f);
                }
            }

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
                    
                        pos = mix(pos, ms.climbEndPos, t_vault);

                        // Logger::debug("pos'd: ", pos);

                        rp3d::Transform transform(PG::torp3d(pos), b->getTransform().getOrientation());
                        b->setTransform(transform);

                        vec3 forward = ms.climbEndPos - ms.climbStartPos;
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

            // check jump
            if (ms.isTryingToJump)
            {
                ms.grounded = false;
                ms.isTryingToJump = false;

                // Logger::debug(TIMESTAMP_PHYSICS, "???");
                
                if (ms.canClimb)
                {

                    // Logger::debug(TIMESTAMP_PHYSICS, "??? 2 electric boogaloo");

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

                    // Logger::debug(TIMESTAMP_PHYSICS, "climb :)");
                    // Logger::debug("start pos: ", ms.climbStartPos);
                    // Logger::debug("end pos: ", ms.climbEndPos);
                    // Logger::debug("vault pos: ", ms.climbVaultPos);
                } // add a tiny bit of coyote frames for feel and also to fix the fact that the physics engine is kinda broken
                else if (ms.walking || (ms.lastGroundedTime < globals.simulationTime.getElapsedTime() + 0.05f)) {
                    velocity.y = ms.jumpVelocity; // for nicer instantaneous jumps, try to change to += if it feels weird
                }
            }

            // get acceleration depending on state
            float accel;
            if (ms.walking)
            {
                accel = ms.ground_accelerate;

                // // try to slow the player down if they're on a slope
                // // kinda magic values, they give a nicer feel imo
                // const float scaling = 1.0f;
                // const float accelZeroAtThisNormal = 0.4f;
                // // normalize t to be between accelZeroAtThisNormal and 1
                // float t = (ms.groundNormalY - accelZeroAtThisNormal) / (1.0f - accelZeroAtThisNormal);
                // // function from https://easings.net/#easeInExpo with modified scaling
                // float normalDistanceFromUp = t == 0.0f ? 0 : pow(2.0f, t * scaling - scaling);

                // normalDistanceFromUp = clamp(normalDistanceFromUp, 0.0f, 1.0f);

                // // interpolate between air accelerate and ground accelerate depending on the slope's angle
                // accel = mix(ms.air_accelerate, ms.ground_accelerate, normalDistanceFromUp); 
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
