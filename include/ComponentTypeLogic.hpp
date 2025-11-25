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

#include <vector>
#include <VulpineBitSet.hpp>

#include <Globals.hpp>

class Entity;

#define UNINITIALIZED_FLOAT 1e12f

struct EntityState3D
{
    EntityState3D(){};
    EntityState3D(vec3 pos) : position(pos){};
    EntityState3D(bool usequat) : usequat(usequat){};
    EntityState3D(bool usequat, vec3 initpos) : usequat(usequat), initPosition(initpos){}; 

    vec3 position = vec3(0, 0, 0);
    quat quaternion = quat(1, 0, 0, 0);
    vec3 lookDirection = vec3(1, 0, 0);

    bool usequat = false;
    bool useinit = true;
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


struct DeplacementState
{
    float speed = 0.f;
    float wantedSpeed = 0.f;

    vec3 deplacementDirection = vec3(1, 0, 0);
    vec3 wantedDepDirection = vec3(0, 0, 0);

    bool grounded = false;

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
    
    public :
    
        Stance _stance = Stance::LEFT;
        Stance _wantedStance = Stance::LEFT;
        // void setStance(Stance s){if(!attacking && !stun && !(blocking && _stance == SPECIAL)) _stance = s;};
        void setStance(Stance s){_wantedStance = s;};
        const Stance& stance() const {return _wantedStance;};

        bool isTryingToAttack = false;
        float isTryingToAttackTime = 0;

        bool isTryingToBlock = false;
        float isTryingToBlockTime = 0;

        bool hasBlockedAttack = false;
        float hasBlockedAttackTime = 0;

        bool stun = false;
        float stunTime = 0;

        bool blocking = false;
        float blockingTime = 0;

        bool attacking = false;
        float attackingTime = 0;

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

        static void clearRelations(){isEnemy.clear();};
    
};

GENERATE_ENUM_FAST_REVERSE(DeplacementBehaviour
    , STAND_STILL 
    , DEMO
    , FOLLOW_WANTED_DIR
)

struct LevelOfDetailsInfos
{
    bool activated = false;

    static inline const float distLevelNear = 50.f;
    static inline const float distLevelFar  = 150.f;
    static inline const float distLevelBias = 5.f;

    vec3 aabbmin; 
    vec3 aabbmax;

    int level = 0;

    struct ChildrenLoadInfo
    {
        const std::string name;
        VulpineBitSet<3> lod;
    };

    std::vector<ChildrenLoadInfo> childrenLoadInfos;

    void computeEntityAABB(Entity *e);
    void computeLevel(vec3 p);
};
