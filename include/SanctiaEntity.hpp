#pragma once

#define MAX_COMP    64
#define MAX_ENTITY  0x8000


#include <GlobalOptions.hpp>
#include <ModularEntityGroupping.hpp>

#include <ComponentTypeLogic.hpp>
#include <ComponentTypeGraphic.hpp>
#include <ComponentTypePhysic.hpp>

#include <Items.hpp>
#include <Globals.hpp>
#include <EntityStats.hpp>
#include <AnimationController.hpp>
#include <Dialogue.hpp>
#include <PhysicsGlobals.hpp>

/***************** GAMEPLAY ATTRIBUTS *****************/
    const int MAX_DATA_COMP_USAGE = MAX_ENTITY;

    /*
        TODO : maybe try to move it to graphic or another category
    */
    COMPONENT(EntityState3D, DATA, MAX_DATA_COMP_USAGE)
    COMPONENT_ADD_SYNCH(EntityState3D)

    COMPONENT(EntityDeplacementState, DATA, MAX_DATA_COMP_USAGE)
    
    COMPONENT(EntityStats, DATA, MAX_DATA_COMP_USAGE)

    COMPONENT(CharacterDialogues, DATA, MAX_DATA_COMP_USAGE)

    COMPONENT(NpcPcRelation, DATA, MAX_DATA_COMP_USAGE)

    COMPONENT(ActionState, DATA, MAX_DATA_COMP_USAGE)

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

    struct AnimationControllerInfos : std::string
    {
        GENERATE_ENUM_FAST_REVERSE(Type, Biped)

        Type type;
    };
    COMPONENT(AnimationControllerInfos, GRAPHIC, MAX_GRAPHIC_COMP_USAGE)

/***************** PHYSICS *****************/
    const int MAX_PHYSIC_COMP_USAGE = MAX_ENTITY;

    COMPONENT(Effect, PHYSIC, MAX_PHYSIC_COMP_USAGE)

    COMPONENT(RigidBody, PHYSIC, MAX_PHYSIC_COMP_USAGE)
    template<> void Component<RigidBody>::ComponentElem::init();
    template<> void Component<RigidBody>::ComponentElem::clean();

/***************** IA *****************/
    const int MAX_IA_COMP_USAGE = MAX_ENTITY;

    COMPONENT(DeplacementBehaviour, AI, MAX_IA_COMP_USAGE)

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
