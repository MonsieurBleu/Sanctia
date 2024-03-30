#include <SanctiaEntity.hpp>
#include <Globals.hpp>
#include <algorithm>
#include <Utils.hpp>
#include <Helpers.hpp>

#include <glm/gtx/string_cast.hpp>

template<>
void Component<EntityModel>::ComponentElem::init()
{
    // std::cout << "creating entity model " << entity->toStr();
    globals.getScene()->add(data);
};

template<>
void Component<EntityModel>::ComponentElem::clean()
{
    // std::cout << "deleting entity model " << entity->toStr();

    if(data.get())
        globals.getScene()->remove(data);
    else
        WARNING_MESSAGE("Trying to clean null component from entity " << entity->ids[ENTITY_LIST] << " named " << entity->comp<EntityInfos>().name)
};

ModelRef getModelFromCollider(B_Collider &c, vec3 color)
{
    switch (c.type)
    {
    case B_ColliderType::Sphere :
        return SphereHelperRef(new SphereHelper(color, c.v1.x));

    case B_ColliderType::Capsule :
        return CapsuleHelperRef(new CapsuleHelper(&c.v4, &c.v5, &c.v1.x, color));
        break;
    
    default:
        break;
    }

    return newModel();
}

template<>
void Component<PhysicsHelpers>::ComponentElem::init()
{
    data = {newObjectGroup()};

    if(entity->hasComp<B_DynamicBodyRef>())
    {
        auto &b = entity->comp<B_DynamicBodyRef>();
        data->add(getModelFromCollider(b->boundingCollider, vec3(1, 1, 0)));
    }

    if(entity->hasComp<Effect>())
    // if(entity->ids[PHYSIC] != NO_ENTITY)
    {
        auto &c = entity->comp<Effect>().zone;
        data->add(getModelFromCollider(c, vec3(1, 0, 0)));
    }



    globals.getScene()->add(data);
    // entity->comp<EntityModel>()->add(data);
};


template<>
void Component<PhysicsHelpers>::ComponentElem::clean()
{
    if(data.get())
        globals.getScene()->remove(data);
    else
        WARNING_MESSAGE("Trying to clean null component from entity " << entity->ids[ENTITY_LIST] << " named " << entity->comp<EntityInfos>().name)
};

template<>
void Component<InfosStatsHelpers>::ComponentElem::init()
{
    if(!entity->hasComp<EntityModel>()) return;

    ValueHelperRef<std::string> N(new ValueHelper(entity->comp<EntityInfos>().name, U"", vec3(0.85)));
    N->state.setPosition(vec3(0, 2.0, 0)).scaleScalar(5);
    entity->comp<EntityModel>()->add(N);
    globals.getScene()->add(N);
    data.models.push_back(N);

    if(entity->hasComp<EntityStats>() && entity->hasComp<EntityModel>())
    {
        auto &s = entity->comp<EntityStats>();
        ValueHelperRef<float> HP(new ValueHelper(s.health.cur, U"Health ", vec3(0, 1, 0)));
        HP->state.setPosition(vec3(0, 2.2, 0)).scaleScalar(5);
        entity->comp<EntityModel>()->add(HP);
        globals.getScene()->add(HP);

        ValueHelperRef<float> ST(new ValueHelper(s.stamina.cur, U"Stamina ", vec3(1, 1, 0)));
        ST->state.setPosition(vec3(0, 2.4, 0)).scaleScalar(5);
        entity->comp<EntityModel>()->add(ST);
        globals.getScene()->add(ST);

        data.models.push_back(HP);
        data.models.push_back(ST);
    }
}

template<>
void Component<InfosStatsHelpers>::ComponentElem::clean()
{
    if(!entity || entity->ids[GRAPHIC] < 0 || entity->ids[GRAPHIC] > MAX_ENTITY || !entity->hasComp<EntityModel>()) return;

    for(auto &i : data.models)
    {
        entity->comp<EntityModel>()->remove(i);
        globals.getScene()->remove(i);
    }
}

template<>
void Component<B_DynamicBodyRef>::ComponentElem::init()
{
    physicsMutex.lock();
    GG::physics.dynamics.push_back(data);
    physicsMutex.unlock();
}

template<>
void Component<B_DynamicBodyRef>::ComponentElem::clean()
{
    static void* ptr = nullptr;
    ptr = data.get();

    // GG::physics.dynamics.erase(
    //     std::remove_if(
    //         GG::physics.dynamics.begin(),
    //         GG::physics.dynamics.end(),
    //         [](B_DynamicBodyRef &a)
    //         {   
    //             return a.get() == ptr;
    //         }), 
    //     GG::physics.dynamics.end());

    for(auto i = GG::physics.dynamics.begin(); i != GG::physics.dynamics.end(); i++)
    {
        if(i->get() == ptr)
        {
            GG::physics.dynamics.erase(i);
            return;
        }
    }
}