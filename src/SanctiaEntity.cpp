#include <SanctiaEntity.hpp>
#include <Globals.hpp>
#include <algorithm>
#include <Utils.hpp>
#include <Helpers.hpp>
#include <MathsUtils.hpp>
#include <GameGlobals.hpp>

#include <glm/gtx/string_cast.hpp>


COMPONENT_DEFINE_SYNCH(EntityState3D) /**************** UNUSED TODO: remove*****************/
{
    if(
        child->hasComp<RigidBody>() &&
        child->comp<RigidBody>()->getType() != rp3d::BodyType::KINEMATIC
        )
        return;

    auto &ps = parent.comp<EntityState3D>();
    auto &cs = child->comp<EntityState3D>();
    cs.position = cs.initPosition + ps.position;

    if(ps.usequat)
    {
        if(cs.usequat)
        {
            cs.quaternion = ps.quaternion * cs.quaternion;
        }
        else
        {
            /*
                TODO : maybe do this part
            */  
        }
    }
    else
    {
        if(cs.usequat)
        {
            cs.quaternion = directionToQuat(ps.lookDirection)*cs.initQuat;
            // std::cout << glm::to_string(cs.quaternion) << "\n";
        }
        else
        {
            /*
                TODO : maybe do this part
            */
        }
    }

    cs._PhysicTmpPos = cs.position;
    cs._PhysicTmpQuat = cs.quaternion;
}

COMPONENT_DEFINE_SYNCH(RigidBody) /**************** UNUSED TODO: remove*****************/
{
    quat parentQuat = PG::toglm(parent.comp<RigidBody>()->getTransform().getOrientation());
    quat childQuat = PG::toglm(child->comp<RigidBody>()->getTransform().getOrientation());

    vec3 parentPos = PG::toglm(parent.comp<RigidBody>()->getTransform().getPosition());
    vec3 childPos = PG::toglm(child->comp<RigidBody>()->getTransform().getPosition());

    childQuat = parentQuat * childQuat;
    childPos += parentPos;

    auto &b = child->comp<RigidBody>();

    b->setTransform(
        rp3d::Transform(PG::torp3d(childPos), PG::torp3d(childQuat))
    );

    // std::cout << to_string(childPos) << "\n";
}

COMPONENT_DEFINE_REPARENT(RigidBody)
{
    if(&parent != &newParent) return;

    auto &childBody = child->comp<RigidBody>();


    if(newParent.hasComp<RigidBody>())
    {
        auto childBodyType = childBody->getType();
        childBody->setType(rp3d::BodyType::KINEMATIC);

        auto &parentBody = newParent.comp<RigidBody>();

        childBody->setTransform(
            parentBody->getTransform() * childBody->getTransform()
        );

        childBody->setType(childBodyType);

        child->set<RigidBody>(childBody);
    }
    else if(child->hasComp<EntityState3D>())
    {
        auto &s = child->comp<EntityState3D>();

        auto cbtype = childBody->getType();
        
        childBody->setIsActive(false);
        childBody->setType(rp3d::BodyType::KINEMATIC);
        
        childBody->setTransform(
            rp3d::Transform(
                PG::torp3d(s.position), PG::torp3d(s.usequat ? s.quaternion : directionToQuat(s.lookDirection))
            )
        );
        
        childBody->setType(cbtype);
        childBody->setIsActive(true);
    }

    // child->set<RigidBody>(childBody);
}   

COMPONENT_DEFINE_REPARENT(EntityState3D)
{
    // if(
    //     child->hasComp<RigidBody>() &&
    //     child->comp<RigidBody>()->getType() != rp3d::BodyType::KINEMATIC
    //     )
    //     return;

    if(!newParent.hasComp<EntityState3D>())
        return;

    auto &ps = newParent.comp<EntityState3D>();
    auto &cs = child->comp<EntityState3D>();
    cs.position = cs.initPosition + ps.position;

    if(ps.usequat)
    {
        if(cs.usequat)
        {
            cs.quaternion = ps.quaternion * cs.initQuat;
        }
        else
        {
            /*
                TODO : maybe do this part
            */  
        }
    }
    else
    {
        if(cs.usequat)
        {
            cs.quaternion = directionToQuat(ps.lookDirection)*cs.initQuat;
            // std::cout << glm::to_string(cs.quaternion) << "\n";
        }
        else
        {
            /*
                TODO : maybe do this part
            */
        }
    }

    cs._PhysicTmpPos = cs.position;
    cs._PhysicTmpQuat = cs.quaternion;


    if(!newParent.hasComp<RigidBody>() && child->hasComp<RigidBody>())
    {
        auto &b = child->comp<RigidBody>();

        if(b->getType() != rp3d::BodyType::STATIC)
        {
            auto childBodyType = b->getType();
            b->setType(rp3d::BodyType::KINEMATIC);

            b->setTransform(
                rp3d::Transform(
                    PG::torp3d(cs.position), PG::torp3d(cs.quaternion)
                )
            );

            b->setType(childBodyType);

            child->set<RigidBody>(b);
        }

    }
}

COMPONENT_DEFINE_COMPATIBILITY_CHECK(RigidBody)
{
    return child->comp<RigidBody>()->getType() == parent.comp<RigidBody>()->getType();
}   

COMPONENT_DEFINE_COMPATIBILITY_CHECK(EntityState3D)
{
    auto &childState = child->comp<EntityState3D>();
    auto &parentState = parent.comp<EntityState3D>();

    return childState.usequat == parentState.usequat;
}   

COMPONENT_DEFINE_MERGE(EntityModel)
{
    if(!parent.hasComp<EntityModel>())
    {
        parent.set<EntityModel>({newObjectGroup()});
    }

    auto childModel = child->comp<EntityModel>();
    auto &parentModel = parent.comp<EntityModel>();

    if(true)
    {
        mat4 t = inverse(parentModel->state.modelMatrix) * childModel->state.modelMatrix;

        childModel->state.setPosition(t[3]);
        childModel->state.setQuaternion(quat(mat3(t))); 

        parentModel->add(childModel);
    }
}

COMPONENT_DEFINE_MERGE(RigidBody)
{
    auto &childBody = child->comp<RigidBody>();

    if(!parent.hasComp<RigidBody>())
    {
        parent.set<RigidBody>(PG::world->createRigidBody(
            // childBody->getTransform()
            rp3d::Transform()
            ));
        parent.comp<RigidBody>()->setType(childBody->getType());
    }

    auto &parentBody = parent.comp<RigidBody>();
    
    auto &childTransform = childBody->getTransform();
    auto &parentTransform = parentBody->getTransform();

    int childCnb = childBody->getNbColliders();

    for(int i = 0; i < childCnb; i++)
    {
        auto collider = childBody->getCollider(i);

        rp3d::Transform transform = parentTransform.getInverse() * collider->getLocalToWorldTransform();
        auto newCollider = parentBody->addCollider(collider->getCollisionShape(), transform);
        newCollider->setCollisionCategoryBits(collider->getCollisionCategoryBits());
        newCollider->setCollideWithMaskBits(collider->getCollideWithMaskBits());
        newCollider->setIsTrigger(collider->getIsTrigger());
        newCollider->setMaterial(collider->getMaterial());
    }

    parentBody->updateLocalCenterOfMassFromColliders();
}

template<> void Component<EntityModel>::ComponentElem::init()
{
    if(data.get())
    {
        // std::cout << "NON EMPTY ENTITY MODEL !!!!!\n";
        globals.getScene()->remove(data);
    }

    globals.getScene()->add(data);
};

template<> void Component<EntityModel>::ComponentElem::clean()
{
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

template<> void Component<Items>::ComponentElem::clean()
{
    for(auto i : data.equipped)
        i = {0, EntityRef()};
};

ModelRef getModelFromCollider(rp3d::Collider* c, vec3 color)
{
    // {
    //     rp3d::AABB aabb = c->getWorldAABB();

    //     // c->getLocalToWorldTransform();    

    //     vec3 aabbmin = PG::toglm(aabb.getMin());
    //     vec3 aabbmax = PG::toglm(aabb.getMax()) - aabbmin;

    //     vec3 laabbmin_tmp = PG::toglm(c->getLocalToWorldTransform().getInverse() * aabb.getMin());
    //     vec3 laabbmax_tmp = PG::toglm(c->getLocalToWorldTransform().getInverse() * aabb.getMax());

    //     vec3 laabbmax = max(laabbmax_tmp, laabbmin_tmp);
    //     vec3 laabbmin = min(laabbmax_tmp, laabbmin_tmp);


    //     std::vector<vec3> points;

    //     int res = max(aabbmax.x, max(aabbmax.y, aabbmax.z))*10;

    //     float resdiv = 1.f/(res-1.f);
    //     bool voxels[res][res][res];

    //     for(int i = 0; i < res; i++)
    //     for(int j = 0; j < res; j++)
    //     for(int k = 0; k < res; k++)
    //     {
    //         vec3 pos = aabbmin + aabbmax*vec3(i, j, k)*resdiv - sign(vec3(i, j, k) - res*0.5f)*resdiv*0.1f;
            
    //         voxels[i][j][k] = c->testPointInside(PG::torp3d(pos));
    //     }

    //     for(int i = 0; i < res; i++)
    //     for(int j = 0; j < res; j++)
    //     for(int k = 0; k < res; k++)
    //     if(voxels[i][j][k])
    //     {
    //         if(!j || !i || !k || i == res-1 || j == res-1 || k == res-1)
    //             points.push_back(laabbmin + laabbmax*vec3(i, j, k)*resdiv);

    //         else if(!voxels[i-1][j][k] || !voxels[i+1][j][k] ||
    //                 !voxels[i][j-1][k] || !voxels[i][j+1][k] ||
    //                 !voxels[i][j][k-1] || !voxels[i][j][k+1]
    //         )
    //             points.push_back(laabbmin + laabbmax*vec3(i, j, k)*resdiv);
    //     }


    //     std::cout << "Creating Physics helper of size " << points.size() << "\n";

    //     return PointsHelperRef(new PointsHelper(points, color));
    // }


    rp3d::AABB aabb = c->getWorldAABB();
    vec3 aabbmin = PG::toglm(aabb.getMin());
    vec3 aabbmax = PG::toglm(aabb.getMax()) - aabbmin;

    bool isFeild = c->getCollisionShape()->getName() == reactphysics3d::CollisionShapeName::HEIGHTFIELD;
    
    auto WtoL = c->getLocalToWorldTransform().getInverse();
    auto LtoW = c->getLocalToWorldTransform();

    vec3 laabbmin_tmp = PG::toglm(WtoL * aabb.getMin()) - 0.1f;
    vec3 laabbmax_tmp = PG::toglm(WtoL * aabb.getMax()) + 0.1f;

    vec3 laabbmax = max(laabbmax_tmp, laabbmin_tmp);
    vec3 laabbmin = min(laabbmax_tmp, laabbmin_tmp);

    std::vector<vec3> points;
    

    vec3 jump = (laabbmax - laabbmin)/25.f;

    int cnt = 0;

    std::cout << to_string(laabbmin) << "\t" << to_string(laabbmax) << "\t" << to_string(jump) << "\n";

    assert(jump.x > 0 && jump.y > 0 && jump.z > 0);

    if(!isFeild)
    {
        for(float x = laabbmin.x; x <= laabbmax.x; x += jump.x)
        for(float y = laabbmin.y; y <= laabbmax.y; y += jump.y)
        {
            rp3d::RaycastInfo infos = rp3d::RaycastInfo();

            auto a = LtoW * rp3d::Vector3(x, y, laabbmin.z);
            auto b = LtoW * rp3d::Vector3(x, y, laabbmax.z);

            c->raycast(rp3d::Ray(a, b), infos);

            if(infos.collider == c)
                points.push_back(PG::toglm(WtoL * infos.worldPoint));

            cnt ++;
        }

        for(float x = laabbmin.x; x <= laabbmax.x; x += jump.x)
        for(float y = laabbmin.y; y <= laabbmax.y; y += jump.y)
        {
            rp3d::RaycastInfo infos = rp3d::RaycastInfo();

            auto a = LtoW * rp3d::Vector3(x, y, laabbmin.z);
            auto b = LtoW * rp3d::Vector3(x, y, laabbmax.z);

            c->raycast(rp3d::Ray(b, a), infos);

            if(infos.collider == c)
                points.push_back(PG::toglm(WtoL * infos.worldPoint));

            cnt ++;
        }

        for(float x = laabbmin.x; x <= laabbmax.x; x += jump.x)
        for(float z = aabbmin.z; z <= aabbmax.z; z += jump.z)
        {
            rp3d::RaycastInfo infos = rp3d::RaycastInfo();

            auto a = LtoW * rp3d::Vector3(x, laabbmin.y, z);
            auto b = LtoW * rp3d::Vector3(x, laabbmax.y, z);

            c->raycast(rp3d::Ray(a, b), infos);

            if(infos.collider == c)
                points.push_back(PG::toglm(WtoL * infos.worldPoint));

            cnt ++;
        }

        for(float x = laabbmin.x; x <= laabbmax.x; x += jump.x)
        for(float z = laabbmin.z; z <= laabbmax.z; z += jump.z)
        {
            rp3d::RaycastInfo infos = rp3d::RaycastInfo();

            auto a = LtoW * rp3d::Vector3(x, laabbmin.y, z);
            auto b = LtoW * rp3d::Vector3(x, laabbmax.y, z);

            c->raycast(rp3d::Ray(b, a), infos);

            if(infos.collider == c)
                points.push_back(PG::toglm(WtoL * infos.worldPoint));

            cnt ++;
        }

        for(float z = laabbmin.z; z <= laabbmax.z; z += jump.z)
        for(float y = laabbmin.y; y <= laabbmax.y; y += jump.y)
        {
            rp3d::RaycastInfo infos = rp3d::RaycastInfo();

            auto a = LtoW * rp3d::Vector3(laabbmin.x, y, z);
            auto b = LtoW * rp3d::Vector3(laabbmax.x, y, z);

            c->raycast(rp3d::Ray(a, b), infos);

            if(infos.collider == c)
                points.push_back(PG::toglm(WtoL * infos.worldPoint));

            cnt ++;
        }

        for(float z = laabbmin.z; z <= laabbmax.z; z += jump.z)
        for(float y = laabbmin.y; y <= laabbmax.y; y += jump.y)
        {
            rp3d::RaycastInfo infos = rp3d::RaycastInfo();

            auto a = LtoW * rp3d::Vector3(laabbmin.x, y, z);
            auto b = LtoW * rp3d::Vector3(laabbmax.x, y, z);

            c->raycast(rp3d::Ray(b, a), infos);

            if(infos.collider == c)
                points.push_back(PG::toglm(WtoL * infos.worldPoint));

            cnt ++;
        }
    }
    else
    {
        jump /= 4.0;

        for(float x = laabbmin.x; x <= laabbmax.x; x += jump.x)
        for(float z = laabbmin.z; z <= laabbmax.z; z += jump.z)
        {
            rp3d::RaycastInfo infos = rp3d::RaycastInfo();

            auto a = LtoW * rp3d::Vector3(x, laabbmin.y, z);
            auto b = LtoW * rp3d::Vector3(x, laabbmax.y, z);

            c->raycast(rp3d::Ray(b, a), infos);

            if(infos.collider == c)
                points.push_back(PG::toglm(WtoL * infos.worldPoint));

            cnt ++;
        }
    }

    std::cout << "Creating Physics helper of size " << points.size() << "\n";

    return PointsHelperRef(new PointsHelper(points, color));
}

template<> void Component<PhysicsHelpers>::ComponentElem::init()
{
    data = {newObjectGroup()};

    // if(entity->hasComp<B_DynamicBodyRef>() && entity != GG::playerEntity.get())
    // {
    //     auto &b = entity->comp<B_DynamicBodyRef>();
    //     data->add(getModelFromCollider(b->boundingCollider, vec3(1, 1, 0)));
    // }

    // if(entity->ids[PHYSIC] != NO_ENTITY)
    // {
    //     Effect &l = entity->comp<Effect>();

    //     data->add(getModelFromCollider(l.zone, vec3(1, 0, 0)));
    // }

    if(entity->hasComp<RigidBody>())
    {
        std::cout << TERMINAL_NOTIF << entity->comp<EntityInfos>().name << "\n" << TERMINAL_RESET;

        RigidBody b = entity->comp<RigidBody>();

        const int nb = b->getNbColliders();

        auto tmp = b->getTransform();
        b->setTransform(rp3d::Transform(tmp.getPosition(), rp3d::Quaternion::identity()));

        for(int i = 0; i < nb; i++)
        {
            auto c = b->getCollider(i);

            vec3 color = vec3(1, 0.5, 0);
            if(c->getIsTrigger())
                color = vec3(0.5, 1, 0);
            if(b->getType() == rp3d::BodyType::STATIC)
                color = vec3(0.85, 0.1, 0.1);

            auto m = getModelFromCollider(c, color);

            auto t = c->getLocalToBodyTransform();
            m->state.setPosition(PG::toglm(t.getPosition()));
            m->state.setQuaternion(PG::toglm(t.getOrientation()));

            data->add(m);
        }

        b->setTransform(tmp);
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
        ValueHelperRef<float> M(new ValueHelper(i.damageMultiplier, U"Damage Mult ", ColorHexToV(0xf04a1d)));
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

template<> void Component<RigidBody>::ComponentElem::init()
{
    int size = data->getNbColliders();
    for(int i = 0; i < size; i++)
    {
        data->getCollider(i)->setUserData(entity);
    }

    /* TODO : maybe remove this and make a special static rigid body component */
    if(data->getType() == rp3d::BodyType::STATIC && entity->hasComp<EntityState3D>())
    {
        auto &t = data->getTransform();
        auto &s = entity->comp<EntityState3D>();
        s.usePhysicInterpolation = false;
        s.position = PG::toglm(t.getPosition());
        s.quaternion = PG::toglm(t.getOrientation());
        s._PhysicTmpQuat = s.quaternion;
        s._PhysicTmpPos = s.position;
        s.usequat = true;
    }
    else
        entity->set<staticEntityFlag>({true});
}

template<> void Component<RigidBody>::ComponentElem::clean()
{
    if(data)
        PG::world->destroyRigidBody(data);
}


void LevelOfDetailsInfos::computeEntityAABB(Entity *e)
{
    
    bool isPureUI = 
        e->ids[ComponentCategory::UI]      != NO_ENTITY &&
        e->ids[ComponentCategory::AI]      == NO_ENTITY &&
        e->ids[ComponentCategory::PHYSIC]  == NO_ENTITY &&
        e->ids[ComponentCategory::GRAPHIC] == NO_ENTITY
        ;
    
    if(isPureUI)
    {
        // std::cout << e->toStr() << "IS PURE UI\n"; 

        activated = false;
        return;
    }

    activated = true;

    bool hasModel = e->hasComp<EntityModel>();
    bool hasRigidBody = e->hasComp<RigidBody>();
    bool hasChildren = e->hasComp<EntityGroupInfo>() && e->comp<EntityGroupInfo>().children.size();

    aabbmin = vec3(1e12);
    aabbmax = vec3(-1e12);

    if(hasModel)
    {
        auto &m = e->comp<EntityModel>();

        auto aabb = m->getMeshesBoundingBox();

        aabbmin = min(aabbmin, aabb.first);
        aabbmax = max(aabbmax, aabb.second);
    }
    // else 
    if(hasRigidBody)
    {
        auto &b = e->comp<RigidBody>();

        if(b->isActive())
        {
            auto aabb = b->getAABB();

            aabbmin = min(aabbmin, PG::toglm(aabb.getMin()));
            aabbmax = max(aabbmax, PG::toglm(aabb.getMax()));
        }

    }
    // else 
    if(hasChildren)
    {
        for(auto c : e->comp<EntityGroupInfo>().children)
            if(c->hasComp<LevelOfDetailsInfos>())
            {
                auto lodi = c->comp<LevelOfDetailsInfos>();

                aabbmin = min(aabbmin, lodi.aabbmin);
                aabbmax = max(aabbmax, lodi.aabbmax);
            }
    }

    // std::cout 
    //     << e->toStr() 
    //     << hasModel << "\n" << hasRigidBody << "\n" << hasChildren << "\n"
    //     << "has AABB " << to_string(aabbmin) << " " << to_string(aabbmax) << "\n\n"; 
}

void LevelOfDetailsInfos::computeLevel(vec3 p)
{
    vec3 closest = clamp(p, aabbmin, aabbmax);
    bool inside = all(greaterThanEqual(p, aabbmin) && lessThanEqual(p, aabbmax));
    float dist = inside ? 0.f : length(closest - p);


    switch (level)
    {
    case 0 :
        
        if(dist > distLevelFar + distLevelBias)
            level = 2;
        else
        if(dist > distLevelNear + distLevelBias)
            level = 1;

        break;
    
    case 1 : 

        if(dist < distLevelNear - distLevelBias)
            level = 0;
        else
        if(dist > distLevelFar + distLevelBias)
            level = 2;

        break;

    case 2 : 

        if(dist < distLevelNear - distLevelBias)
            level = 0;
        else
        if(dist < distLevelFar - distLevelBias)
            level = 1;

        break;

    default:
        break;
    }
}

template<> void Component<LevelOfDetailsInfos>::ComponentElem::init()
{
    data.computeEntityAABB(entity);
}

COMPONENT_DEFINE_REPARENT(LevelOfDetailsInfos)
{
    child->comp<LevelOfDetailsInfos>().computeEntityAABB(child.get());
    parent.comp<LevelOfDetailsInfos>().computeEntityAABB(&parent);
}

COMPONENT_DEFINE_MERGE(LevelOfDetailsInfos)
{
    if(parent.hasComp<LevelOfDetailsInfos>())
    {
        auto &clodi = child->comp<LevelOfDetailsInfos>();
        auto &plodi = parent.comp<LevelOfDetailsInfos>();

        plodi.aabbmin = min(plodi.aabbmin, clodi.aabbmin);
        plodi.aabbmax = max(plodi.aabbmax, clodi.aabbmax);
        
        for(auto &i : clodi.childrenLoadInfos)
            plodi.childrenLoadInfos.push_back(i);
    }
}

COMPONENT_DEFINE_SYNCH(LevelOfDetailsInfos)
{
    if(&parent != child.get()) return;

    auto &clodi = child->comp<LevelOfDetailsInfos>();

    if(!clodi.activated) return;

    clodi.computeLevel(globals.currentCamera->getPosition());

    // std::cout << child->toStr() << "LOD level : " << clodi.level << "\n\n";

    

}
