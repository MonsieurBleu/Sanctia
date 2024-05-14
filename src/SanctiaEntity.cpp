#include <SanctiaEntity.hpp>
#include <Globals.hpp>
#include <algorithm>
#include <Utils.hpp>
#include <Helpers.hpp>
#include <MathsUtils.hpp>
#include <GameGlobals.hpp>

#include <glm/gtx/string_cast.hpp>

template<> void Component<EntityModel>::ComponentElem::init()
{
    // std::cout << "creating entity model " << entity->toStr();
    globals.getScene()->add(data);
};

template<> void Component<EntityModel>::ComponentElem::clean()
{
    // std::cout << "deleting entity model " << entity->toStr();

    if(data.get())
        globals.getScene()->remove(data);
    else
        WARNING_MESSAGE("Trying to clean null component from entity " << entity->ids[ENTITY_LIST] << " named " << entity->comp<EntityInfos>().name)
};

template<> void Component<SkeletonAnimationState>::ComponentElem::init()
{
    if(entity->hasComp<EntityModel>())
    {
        entity->comp<EntityModel>()->setAnimation(&data);
    }
}

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

template<> void Component<PhysicsHelpers>::ComponentElem::init()
{
    data = {newObjectGroup()};

    if(entity->hasComp<B_DynamicBodyRef>() && entity != GG::playerEntity.get())
    {
        auto &b = entity->comp<B_DynamicBodyRef>();
        data->add(getModelFromCollider(b->boundingCollider, vec3(1, 1, 0)));
    }

    if(entity->ids[PHYSIC] != NO_ENTITY)
    {
        Effect &l = entity->comp<Effect>();

        data->add(getModelFromCollider(l.zone, vec3(1, 0, 0)));
    }

    globals.getScene()->add(data);
    // entity->comp<EntityModel>()->add(data);
};

template<> void Component<PhysicsHelpers>::ComponentElem::clean()
{
    if(data.get())
        globals.getScene()->remove(data);
    else
        WARNING_MESSAGE("Trying to clean null component from entity " << entity->ids[ENTITY_LIST] << " named " << entity->comp<EntityInfos>().name)
};

template<> void Component<InfosStatsHelpers>::ComponentElem::init()
{
    if(!entity->hasComp<EntityModel>()) return;

    float textScale = 3.0;
    float offHeight = 0.11;

    auto minmax = entity->comp<EntityModel>()->getMeshesBoundingBox();
    vec3 size = abs(minmax.first) + abs(minmax.second);
    float curHeight = size.y + offHeight;
    vec3 dir = vec3(0, 1, 0);

    // if(size.x > size.y && size.x > size.z)
    // {
    //     curHeight = size.x + offHeight;
    //     dir = vec3(1, 0, 0);
    // }
    // if(size.x > size.y)
    // {
    //     curHeight = size.x + offHeight;
    //     dir = vec3(1, 0, 0);
    // }
    
    if(entity->hasComp<ItemInfos>())
    {
        curHeight = 0;
        dir = vec3(1, 0, 0);
    }

    #define ADD_VALUE_HELPER(Helper) \
        Helper->state.setPosition(curHeight * dir).scaleScalar(textScale); \
        entity->comp<EntityModel>()->add(Helper); \
        globals.getScene()->add(Helper); \
        data.models.push_back(Helper); \
        curHeight += offHeight; 

    ValueHelperRef<std::string> N(new ValueHelper(entity->comp<EntityInfos>().name, U"Name ", vec3(0.85)));
    ADD_VALUE_HELPER(N)


    if(entity->hasComp<EntityStats>())
    {
        auto &s = entity->comp<EntityStats>();
        ValueHelperRef<float> HP(new ValueHelper(s.health.cur, U"Health ", vec3(0, 1, 0)));
        ADD_VALUE_HELPER(HP)

        ValueHelperRef<float> ST(new ValueHelper(s.stamina.cur, U"Stamina ", vec3(1, 1, 0)));
        ADD_VALUE_HELPER(ST)
    }

    if(entity->hasComp<NpcPcRelation>())
    {
        auto &r = entity->comp<NpcPcRelation>();
        ValueHelperRef<safeBoolOverload> K(new ValueHelper(*(safeBoolOverload*)&r.known, U"Know Player ", ColorHexToV(0x6668DE))); 
        ADD_VALUE_HELPER(K)

        ValueHelperRef<short> A(new ValueHelper(r.affinity, U"Affinity ", ColorHexToV(0x66B4DE)));
        ADD_VALUE_HELPER(A)
    }

    if(entity->hasComp<ItemInfos>())
    {
        auto &i = entity->comp<ItemInfos>();

        ValueHelperRef<int> P(new ValueHelper(i.price, U"Price ", ColorHexToV(0xffcf40)));
        ValueHelperRef<float> M(new ValueHelper(i.dmgMult, U"Damage Mult ", ColorHexToV(0xf04a1d)));
        ValueHelperRef<int> T(new ValueHelper(i.dmgType, U"Damage Type ", ColorHexToV(0xc0e81e)));

        ADD_VALUE_HELPER(P)
        ADD_VALUE_HELPER(M)
        ADD_VALUE_HELPER(T)
    }
}

template<> void Component<InfosStatsHelpers>::ComponentElem::clean()
{
    if(!entity || entity->ids[GRAPHIC] < 0 || entity->ids[GRAPHIC] > MAX_ENTITY || !entity->hasComp<EntityModel>()) return;

    for(auto &i : data.models)
    {
        entity->comp<EntityModel>()->remove(i);
        globals.getScene()->remove(i);
    }
}

template<> void Component<B_DynamicBodyRef>::ComponentElem::init()
{
    physicsMutex.lock();
    GG::physics.dynamics.push_back(data);
    physicsMutex.unlock();
}

template<> void Component<B_DynamicBodyRef>::ComponentElem::clean()
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

// template<> void Component<Effect>::ComponentElem::init()
// {
//     data.expired = false;
// }