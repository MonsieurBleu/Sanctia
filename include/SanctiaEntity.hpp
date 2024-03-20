#pragma once

#define MAX_COMP    64
#define MAX_ENTITY  512

#include <Entity.hpp>

#include <ObjectGroup.hpp>
#include <GameGlobals.hpp>
#include <Globals.hpp>
#include <BluePhysics.hpp>
#include <EntityStats.hpp>

struct EntityModel : public ObjectGroupRef
{   
};

COMPONENT(EntityModel, GRAPHIC, MAX_ENTITY);

template<>
void Component<EntityModel>::ComponentElem::init();

template<>
void Component<EntityModel>::ComponentElem::clean();

struct EntityState3D
{
    vec3 position;
    vec3 direction;
};

COMPONENT(EntityState3D, DATA, MAX_ENTITY);


COMPONENT(B_DynamicBodyRef, PHYSIC, MAX_ENTITY);

template<>
void Component<B_DynamicBodyRef>::ComponentElem::init();

template<>
void Component<B_DynamicBodyRef>::ComponentElem::clean();


COMPONENT(EntityStats, DATA, MAX_ENTITY);

COMPONENT(Effect, PHYSIC, MAX_ENTITY);
