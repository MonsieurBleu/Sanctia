#pragma once

#define MAX_COMP    64
#define MAX_ENTITY  0x8000


#include <GlobalOptions.hpp>
#include <ECS/ModularEntityGroupping.hpp>

#include <ECS/EngineComponents.hpp>

#include <Items.hpp>
#include <Globals.hpp>
#include <EntityStats.hpp>
#include <Graphics/AnimationController.hpp>
#include <Dialogue.hpp>

#include <ComponentTypeLogic.hpp>
#include <ComponentTypeGraphic.hpp>
#include <ComponentTypePhysic.hpp>
#include <ComponentTypeAI.hpp>

#undef CURRENT_MAX_COMP_USAGE
#define CURRENT_MAX_COMP_USAGE MAX_ENTITY


/***************** GAMEPLAY ATTRIBUTS *****************/

    #undef CURRENT_CATEGORY
    #define CURRENT_CATEGORY DATA

    Coherent_Component(EntityState3D)

    Ephemeral_Component(EntityDeplacementState)

    Ephemeral_Component(EntityStats)

    Ephemeral_Component(CharacterDialogues)

    Ephemeral_Component(NpcPcRelation)

    Ephemeral_Component(ActionState)

    Ephemeral_Component(Faction)


/***************** ITEMS *****************/

    Ephemeral_Component(ItemInfos)

    Ephemeral_Component(Items)
    template<> void Component<Items>::ComponentElem::clean();

    Ephemeral_Component(ItemTransform)


/***************** GRAPHICS *****************/

    #undef CURRENT_CATEGORY
    #define CURRENT_CATEGORY GRAPHIC

    Intrinsic_Component(EntityModel)
    template<> void Component<EntityModel>::ComponentElem::init();
    template<> void Component<EntityModel>::ComponentElem::clean();

    Ephemeral_Component(SkeletonAnimationState)
    template<> void Component<SkeletonAnimationState>::ComponentElem::init();

    Ephemeral_Component(AnimationControllerRef)

    Ephemeral_Component(AnimationControllerInfos)


/***************** HELPERS *****************/

    Ephemeral_Component(PhysicsHelpers);
    template<> void Component<PhysicsHelpers>::ComponentElem::init();
    template<> void Component<PhysicsHelpers>::ComponentElem::clean();

    Ephemeral_Component(InfosStatsHelpers);
    template<> void Component<InfosStatsHelpers>::ComponentElem::init();
    template<> void Component<InfosStatsHelpers>::ComponentElem::clean();

/***************** PHYSICS *****************/

    #undef CURRENT_CATEGORY
    #define CURRENT_CATEGORY PHYSIC

    Ephemeral_Component(Effect)

    Aligned_Intricate_Component(RigidBody)
    template<> void Component<RigidBody>::ComponentElem::init();
    template<> void Component<RigidBody>::ComponentElem::clean();

    Ephemeral_Component(staticEntityFlag)

/***************** IA *****************/

    #undef CURRENT_CATEGORY
    #define CURRENT_CATEGORY AI

    Ephemeral_Component(DeplacementBehaviour)

    Ephemeral_Component(AgentState)

    Ephemeral_Component(Target)





