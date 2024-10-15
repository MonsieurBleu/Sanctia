#pragma once

#include <MappedEnum.hpp>

#ifndef GLM_VERSION_MAJOR 
#include <glm/glm.hpp>
#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtx/euler_angles.hpp>
#endif

using namespace glm;

#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtx/quaternion.hpp>
#include <glm/gtc/quaternion.hpp>

#include <unordered_set>

#define UNINITIALIZED_FLOAT 1e12

struct EntityState3D
{
    EntityState3D(){};
    EntityState3D(vec3 pos) : position(pos){};
    EntityState3D(bool usequat) : usequat(usequat){};

    vec3 position = vec3(0, 0, 0);
    quat quaternion = quat(1, 0, 0, 0);
    vec3 lookDirection = vec3(1, 0, 0);

    bool usequat = false;
    bool useinit = false;
    bool usePhysicInterpolation = true;

    vec3 initPosition = vec3(0);
    quat initQuat = quat(1, 0, 0, 0);
    vec3 initLookDirection = vec3(0);

    vec3 _PhysicTmpPos = vec3(UNINITIALIZED_FLOAT);
    quat _PhysicTmpQuat = quat(1, 0, 0, 0);
    // vec3 _PhysicTmpLookDirection = vec3(0);

    #ifdef SANCTIA_DEBUG_PHYSIC_HELPER
    bool physicActivated = true;
    #endif
};


struct EntityDeplacementState
{
    float speed = 0.f;

    vec3 deplacementDirection = vec3(1, 0, 0);
    vec3 wantedDepDirection = vec3(0, 0, 0);

    bool grounded = false;

    float wantedSpeed = 0.f;

    float walkSpeed = 2.f;
    float sprintSpeed = 7.f;
    float airSpeed = 1.f;
};



struct NpcPcRelation
{
    bool known = false;
    short affinity = 0; 
};

class ActionState
{
    public : 
        enum Stance : uint8 {LEFT, RIGHT, SPECIAL};

    private :
        Stance _stance = Stance::LEFT;

    public :

        void setStance(Stance s){if(!attacking && !stun && !(blocking && _stance == SPECIAL)) _stance = s;};
        const Stance& stance() const {return _stance;};

        bool isTryingToAttack = false;
        bool isTryingToBlock = false;
        bool hasBlockedAttack = false;

        bool stun = false;
        bool blocking = false;

        bool attacking = false;

        enum LockedDeplacement{NONE, SPEED_ONLY, DIRECTION} lockType;
        float lockedMaxSpeed = 0;
        vec3 lockedDirection = vec3(1, 0, 0);

};

class Faction
{
    public :

        GENERATE_ENUM_FAST_REVERSE(Type, 
            NEUTRAL, 
            ENVIRONEMENT, 
            PLAYER, 
            PLAYER_ENEMY, 
            MONSTERS        
        );

        Type type = Type::NEUTRAL;

        // enum class Type : uint8 
        //     {
        //         NEUTRAL, 
        //         ENVIRONEMENT, 
        //         PLAYER, 
        //         PLAYER_ENEMY, 
        //         MONSTERS
        //     } type = Type::NEUTRAL;

    private :
        static uint16 Pair(Faction f1, Faction f2)
        {
            uint8 f[2] = {min((uint8)f1.type, (uint8)f2.type), max((uint8)f1.type, (uint8)f2.type)};
            return *(uint16*)f;
        }

        static inline std::unordered_set<uint16> isEnemy;

    public :
        static void setEnemy(Faction f1, Faction f2)
        {
            isEnemy.insert(Pair(f1, f2));
        };

        static bool areEnemy(Faction f1, Faction f2)
        {
            return isEnemy.find(Pair(f1, f2)) != isEnemy.end();
        };
    
};

GENERATE_ENUM_FAST_REVERSE(DeplacementBehaviour
    , STAND_STILL 
    , DEMO
    , FOLLOW_WANTED_DIR
)

struct AgentState
{
    GENERATE_ENUM_FAST_REVERSE(State
        , COMBAT_POSITIONING
        , COMBAT_ATTACKING
        , COMBAT_BLOCKING
    ) 
    
    State state;

    float timeSinceLastState = 0.f;
    float randomTime = 0.f;

    void TransitionTo(State newState, float minTime = 0.f, float maxTime = 0.f)
    {
        state = newState; 
        timeSinceLastState = 0.f;
        float r = (float)(std::rand()%1024)/1024.f;
        randomTime = minTime + r*(maxTime-minTime);
    }
};