#include <SanctiaEntity.hpp>
#include <Globals.hpp>
#include <algorithm>
#include <Utils.hpp>
#include <Helpers.hpp>
#include <MathsUtils.hpp>
#include <GameGlobals.hpp>

#include <glm/gtx/string_cast.hpp>
#include <reactphysics3d/mathematics/Vector3.h>

COMPONENT_DEFINE_SYNCH(state3D) /**************** UNUSED TODO: remove*****************/
{
    if(
        child->has<RigidBody>() &&
        child->comp<RigidBody>()->getType() != rp3d::BodyType::KINEMATIC
        )
        return;

    auto &ps = parent.comp<state3D>();
    auto &cs = child->comp<state3D>();
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


    if(newParent.has<RigidBody>())
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
    else if(child->has<state3D>())
    {
        auto &s = child->comp<state3D>();

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

COMPONENT_DEFINE_REPARENT(state3D)
{
    auto &cs = child->comp<state3D>();

    if(!cs.usequat && child->has<EntityModel>() && child->comp<EntityModel>())
        child->comp<EntityModel>()->state.setRotation(directionToEuler(cs.lookDirection));

    if(!newParent.has<state3D>())
        return;

    auto &ps = newParent.comp<state3D>();
    

    quat ps_q = ps.usequat ? ps.quaternion : directionToQuat(ps.lookDirection);
    cs.position = ps.position + (ps_q * cs.initPosition);

    if(cs.usequat)
        cs.quaternion = ps_q * cs.initQuat;
    else
        cs.lookDirection = normalize(ps_q*vec3(1, 0, 0)*vec3(1, 0, 1));

    cs._PhysicTmpPos = cs.position;
    cs._PhysicTmpQuat = cs.quaternion;

    if(!newParent.has<RigidBody>() && child->has<RigidBody>())
    {
        auto &b = child->comp<RigidBody>();

        if(b->getType() != rp3d::BodyType::STATIC)
        {
            b->setIsActive(false);
            auto childBodyType = b->getType();
            b->setType(rp3d::BodyType::KINEMATIC);

            b->setTransform(
                rp3d::Transform(
                    PG::torp3d(cs.position), PG::torp3d(cs.usequat ? cs.quaternion : directionToQuat(cs.lookDirection))
                )
            );

            b->setType(childBodyType);

            child->set<RigidBody>(b);

            b->setIsActive(true);
        }

    }


}

COMPONENT_DEFINE_COMPATIBILITY_CHECK(RigidBody)
{
    return child->comp<RigidBody>()->getType() == parent.comp<RigidBody>()->getType();
}   

COMPONENT_DEFINE_COMPATIBILITY_CHECK(state3D)
{
    auto &childState = child->comp<state3D>();
    auto &parentState = parent.comp<state3D>();

    return childState.usequat == parentState.usequat;
}   

void setEntityStainStatusUniform(Entity *e)
{
    // NOTIF_MESSAGE(e->has<StainStatus>())
    if(e->comp<EntityModel>() and e->has<StainStatus>())
    {
        auto &m = e->comp<EntityModel>();
        auto &s = e->comp<StainStatus>();

        m->iterateOnAllMesh_Recursive([&](ModelRef mesh)
        {
            mesh->baseUniforms.add(ShaderUniform((vec3 *)&s, 25));
            // mesh->uniforms.add(ShaderUniform(&s.bloodyness, 26));
            // mesh->uniforms.add(ShaderUniform(&s.fatigue, 27));
        });
    }
}

void removeEntityStainStatusUniform(Entity *e)
{
    if(e->has<EntityModel>() and e->comp<EntityModel>() and e->has<StainStatus>())
    {
        auto &m = e->comp<EntityModel>();
        auto &s = e->comp<StainStatus>();

        m->iterateOnAllMesh_Recursive([&](ModelRef mesh)
        {
            for(auto i = mesh->uniforms.uniforms.begin(); i != mesh->uniforms.uniforms.end(); i++)
                if(i->getLocation() == 25){mesh->uniforms.uniforms.erase(i);break;}

            // for(auto i = mesh->uniforms.uniforms.begin(); i != mesh->uniforms.uniforms.end(); i++)
            //     if(i->getLocation() == 26){mesh->uniforms.uniforms.erase(i);break;}

            // for(auto i = mesh->uniforms.uniforms.begin(); i != mesh->uniforms.uniforms.end(); i++)
            //     if(i->getLocation() == 27){mesh->uniforms.uniforms.erase(i);break;}
        });
    }
}

COMPONENT_DEFINE_MERGE(EntityModel)
{
    if(!parent.has<EntityModel>())
    {
        parent.set<EntityModel>({newObjectGroup()});
    }

    auto childModel = child->comp<EntityModel>();
    auto &parentModel = parent.comp<EntityModel>();

    removeEntityStainStatusUniform(child.get());
    removeEntityStainStatusUniform(&parent);

    if(true)
    {
        mat4 t = inverse(parentModel->state.modelMatrix) * childModel->state.modelMatrix;

        childModel->state.setPosition(t[3]);
        childModel->state.setQuaternion(quat(mat3(t))); 

        parentModel->add(childModel);
    }

    setEntityStainStatusUniform(&parent);
}

COMPONENT_DEFINE_MERGE(RigidBody)
{
    auto &childBody = child->comp<RigidBody>();

    if(!parent.has<RigidBody>())
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

void setEntityModelStaticFlagUniform(Entity *e)
{
    if(e->has<EntityModel>() and e->comp<EntityModel>() and e->has<staticEntityFlag>())
    {
        auto &m = e->comp<EntityModel>();
        auto &f = e->comp<staticEntityFlag>();

        m->iterateOnAllMesh_Recursive([&](ModelRef mesh)
        {
            mesh->uniforms.add(ShaderUniform((int *)&f.isDYnamic, 24));
        });

        // NOTIF_MESSAGE(f.isDYnamic << "  " << e->toStr());
    }
}

template<> void Component<EntityModel>::ComponentElem::init()
{
    if(data.get())
    {
        // std::cout << "NON EMPTY ENTITY MODEL !!!!!\n";
        globals.getScene()->remove(data);
    }

    globals.getScene()->add(data);

    entity->set<StainStatus>(StainStatus());
    setEntityStainStatusUniform(entity);
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
    if(entity->has<EntityModel>())
    {
        entity->comp<EntityModel>()->setAnimation(&data);
    }
}

template<> void Component<Items>::ComponentElem::clean()
{
    for(auto i : data.equipped)
        i = {0, EntityRef()};
};

template<> void Component<state3D>::ComponentElem::clean()
{
    System<Target>([&](Entity &e)
    {
        if(e.comp<Target>().target == entity)
        {
            e.comp<Target>().target = nullptr;
        }
    });
};

template<> void Component<Target>::ComponentElem::clean()
{
    data.target = nullptr;
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

    vec3 laabbmax = 
        max(PG::toglm(WtoL * rp3d::Vector3(aabb.getMin().x, aabb.getMin().y, aabb.getMin().z)),
        max(PG::toglm(WtoL * rp3d::Vector3(aabb.getMin().x, aabb.getMin().y, aabb.getMax().z)),
        max(PG::toglm(WtoL * rp3d::Vector3(aabb.getMin().x, aabb.getMax().y, aabb.getMin().z)),
        max(PG::toglm(WtoL * rp3d::Vector3(aabb.getMin().x, aabb.getMax().y, aabb.getMax().z)),
        max(PG::toglm(WtoL * rp3d::Vector3(aabb.getMax().x, aabb.getMin().y, aabb.getMin().z)),
        max(PG::toglm(WtoL * rp3d::Vector3(aabb.getMax().x, aabb.getMin().y, aabb.getMax().z)),
        max(PG::toglm(WtoL * rp3d::Vector3(aabb.getMax().x, aabb.getMax().y, aabb.getMin().z)),
            PG::toglm(WtoL * rp3d::Vector3(aabb.getMax().x, aabb.getMax().y, aabb.getMax().z))
        )))))))
    ;

    vec3 laabbmin = 
        min(PG::toglm(WtoL * rp3d::Vector3(aabb.getMin().x, aabb.getMin().y, aabb.getMin().z)),
        min(PG::toglm(WtoL * rp3d::Vector3(aabb.getMin().x, aabb.getMin().y, aabb.getMax().z)),
        min(PG::toglm(WtoL * rp3d::Vector3(aabb.getMin().x, aabb.getMax().y, aabb.getMin().z)),
        min(PG::toglm(WtoL * rp3d::Vector3(aabb.getMin().x, aabb.getMax().y, aabb.getMax().z)),
        min(PG::toglm(WtoL * rp3d::Vector3(aabb.getMax().x, aabb.getMin().y, aabb.getMin().z)),
        min(PG::toglm(WtoL * rp3d::Vector3(aabb.getMax().x, aabb.getMin().y, aabb.getMax().z)),
        min(PG::toglm(WtoL * rp3d::Vector3(aabb.getMax().x, aabb.getMax().y, aabb.getMin().z)),
            PG::toglm(WtoL * rp3d::Vector3(aabb.getMax().x, aabb.getMax().y, aabb.getMax().z))
        )))))))
    ;


    std::vector<vec3> points;
    

    vec3 jump = (laabbmax - laabbmin)/25.f;

    // std::cout << to_string(laabbmin) << "\t" << to_string(laabbmax) << "\t" << to_string(jump) << "\n";

    #define AABB_FACE_RAYCAST(cur, dim1, dim2) \
        for(float dim1 = laabbmin. dim1; dim1 <= laabbmax. dim1; dim1 += jump. dim1) \
        for(float dim2 = laabbmin. dim2; dim2 <= laabbmax. dim2; dim2 += jump. dim2) \
        { \
            rp3d::RaycastInfo infos = rp3d::RaycastInfo(); \
            rp3d::RaycastInfo infos2 = rp3d::RaycastInfo(); \
            vec3 tmpa; tmpa. dim1 = dim1; tmpa. dim2 = dim2; tmpa. cur = laabbmin. cur; \
            vec3 tmpb; tmpb. dim1 = dim1; tmpb. dim2 = dim2; tmpb. cur = laabbmax. cur; \
            auto a = LtoW * rp3d::Vector3(PG::torp3d(tmpa)); \
            auto b = LtoW * rp3d::Vector3(PG::torp3d(tmpb)); \
            c->raycast(rp3d::Ray(a, b), infos); \
            if(infos.collider == c) points.push_back(PG::toglm(WtoL * infos.worldPoint)); \
            c->raycast(rp3d::Ray(b, a), infos2); \
            if(infos2.collider == c) points.push_back(PG::toglm(WtoL * infos2.worldPoint)); \
        }

    if(!isFeild)
    {
        assert(jump.x > 0 && jump.y > 0 && jump.z > 0);

        AABB_FACE_RAYCAST(z, x, y)

        AABB_FACE_RAYCAST(y, x, z)

        AABB_FACE_RAYCAST(x, y, z)

    }
    else
    {
        assert(jump.x > 0 && jump.z > 0);

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
        }
    }

    // std::cout << "Creating Physics helper of size " << points.size() << "\n";

    return PointsHelperRef(new PointsHelper(points, color));
}

template<> void Component<PhysicsHelpers>::ComponentElem::init()
{
    data = {newObjectGroup()};

    // if(entity->has<B_DynamicBodyRef>() && entity != GG::playerEntity.get())
    // {
    //     auto &b = entity->comp<B_DynamicBodyRef>();
    //     data->add(getModelFromCollider(b->boundingCollider, vec3(1, 1, 0)));
    // }

    // if(entity->ids[PHYSIC] != NO_ENTITY)
    // {
    //     Effect &l = entity->comp<Effect>();

    //     data->add(getModelFromCollider(l.zone, vec3(1, 0, 0)));
    // }

    if(entity->has<RigidBody>())
    {
        // std::cout << TERMINAL_NOTIF << entity->comp<EntityInfos>().name << "\n" << TERMINAL_RESET;

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
    if(!entity->has<EntityModel>()) return;

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
    
    if(entity->has<ItemInfos>())
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

    if(entity->has<EntityStats>())
    {
        auto &s = entity->comp<EntityStats>();
        ValueHelperRef<float> HP(new ValueHelper(s.health.cur, U"Health ", vec3(0, 1, 0)));
        ADD_VALUE_HELPER(HP)

        ValueHelperRef<float> ST(new ValueHelper(s.stamina.cur, U"Stamina ", vec3(1, 1, 0)));
        ADD_VALUE_HELPER(ST)
    }

    if(entity->has<NpcPcRelation>())
    {
        auto &r = entity->comp<NpcPcRelation>();
        ValueHelperRef<safeBoolOverload> K(new ValueHelper(*(safeBoolOverload*)&r.known, U"Know Player ", ColorHexToV(0x6668DE))); 
        ADD_VALUE_HELPER(K)

        ValueHelperRef<short> A(new ValueHelper(r.affinity, U"Affinity ", ColorHexToV(0x66B4DE)));
        ADD_VALUE_HELPER(A)
    }

    if(entity->has<ItemInfos>())
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
    if(!entity || entity->ids[GRAPHIC] < 0 || entity->ids[GRAPHIC] > MAX_ENTITY || !entity->has<EntityModel>()) return;

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
    if(data->getType() == rp3d::BodyType::STATIC && entity->has<state3D>())
    {
        auto &t = data->getTransform();
        auto &s = entity->comp<state3D>();
        s.usePhysicInterpolation = false;
        s.position = PG::toglm(t.getPosition());
        s.quaternion = PG::toglm(t.getOrientation());
        s._PhysicTmpQuat = s.quaternion;
        s._PhysicTmpPos = s.position;
        s.usequat = true;

        entity->set<staticEntityFlag>({false});
    }
    else
    {
        entity->set<staticEntityFlag>({true});
    }
    setEntityModelStaticFlagUniform(entity);
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
        e->ids[ComponentCategory::GRAPHIC] == NO_ENTITY &&
        e->ids[ComponentCategory::DATA]    == NO_ENTITY
        ;
    
    if(isPureUI)
    {
        // std::cout << e->toStr() << "IS PURE UI\n"; 

        activated = false;
        return;
    }

    activated = true;

    bool hasModel = e->has<EntityModel>();
    bool hasRigidBody = e->has<RigidBody>();
    bool hasChildren = e->has<EntityGroupInfo>() && e->comp<EntityGroupInfo>().children.size();

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
            if(c->has<LevelOfDetailsInfos>())
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
    if(parent.has<LevelOfDetailsInfos>())
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

COMPONENT_DEFINE_COMPATIBILITY_CHECK(Script)
{
    return false;
}



template<> void Component<StainStatus>::ComponentElem::init()
{
    // setEntityStainStatusUniform(entity);
}

template<> void Component<StainStatus>::ComponentElem::clean()
{
    removeEntityStainStatusUniform(entity);
}

// COMPONENT_DEFINE_MERGE(StainStatus)
// {
//     removeEntityStainStatusUniform(child)
// }

EntityRef spawnEntity(const std::string &name, vec3 spawnPoint)
{
    auto it = Loader<EntityRef>::loadingInfos.find(name);

    if(it == Loader<EntityRef>::loadingInfos.end())
    {
        FILE_ERROR_MESSAGE(name, "Entity not found.");
        return newEntity();
    }

    VulpineTextBuffRef file(new VulpineTextBuff(it->second->buff->getSource().c_str()));

    auto e = DataLoader<EntityRef>::read(file);

    if(e->has<state3D>())
    {
        e->comp<state3D>().useinit = true;
        e->comp<state3D>().initPosition = spawnPoint;
    }

    return e;
}


bool isVisible(Entity &a, Entity &b)
{
    if(a.has<state3D>() && b.has<state3D>())
    {
        return distance(a.comp<state3D>().position, b.comp<state3D>().position) < 100.f;
    }

    return true;
}

Entity* getClosestVisibleEnemy(Entity &e)
{
    auto &state3D1 = e.comp<state3D>();
    auto &faction1 = e.comp<Faction>();
    Entity *bestMatch = &e;
    float minDistance = 1e6f;

    System<state3D, Faction>([&](Entity &f){
        
        auto &state3D2 = f.comp<state3D>();
        auto &faction2 = f.comp<Faction>();

        if(
            f.ids[0] != e.ids[0] and
            Faction::areEnemy(faction1, faction2) and
            (!f.has<EntityStats>() or f.comp<EntityStats>().alive) and
            isVisible(e, f)
        )
        {
            float d = distance(state3D1.position, state3D2.position);

            // d += (float)(rand()%8)/8.f - 4.f;
    
            if(d < minDistance)
            {
                minDistance = d;
                bestMatch = &f;
            }
        }
    });

    return bestMatch;
}

Entity* getClosestVisibleAlly(Entity &e)
{
    auto &state3D1 = e.comp<state3D>();
    auto &faction1 = e.comp<Faction>();
    Entity *bestMatch = &e;
    float minDistance = 1e6f;

    System<state3D, Faction>([&](Entity &f){
        
        auto &state3D2 = f.comp<state3D>();
        auto &faction2 = f.comp<Faction>();

        if(
            !f.is(e) and 
            !Faction::areEnemy(faction1, faction2) and 
            (!f.has<EntityStats>() or f.comp<EntityStats>().alive) and
            isVisible(e, f))
        {
            float d = distance(state3D1.position, state3D2.position);
    
            if(d < minDistance)
            {
                minDistance = d;
                bestMatch = &f;
            }
        }
    });

    return bestMatch;
}