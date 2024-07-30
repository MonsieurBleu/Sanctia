#pragma once

#define MAX_COMP    64
#define MAX_ENTITY  8*4096

#include <GlobalOptions.hpp>

#include <Items.hpp>
#include <Entity.hpp>
#include <ObjectGroup.hpp>
#include <GameGlobals.hpp>
#include <Globals.hpp>
#include <EntityStats.hpp>
#include <Skeleton.hpp>
#include <Animation.hpp>
#include <AnimationController.hpp>
#include <Dialogue.hpp>
#include <unordered_set>

#include <PhysicsGlobals.hpp>

/***************** GAMEPLAY ATTRIBUTS *****************/
    const int MAX_DATA_COMP_USAGE = MAX_ENTITY;

    struct EntityState3D
    {
        vec3 position = vec3(0, 0, 0);
        vec3 lookDirection = vec3(1, 0, 0);
        float speed = 0.f;
        vec3 deplacementDirection = vec3(1, 0, 0);


        vec3 wantedDepDirection = vec3(0, 0, 0);
        float wantedSpeed = 0.f;
        float walkSpeed = 2.f;
        float sprintSpeed = 7.f;
        float airSpeed = 1.f;

        bool grounded = false;

        bool usequat = false;
        quat quaternion;

        #ifdef SANCTIA_DEBUG_PHYSIC_HELPER
        bool physicActivated = true;
        #endif

        // enum FollowType : uint8
        // {
        //     ModelFollowPhysic,
        //     PhysicFollowModel
        // } follow = '\0';


        /*
            MODIFIED BY PHYSICS : 

                POSITION 
                SPEED
                DEPLACEMENT DIRECTION 
                QUATERNION        


            MODIFIED BY CONTROLLS & AGENT LOGIC : 

                WANTED DEPLACEMENT DIRECTION 
                LOOK DIRECTION
                WANTED SPEED

            TYPE OF FOLLOWING : 

                SIMPLE OBJECTS 
                    PHYSICS ARE NORMAL 
                    MODEL FOLLOW QUATERNION AND POSITION 

                LIVING ENTITIES 
                    PHYSICS ARE INFLUENCED BY WANTED DEPLACEMENT DIRECTION AND WANTED SPEED
                    MODEL FOLLOW POSITION AND USE SPECIAL SLERP AND/OR SKELETAL ANIMATION TO FOLLOW LOOK DIRECTION
                        ==> THE BODY MUST ALWAYS BE STANDING UP
        */
    };

    COMPONENT(EntityState3D, DATA, MAX_DATA_COMP_USAGE)

    COMPONENT(EntityStats, DATA, MAX_DATA_COMP_USAGE)

    COMPONENT(CharacterDialogues, DATA, MAX_DATA_COMP_USAGE)

    struct NpcPcRelation
    {
        bool known = false;
        short affinity = 0; 
    };

    COMPONENT(NpcPcRelation, DATA, MAX_DATA_COMP_USAGE)

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

    COMPONENT(ActionState, DATA, MAX_DATA_COMP_USAGE)

    class Faction
    {
        public :
            enum class Type : uint8 
                {
                    NEUTRAL, 
                    ENVIRONEMENT, 
                    PLAYER, 
                    PLAYER_ENEMY, 
                    MONSTERS
                } type = Type::NEUTRAL;

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

    COMPONENT(Faction, DATA, MAX_DATA_COMP_USAGE)

/***************** ITEMS *****************/

    COMPONENT(ItemInfos, DATA, MAX_DATA_COMP_USAGE)

    COMPONENT(Items, DATA, MAX_DATA_COMP_USAGE)

    COMPONENT(ItemTransform, DATA, MAX_DATA_COMP_USAGE)


/***************** GRAPHICS *****************/
    const int MAX_GRAPHIC_COMP_USAGE = MAX_ENTITY;

    struct EntityModel : public ObjectGroupRef{};

    COMPONENT(EntityModel, GRAPHIC, MAX_GRAPHIC_COMP_USAGE)
    template<>void Component<EntityModel>::ComponentElem::init();
    template<>void Component<EntityModel>::ComponentElem::clean();

    COMPONENT(SkeletonAnimationState, GRAPHIC, MAX_GRAPHIC_COMP_USAGE)
    template<>void Component<SkeletonAnimationState>::ComponentElem::init();

    COMPONENT(AnimationControllerRef, GRAPHIC, MAX_GRAPHIC_COMP_USAGE)

/***************** PHYSICS *****************/
    const int MAX_PHYSIC_COMP_USAGE = MAX_ENTITY;

    COMPONENT(Effect, PHYSIC, MAX_PHYSIC_COMP_USAGE)
    // template<> void Component<Effect>::ComponentElem::init();

    COMPONENT(rp3d::RigidBody*, PHYSIC, MAX_PHYSIC_COMP_USAGE)
    template<> void Component<rp3d::RigidBody*>::ComponentElem::clean();

/***************** IA *****************/
    const int MAX_IA_COMP_USAGE = MAX_ENTITY;

    enum DeplacementBehaviour
    {
        STAND_STILL, DEMO, FOLLOW_WANTED_DIR
    };
    COMPONENT(DeplacementBehaviour, AI, MAX_IA_COMP_USAGE)

    struct AgentState
    {
        enum State
        {
            COMBAT_POSITIONING,
            COMBAT_ATTACKING,
            COMBAT_BLOCKING
        } state;

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
    COMPONENT(AgentState, AI, MAX_IA_COMP_USAGE)

    struct Target : EntityRef {};
    COMPONENT(Target, AI, MAX_IA_COMP_USAGE)

/***************** HELPERS *****************/
    struct PhysicsHelpers : public ObjectGroupRef{};

    COMPONENT(PhysicsHelpers, GRAPHIC, MAX_GRAPHIC_COMP_USAGE);
    template<> void Component<PhysicsHelpers>::ComponentElem::clean();
    template<> void Component<PhysicsHelpers>::ComponentElem::init();

    struct InfosStatsHelpers{std::vector<ModelRef> models;};
    COMPONENT(InfosStatsHelpers, GRAPHIC, MAX_GRAPHIC_COMP_USAGE);
    template<> void Component<InfosStatsHelpers>::ComponentElem::init();
    template<> void Component<InfosStatsHelpers>::ComponentElem::clean();
