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

    rp3d::Transform t;

    auto &childBody = child->comp<RigidBody>();
    auto &parentBody = parent.comp<RigidBody>();
    
    auto &childTransform = childBody->getTransform();
    auto &parentTransform = parentBody->getTransform();

    t.setPosition(childTransform.getPosition() + parentTransform.getPosition());
    t.setOrientation(childTransform.getOrientation() * parentTransform.getOrientation());

    auto childBodyType = childBody->getType();
    childBody->setType(rp3d::BodyType::KINEMATIC);
    childBody->setTransform(t);
    childBody->setType(childBodyType);
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
    globals.getScene()->remove(child->comp<EntityModel>());

    // std::cout << "MERGING ENTITY MODEL FOR CHILD : " << child->comp<EntityInfos>().name << "\t"
    // << " AND PARENT " << parent.comp<EntityInfos>().name << "\n";

    if(!parent.hasComp<EntityModel>())
    {
        // parent.set<EntityModel>(child->comp<EntityModel>());
        parent.set<EntityModel>({newObjectGroup()});

        // std::cout << "PARENT COMP ENTITY MODEL WAS CREATED\n";
        // return;
    }

    auto childModel = child->comp<EntityModel>()->copy();
    // auto childModel = child->comp<EntityModel>();
    auto &parentModel = parent.comp<EntityModel>();

    auto &childState = child->comp<EntityState3D>();

    auto parentState = parent.hasComp<EntityState3D>() ? parent.comp<EntityState3D>() : EntityState3D();

    

    /* TODO : investigate if rotation need to be merged too */
    // if(childState.usequat)
    // {

    // }

    // if(parent.hasComp<EntityModel>())
    if(!parentModel->getChildren().size())
    {
        childModel->state.setPosition(childState.position - parentState.position);

        parentModel->add(childModel);
        globals.getScene()->add(childModel);
        // std::cout << glm::to_string(childModel->state.position) << "\n";
    
        // std::cout << "SIMPLE ENTITY MODEL MERGE\n";
    }
    else 
    {
        // std::cout << "BATCHING ENTITY MODEL MERGE\n";

        auto mesh1 = parentModel->getChildren()[0]->getMeshes()[0];
        auto mesh2 = childModel->getMeshes()[0];

        auto vao1 = mesh1->getVao();
        auto vao2 = mesh2->getVao();

        auto &m1 = vao1->attributes[0];
        auto &m2 = vao2->attributes[0];

        // m1->getVao();
        int vcount = m2.getVertexCount();
        
        GenericSharedBuffer nbuff(new char[sizeof(ivec4)*(m1.getVertexCount() + m2.getVertexCount())]);
        memcpy(nbuff.get(), m1.getBufferAddr(), sizeof(ivec4)*m1.getVertexCount());

        // childModel->state.update();
        mat4 pt = mesh1->state.modelMatrix;
        mat4 ct = mesh2->state.modelMatrix;
        mat4 transform = inverse(pt) * ct;

        for(int i = 0; i < vcount; i++)
        {
            ivec4 v = ((ivec4*)m2.getBufferAddr())[i];

            

            vec3 modelPosition = vec3(ivec3(ivec3(v) & (0x00FFFFFF)) - 0x800000)*1e-3f;
            // std::cout << to_string(modelPosition)<< "\t";

            modelPosition = vec3(transform * vec4(modelPosition, 1));
            // modelPosition = vec3(childModel->state.modelMatrix * vec4(modelPosition, 1));
            // modelPosition = modelPosition + childModel->state.position;
            // modelPosition = modelPosition + vec3(1, 0, 0);
            // std::cout << to_string(modelPosition) << "\n";

            ivec3 packedPosition = (ivec3(modelPosition*1e3f) + 0x800000) 
            // & (0x00FFFFFF)
            ;
            // std::cout << to_string(vec3(packedPosition - 0x800000)*1e-3f) << "\n";

            v.x &= ~(0x00FFFFFF);
            v.x |= packedPosition.x;

            v.y &= ~(0x00FFFFFF);
            v.y |= packedPosition.y;

            v.z &= ~(0x00FFFFFF);
            v.z |= packedPosition.z;

            ((ivec4*)nbuff.get())[i + m1.getVertexCount()] = v;
        }

        // m1.updateData(nbuff, m1.getVertexCount() + m2.getVertexCount());

        MeshVao finalVao(new VertexAttributeGroup({
            VertexAttribute(nbuff, 0, m1.getVertexCount() + m2.getVertexCount(), 4, GL_UNSIGNED_INT, false)
        }));

        GenericSharedBuffer nfaces(new char[sizeof(ivec3)*(vao1.nbFaces/3 + vao2.nbFaces/3)]);
        memcpy(nfaces.get(), vao1.faces.get(), sizeof(ivec3)*vao1.nbFaces/3);
        // memset(nfaces.get(), 0, sizeof(ivec3)*vao1.nbFaces/3);
        
        for(int i = 0; i < vao2.nbFaces/3; i++)
        {
            ((ivec3*)nfaces.get())[i + vao1.nbFaces/3] = ((ivec3*)vao2.faces.get())[i] + (int)m1.getVertexCount();

            // ((ivec3*)nfaces.get())[i + vao1.nbFaces/3] = ((ivec3*)vao2.faces.get())[i];

            // std::cout << 
            // // ((int*)vao2.get())[i]
            // glm::to_string(((ivec3*)vao2.faces.get())[i]) << "\t" <<
            // glm::to_string(((ivec3*)nfaces.get())[i + vao1.nbFaces/3]) 
            // << "\n";
        }

        // for(int i = 0; i < 10; i++)
        // {
        //     std::cout << 
        //     ((int*)vao2.faces.get())[i]
        //     // glm::to_string(((ivec3*)vao2.get())[i]) << "\t" <<
        //     // glm::to_string(((ivec3*)nfaces.get())[i + vao1.nbFaces/3]) 
        //     << "\n";
        // }

        // vao1.faces = nfaces;
        // vao1.nbFaces = vao1.nbFaces + vao2.nbFaces;
        // vao1->generate();

        finalVao.faces = nfaces;
        finalVao.nbFaces = vao1.nbFaces + vao2.nbFaces;
        mesh1->setVao(finalVao);
    }
}

COMPONENT_DEFINE_MERGE(RigidBody)
{
    // std::cout << "MERGING RIGIDBODY\n";
    auto &childBody = child->comp<RigidBody>();

    if(!parent.hasComp<RigidBody>())
    {
        // std::cout << "OVERWRITTING\n";
        // parent.set<RigidBody>(child->comp<RigidBody>());
        parent.set<RigidBody>(PG::world->createRigidBody(
            // childBody->getTransform()
            rp3d::Transform()
            ));
        parent.comp<RigidBody>()->setType(childBody->getType());
        // return;
    }
    // return;

    auto &parentBody = parent.comp<RigidBody>();
    
    auto &childTransform = childBody->getTransform();
    auto &parentTransform = parentBody->getTransform();

    int childCnb = childBody->getNbColliders();

    for(int i = 0; i < childCnb; i++)
    {
        auto collider = childBody->getCollider(i);

        auto t = collider->getLocalToWorldTransform();

        t.setPosition(t.getPosition() - parentTransform.getPosition());
        auto newCollider = parentBody->addCollider(collider->getCollisionShape(), t);

        newCollider->setCollisionCategoryBits(collider->getCollisionCategoryBits());
        newCollider->setCollideWithMaskBits(collider->getCollideWithMaskBits());
        newCollider->setIsTrigger(collider->getIsTrigger());
        newCollider->setMaterial(collider->getMaterial());
        newCollider->setLocalToBodyTransform(t);
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

            if(infos.collider)
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

            if(infos.collider)
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

            if(infos.collider)
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

            if(infos.collider)
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

            if(infos.collider)
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

            if(infos.collider)
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

            if(infos.collider)
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
