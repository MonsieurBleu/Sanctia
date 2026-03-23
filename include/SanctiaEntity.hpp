#pragma once

#define MAX_COMP    64
// #define MAX_ENTITY  0x8000
#define MAX_ENTITY  (1<<17)


#include <GlobalOptions.hpp>

#define ENTITY_HEADER_SAFE_INCLUDE
#include <ECS/ModularEntityGroupping.hpp>
#undef ENTITY_HEADER_SAFE_INCLUDE

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
#include <ECS/ComponentTypeScripting.hpp>

#undef CURRENT_MAX_COMP_USAGE
#define CURRENT_MAX_COMP_USAGE MAX_ENTITY


EntityRef spawnEntity(const std::string &name, vec3 spawnPoint = vec3(0), quat rotation = quat(0, 0, 0, 0));

bool isVisible(Entity &a, Entity &b);

Entity* getClosestVisibleEnemy(Entity &e);

Entity* getClosestVisibleAlly(Entity &e);

/***************** SCRIPTING COMPONENTS *****************/

    #undef CURRENT_CATEGORY
    #define CURRENT_CATEGORY SCRIPTING

    Component_Compatible(Script)


/***************** GAMEPLAY ATTRIBUTS *****************/

    #undef CURRENT_CATEGORY
    #define CURRENT_CATEGORY DATA

    Component_Init_Compatible(state3D)
    template<> void Component<state3D>::ComponentElem::clean();

    Component(StainStatus)
    template<> void Component<StainStatus>::ComponentElem::init();
    template<> void Component<StainStatus>::ComponentElem::clean();


/***************** DYNAMIC GAMEPLAY ATTRIBUTS *****************/

    #undef CURRENT_CATEGORY
    #define CURRENT_CATEGORY DATA_DYNAMIC

    Component(MovementState)

    Component(EntityStats)

    Component(CharacterDialogues)

    Component(NpcPcRelation)

    Component(ActionState)

    Component(Faction)


/***************** ITEMS *****************/

    Component(ItemInfos)

    Component(Items)
    template<> void Component<Items>::ComponentElem::clean();

    Component(ItemTransform)


/***************** GRAPHICS *****************/

    #undef CURRENT_CATEGORY
    #define CURRENT_CATEGORY GRAPHIC

    Component_Init_Merge(EntityModel)
    template<> void Component<EntityModel>::ComponentElem::init();
    template<> void Component<EntityModel>::ComponentElem::clean();

    Component_Init_Merge(LevelOfDetailsInfos)
    template<> void Component<LevelOfDetailsInfos>::ComponentElem::init();


/***************** HELPERS *****************/

    Component(PhysicsHelpers);
    template<> void Component<PhysicsHelpers>::ComponentElem::init();
    template<> void Component<PhysicsHelpers>::ComponentElem::clean();

    Component(InfosStatsHelpers);
    template<> void Component<InfosStatsHelpers>::ComponentElem::init();
    template<> void Component<InfosStatsHelpers>::ComponentElem::clean();


/***************** GRAPHICS DYNAMIC *****************/

    #undef CURRENT_CATEGORY
    #define CURRENT_CATEGORY GRAPHIC_DYNAMIC

    Component(SkeletonAnimationState)
    template<> void Component<SkeletonAnimationState>::ComponentElem::init();

    Component(AnimationControllerRef)

    Component(AnimationControllerInfos)


/***************** PHYSICS *****************/

    #undef CURRENT_CATEGORY
    #define CURRENT_CATEGORY PHYSIC
    
    Component_Init_Merge_Compatible(RigidBody)
    template<> void Component<RigidBody>::ComponentElem::init();
    template<> void Component<RigidBody>::ComponentElem::clean();

    Component(HeightFieldDummyFlag)

    Component(PhysicsInfos)


/***************** PHYSICS DYNAMIC *****************/

    #undef CURRENT_CATEGORY
    #define CURRENT_CATEGORY PHYSIC_DYNAMIC

    Component(Effect)

    Component(NonStaticBodyDummyFlag)


/***************** IA *****************/

    #undef CURRENT_CATEGORY
    #define CURRENT_CATEGORY AI

    Component(DeplacementBehaviour)

    Component(AgentState__old)

    Component(AgentState)

    Component(Target)
    template<> void Component<Target>::ComponentElem::clean();

    Component(AgentProfile)




