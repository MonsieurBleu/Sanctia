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
        vec3 direction = vec3(1, 0, 0);
        float speed = 0.f;
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
        bool isTryingToAttack = false;
    };

    COMPONENT(EntityActionState, DATA, MAX_DATA_COMP_USAGE);


/***************** GRAPHICS *****************/
    const int MAX_GRAPHIC_COMP_USAGE = MAX_ENTITY;

    struct EntityModel : public ObjectGroupRef{};

    COMPONENT(EntityModel, GRAPHIC, MAX_GRAPHIC_COMP_USAGE);
    template<>void Component<EntityModel>::ComponentElem::init();
    template<>void Component<EntityModel>::ComponentElem::clean();

    COMPONENT(SkeletonAnimationState, GRAPHIC, MAX_GRAPHIC_COMP_USAGE);
    template<>void Component<SkeletonAnimationState>::ComponentElem::init();

    COMPONENT(AnimationControllerRef, GRAPHIC, MAX_GRAPHIC_COMP_USAGE);

    
    class ItemsModel 
    {
        private : 

            struct SkeletonAttachedModel{
                int bone = 0; ObjectGroupRef model; 
                void followSkeleton(SkeletonAnimationState &);
            };

            void switchIem(SkeletonAttachedModel& cur, const SkeletonAttachedModel &newItem);
            
            Entity *usr = nullptr;

            #define ITEM_MODEL(name) SkeletonAttachedModel name; \
                ItemsModel& switch##name(const SkeletonAttachedModel &n){switchIem(name, n); return *this;};
        
        public : 
            void FollowSkeleton(SkeletonAttachedModel& item);

            ItemsModel(){};
            ItemsModel(Entity *usr) : usr(usr){};
            ITEM_MODEL(Weapon);
            ITEM_MODEL(Lantern);
    };

    COMPONENT(ItemsModel, GRAPHIC, MAX_GRAPHIC_COMP_USAGE);
    template<>void Component<ItemsModel>::ComponentElem::init();

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
