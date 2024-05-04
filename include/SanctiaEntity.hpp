#pragma once

#define MAX_COMP    64
#define MAX_ENTITY  512

#include <Entity.hpp>
#include <ObjectGroup.hpp>
#include <GameGlobals.hpp>
#include <Globals.hpp>
#include <BluePhysics.hpp>
#include <EntityStats.hpp>
#include <Skeleton.hpp>
#include <Animation.hpp>
#include <AnimationController.hpp>
#include <Dialogue.hpp>


/***************** GAMEPLAY ATTRIBUTS *****************/
    const int MAX_DATA_COMP_USAGE = MAX_ENTITY;

    struct EntityState3D
    {
        vec3 position = vec3(0, 0, 0);
        vec3 lookDirection = vec3(1, 0, 0);
        float speed = 0.f;
        vec3 deplacementDirection = vec3(1, 0, 0);
        vec3 wantedDepDirection = vec3(1, 0, 0);
    };

    COMPONENT(EntityState3D, DATA, MAX_DATA_COMP_USAGE);

    COMPONENT(EntityStats, DATA, MAX_DATA_COMP_USAGE);

    COMPONENT(CharacterDialogues, DATA, MAX_DATA_COMP_USAGE);

    struct NpcPcRelation
    {
        bool known = false;
        short affinity = 0; 
    };

    COMPONENT(NpcPcRelation, DATA, MAX_DATA_COMP_USAGE);

    struct EntityActionState
    {
        enum Stance {LEFT, RIGHT, SPECIAL} stance = Stance::LEFT;

        bool isTryingToAttack = false;

        bool stun = false;
        bool blocking = false;

        enum LockedDeplacement{NONE, SPEED_ONLY, DIRECTION} lockDirection;
        float lockedMaxSpeed = 0;
        float lockedAcceleration = 0;
        vec3 lockedDirection = vec3(1, 0, 0);
    };

    COMPONENT(EntityActionState, DATA, MAX_DATA_COMP_USAGE);


/***************** ITEMS *****************/
    struct ItemInfos
    {
        int price = 1.f;
        float dmgMult = 20.f;
        int dmgType = DamageType::Pure;
        B_Collider dmgZone;
    };

    COMPONENT(ItemInfos, DATA, MAX_DATA_COMP_USAGE);

    enum EquipementSlots
    {
        WEAPON_SLOT
    };

    struct Items
    {
        struct Equipement{int id = 0; EntityRef item;} equipped[16];
    };

    COMPONENT(Items, DATA, MAX_DATA_COMP_USAGE);

    struct ItemTransform{mat4 t;};

    COMPONENT(ItemTransform, DATA, MAX_DATA_COMP_USAGE);


/***************** GRAPHICS *****************/
    const int MAX_GRAPHIC_COMP_USAGE = MAX_ENTITY;

    struct EntityModel : public ObjectGroupRef{};

    COMPONENT(EntityModel, GRAPHIC, MAX_GRAPHIC_COMP_USAGE);
    template<>void Component<EntityModel>::ComponentElem::init();
    template<>void Component<EntityModel>::ComponentElem::clean();

    COMPONENT(SkeletonAnimationState, GRAPHIC, MAX_GRAPHIC_COMP_USAGE);
    template<>void Component<SkeletonAnimationState>::ComponentElem::init();

    COMPONENT(AnimationControllerRef, GRAPHIC, MAX_GRAPHIC_COMP_USAGE);

/***************** PHYSICS *****************/
    const int MAX_PHYSIC_COMP_USAGE = MAX_ENTITY;

    COMPONENT(B_DynamicBodyRef, PHYSIC, MAX_PHYSIC_COMP_USAGE);
    template<> void Component<B_DynamicBodyRef>::ComponentElem::init();
    template<> void Component<B_DynamicBodyRef>::ComponentElem::clean();

    COMPONENT(Effect, PHYSIC, MAX_PHYSIC_COMP_USAGE);


/***************** IA *****************/
    const int MAX_IA_COMP_USAGE = MAX_ENTITY;

    enum DeplacementBehaviour
    {
        DEMO
    };
    COMPONENT(DeplacementBehaviour, AI, MAX_IA_COMP_USAGE);


/***************** HELPERS *****************/
    struct PhysicsHelpers : public ObjectGroupRef{};

    COMPONENT(PhysicsHelpers, GRAPHIC, MAX_GRAPHIC_COMP_USAGE);
    template<> void Component<PhysicsHelpers>::ComponentElem::clean();
    template<> void Component<PhysicsHelpers>::ComponentElem::init();

    struct InfosStatsHelpers{std::vector<ModelRef> models;};
    COMPONENT(InfosStatsHelpers, GRAPHIC, MAX_GRAPHIC_COMP_USAGE);
    template<> void Component<InfosStatsHelpers>::ComponentElem::init();
    template<> void Component<InfosStatsHelpers>::ComponentElem::clean();
