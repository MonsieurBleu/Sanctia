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

ModelRef getModelFromCollider(B_Collider c, vec3 color)
{
    switch (c.type)
    {
    case B_ColliderType::Sphere :
        return SphereHelperRef(new SphereHelper(color, c.v1.x));

    case B_ColliderType::Capsule :
        std::cout << "Yooo capsule\t" << to_string(c.v2) << to_string(c.v3) <<"\n";
        return CapsuleHelperRef(new CapsuleHelper(c.v2, c.v3, c.v1.x));
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
        auto g = newObjectGroup();
        auto b = entity->comp<B_DynamicBodyRef>();
        g->add(getModelFromCollider(b->boundingCollider, vec3(1, 1, 0)));

        data.dbodies.push_back({b, g});

        data->add(g);
    }

    // if(entity->hasComp<Effect>())
    // {
    //     data.bodies
    // }

    globals.getScene()->add(data);
    // entity->comp<EntityModel>()->add(data);
};


template<>
void Component<PhysicsHelpers>::ComponentElem::clean()
{
    std::cout << "deleting entity model " << entity->toStr();

    if(data.get())
        globals.getScene()->remove(data);
    else
        WARNING_MESSAGE("Trying to clean null component from entity " << entity->ids[ENTITY_LIST] << " named " << entity->comp<EntityInfos>().name)
};

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