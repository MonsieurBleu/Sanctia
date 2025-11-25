#include "ECS/ComponentTypeScripting.hpp"
#include "Graphics/ObjectGroup.hpp"
#include "Matrix.hpp"
#include <SanctiaEntity.hpp>
#include <AssetManager.hpp>
#include <AssetManagerUtils.hpp>
#include <MappedEnum.hpp>
#include <GameConstants.hpp>

DATA_WRITE_FUNC_INIT(Faction);
    out->Entry();
    WRITE_NAME(type, out);
    out->write(CONST_STRING_SIZED(Faction::TypeReverseMap[data.type]));
DATA_WRITE_END_FUNC

DATA_READ_FUNC(Faction)
{
    DATA_READ_INIT(Faction);
    buff->read();
    buff->read();
    MAP_SAFE_READ(Faction::TypeMap, buff, data.type, buff->read());
    buff->read();
    DATA_READ_END;
}

AUTOGEN_DATA_RW_FUNC(ActionState 
    , lockedDirection
    , isTryingToAttack
    , isTryingToBlock
    , stun
    , blocking
    , attacking
    , lockedMaxSpeed
    )

AUTOGEN_DATA_READ_FUNC(EntityState3D 
    , position
    , quaternion
    , lookDirection
    , usequat
    , useinit
    , initPosition
    , initQuat
    , initPosition
    , initLookDirection
    )

DATA_WRITE_FUNC_INIT(EntityState3D)

    if(data.position != vec3(0))
        FTXTP_WRITE_ELEMENT(data, position);

    if(data.usequat)
    {
        if(data.useinit)
        {
            if(data.initQuat != quat(1, 0, 0, 0))
                FTXTP_WRITE_ELEMENT(data, initQuat);
            if(data.initPosition != vec3(0))
                FTXTP_WRITE_ELEMENT(data, initPosition);
        }

        if(data.quaternion != quat(1, 0, 0, 0))
            FTXTP_WRITE_ELEMENT(data, quaternion);
    }
    else
    {
        if(data.useinit)
        {
            FTXTP_WRITE_ELEMENT(data, initLookDirection);
            FTXTP_WRITE_ELEMENT(data, initPosition);
        }

        FTXTP_WRITE_ELEMENT(data, lookDirection);
    }

    FTXTP_WRITE_ELEMENT(data, usequat);
    FTXTP_WRITE_ELEMENT(data, useinit);

DATA_WRITE_END_FUNC

AUTOGEN_DATA_RW_FUNC(EntityDeplacementState
    , speed
    , wantedSpeed
    , deplacementDirection
    , wantedDepDirection
    , grounded
    , walking
    , isJumping
    , walkSpeed
    , sprintSpeed
    , airSpeed
    , friction
    , stopspeed
    , ground_accelerate
    , air_accelerate
    , gravity
    , jumpVelocity
    )

/* statBars aren't special nor complexe, it's prettier for them to be anonymous when written */
AUTOGEN_DATA_RW_FUNC_AN(statBar, min, max, cur)

DATA_WRITE_FUNC_INIT(EntityStats)

    FTXTP_WRITE_ELEMENT(data, alive);

    out->Entry();
    WRITE_NAME(health, out);
    DataLoader<statBar>::write(data.health, out);

    out->Entry();
    WRITE_NAME(stamina, out);
    DataLoader<statBar>::write(data.stamina, out);

    out->Entry();
    out->Tabulate();
    WRITE_NAME(resistances, out);
    for(int i = 0; i < DamageType_Size; i++)
    {
        out->Entry();
        out->write(CONST_STRING_SIZED(DamageTypeReverseMap[i]));
        out->write(" ", 1);
        FastTextParser::write<float>(data.resistances[i], out->getReadHead());
    }

    out->Break();

DATA_WRITE_END_FUNC

DATA_READ_FUNC_INIT(EntityStats)

    IF_MEMBER_FTXTP_LOAD(data, alive)

    else IF_MEMBER(health)
        data.health = DataLoader<statBar>::read(buff);

    else IF_MEMBER(stamina)
        data.stamina = DataLoader<statBar>::read(buff);

    else IF_MEMBER(resistances)
    {
        while(NEW_VALUE)
        {
            int id = -1;
            MAP_SAFE_READ(DamageTypeMap, buff, id, buff->read());

            value = buff->read();

            if(id == -1) continue;

            data.resistances[id] = FastTextParser::read<float>(value);
        }
    }

DATA_READ_END_FUNC


DATA_WRITE_FUNC_INIT(CharacterDialogues)

    out->Entry();
    WRITE_NAME(filename, out);
    out->write("\"", 1);
    out->write(CONST_STRING_SIZED(data.filename));
    out->write("\"", 1);

    out->Entry();
    WRITE_NAME(name, out);
    out->write("\"", 1);
    out->write(CONST_STRING_SIZED(data.name));
    out->write("\"", 1);

DATA_WRITE_END_FUNC

DATA_READ_FUNC_INIT(CharacterDialogues)

    IF_MEMBER_READ_VALUE(filename)
        data.filename = std::string(value);

    else IF_MEMBER_READ_VALUE(name)
        data.name = std::string(value);

DATA_READ_END_FUNC

AUTOGEN_DATA_RW_FUNC(NpcPcRelation, known, affinity);

DATA_WRITE_FUNC_INIT(ItemInfos)
    FTXTP_WRITE_ELEMENT(data, price);
    FTXTP_WRITE_ELEMENT(data, damageMultiplier);
    out->Entry();
    WRITE_NAME(damageType, out);
    out->write(CONST_STRING_SIZED(DamageTypeReverseMap[data.dmgType]));
DATA_WRITE_END_FUNC

DATA_READ_FUNC_INIT(ItemInfos)
    IF_MEMBER_FTXTP_LOAD(data, price)
    else IF_MEMBER_FTXTP_LOAD(data, damageMultiplier)
    else IF_MEMBER(damageType)
        MAP_SAFE_READ(DamageTypeMap, buff, data.dmgType, buff->read())
DATA_READ_END_FUNC

DATA_WRITE_FUNC_INIT(Items)

    for(uint64_t i = 0; i < sizeof(data.equipped)/sizeof(Items::Equipement); i++)
        if(data.equipped[i].item)
        {
            out->Entry();
            out->write(CONST_STRING_SIZED(EquipementSlotsReverseMap[i]));
            out->Tabulate();

            out->Entry();
            WRITE_NAME(boneAttachementID, out);
            FastTextParser::write<uint>(data.equipped[i].id, out->getReadHead());

            out->Entry();
            DataLoader<EntityRef>::write(data.equipped[i].item, out);

            out->Break();
        }

DATA_WRITE_END_FUNC

DATA_READ_FUNC_INIT(Items)
    auto i = EquipementSlotsMap.find(member);

    if(i != EquipementSlotsMap.end())
    {
        buff->read(); buff->read();
        value = buff->read();
        data.equipped[i->second].id = FastTextParser::read<uint>(value);
        buff->read(); buff->read(); 
        value = buff->read();
        if(*value == '|')
            data.equipped[i->second].item = spawnEntity(buff->read());
        else
            data.equipped[i->second].item = DataLoader<EntityRef>::read(buff);

        // if(data.equipped[i->second].item->hasComp<EntityState3D>())
        // {
        //     WARNING_MESSAGE(data.equipped[i->second].item->toStr())
        //     data.equipped[i->second].item->comp<EntityState3D>().position = vec3(0);
        //     data.equipped[i->second].item->comp<EntityState3D>().initPosition = vec3(0);
            
        //     data.equipped[i->second].item->comp<EntityState3D>().quaternion = quat(1, 0, 0, 0);
        //     data.equipped[i->second].item->comp<EntityState3D>().initQuat = quat(1, 0, 0, 0);
        // }

        buff->read();
    }
DATA_READ_END_FUNC

DATA_WRITE_FUNC_INIT(EntityGroupInfo)

    // out->Entry();
    // WRITE_NAME(children, out);
    // out->Tabulate();
    
    for(auto i : data.children)
    {
        out->Entry();
        WRITE_NAME(child, out);

        std::string name = i->comp<EntityInfos>().name;
        if(Loader<EntityRef>::loadingInfos.find(name) != Loader<EntityRef>::loadingInfos.end())
        {
            WRITE_NAME(|, out);
            name = "\"" + name + "\"";
            out->write(CONST_STRING_SIZED(name));
        }
        else
        {
            WRITE_NAME(:, out);
            DataLoader<EntityRef>::write(i, out);
        }
    }
    
    // out->Break();

DATA_WRITE_END_FUNC

DATA_READ_FUNC_ENTITY(EntityGroupInfo)
{
    DATA_READ_INIT(EntityGroupInfo);

    WHILE_NEW_VALUE

        // std::cout << member << "\n";

        IF_MEMBER_READ_VALUE(child)
        {
            EntityRef c;

            if(*value == ':')
            {
                c = DataLoader<EntityRef>::read(buff);
            }
            else if(*value == '|')
            {
                std::string name(buff->read());

                auto ent = Loader<EntityRef>::loadingInfos.find(name);

                if(ent == Loader<EntityRef>::loadingInfos.end())
                {
                    ERROR_MESSAGE("No information loaded for entity '" << name << "' referenced inside the file '" << buff->getSource() << "'");
                    assert(false);
                }

                VulpineTextBuffRef source(new VulpineTextBuff(
                    ent->second->buff->getSource().c_str()
                ));

                c = DataLoader<EntityRef>::read(source);
            }

            ComponentModularity::addChild(*e, c);
            data.children.push_back(c);
        }

    WHILE_NEW_VALUE_END

    DATA_READ_END
}



AUTOGEN_DATA_RW_FUNC(ItemTransform, mat);

DATA_WRITE_FUNC_INIT(EntityModel)
    if(!data->name.size())
    {
        for(auto c : data->getChildren())
        {
            if(c->name.size())
            {
                out->write("\"", 1);
                out->write(CONST_STRING_SIZED(c->name)-1);
                out->write("\" ", 2);
            }
            else
            {
                WARNING_MESSAGE("Can't save " << type_name<EntityModel>() << " component. Name is empty");
                out->write("\"\"", 2);
            }
        }
    }  
    else
    {
        out->write("\"", 1);
        out->write(CONST_STRING_SIZED(data->name));
        out->write("\"", 1);
    }
DATA_WRITE_END_FUNC

DATA_READ_FUNC(EntityModel)
{ 
    DATA_READ_INIT(EntityModel)

    data = {newObjectGroup()};

    const char *ptr;
    int cnt = 0;
    while(*(ptr = buff->read()) != ';')
    {
        ObjectGroupRef lodn = Loader<ObjectGroup>::get(std::string(ptr)).copy();
        if(cnt)
        {
            lodn->state.setHideStatus(ModelStatus::HIDE);
        }
        data->add(lodn);
        cnt ++;
    }

    return data;
}
DATA_WRITE_FUNC_INIT(SkeletonAnimationState)
    out->Entry();
    WRITE_NAME(name, out)
    if(!data.name.size())
    {
        WARNING_MESSAGE("Can't save " << type_name<decltype(data)>() << " component. Name is empty");
        out->write("\"\"", 2);
    }  
    else 
        out->write(CONST_STRING_SIZED(data.name));

    out->Entry();
    WRITE_NAME(data, out)
    out->write("\"", 1);
    size_t size = sizeof(mat4)*data.size(); 
    out->write((const char*)&data[0], size);
    out->write("\"", 1);
DATA_WRITE_END_FUNC

DATA_READ_FUNC_INIT(SkeletonAnimationState)

    IF_MEMBER_READ_VALUE(name)
        data = SkeletonAnimationState(Loader<SkeletonRef>::get(std::string(value)));
    else IF_MEMBER_READ_VALUE(data)
    {
        size_t size = sizeof(mat4)*data.size();
        memcpy(&data[0], value, size);
    }

DATA_READ_END_FUNC

/*
    TODO : maybe put more infos like the current state of the controller
*/
DATA_WRITE_FUNC_INIT(AnimationControllerInfos)
    out->Entry();
    WRITE_NAME(name, out)
    if(!data.size())
    {
        WARNING_MESSAGE("Can't save " << type_name<decltype(data)>() << " component. Name is empty");
        out->write("\"\"", 2);
    }  
    else 
        out->write(CONST_STRING_SIZED(data));

    out->Entry();
    WRITE_NAME(type, out);
    out->write(CONST_STRING_SIZED(AnimationControllerInfos::TypeReverseMap[data.type]));
DATA_WRITE_END_FUNC

DATA_READ_FUNC_INIT(AnimationControllerInfos)
    IF_MEMBER_READ_VALUE(name)
        data = {std::string(value)};
    else IF_MEMBER_READ_VALUE(type)
        MAP_SAFE_READ(AnimationControllerInfos::TypeMap, buff, data.type, value)
DATA_READ_END_FUNC

/*
    Effect Loader is kind-of empty for now, because effects are created
    by animation controller. This could change in the future.
*/
DATA_WRITE_FUNC_INIT(Effect)
DATA_WRITE_END_FUNC

DATA_READ_FUNC(Effect)
{ 
    DATA_READ_INIT(Effect)
    buff->read();
    DATA_READ_END
}

/***************** RP3D LOADERS *****************/

typedef rp3d::Transform Transform;

DATA_WRITE_FUNC_INIT(Transform)
    
    WRITE_FUNC_RESULT(position, PG::toglm(data.getPosition()));

    rp3d::Quaternion q = data.getOrientation();
    WRITE_FUNC_RESULT_COND(quaternion, vec4(q.w, q.x, q.y, q.z), != vec4(1, 0, 0, 0));

DATA_WRITE_END_FUNC

DATA_READ_FUNC_INIT(Transform);
    IF_MEMBER_READ_VALUE(position)
        data.setPosition(PG::torp3d(FastTextParser::read<vec3>(value)));
    else
    IF_MEMBER_READ_VALUE(quaternion)
    {
        vec4 v = FastTextParser::read<vec4>(value);
        data.setOrientation(PG::torp3d(quat(v.x, v.y, v.z, v.w)));
    }
DATA_READ_END_FUNC

void writeCollisionCategory(VulpineTextOutputRef out, uint16 cc)
{
    static const uint64 namedCategoryNB = sizeof(CollideCategoryReverseMap)/sizeof(std::string); 

    uint64 i = 0;
    for(int b = 1; cc && i < namedCategoryNB; cc &= ~b, b = b<<1, i++)
        if(cc&b)
        {
            auto &s = CollideCategoryReverseMap[i];
            out->Entry();
            out->write(CONST_STRING_SIZED(s));
        }
}

uint16 readCollisionCategory(VulpineTextBuffRef buff)
{
    uint16 res = 0;

    while (NEW_VALUE)
    {
        uint16 id = UINT16_MAX;
        MAP_SAFE_READ(CollideCategoryMap, buff, id, buff->read());

        if(id == UINT16_MAX) continue;

        res |= 1<<id; 
    }
    
    return res;
}

typedef rp3d::CapsuleShape* CapsuleShape;
typedef rp3d::BoxShape* BoxShape;
typedef rp3d::SphereShape* SphereShape;
typedef rp3d::ConvexMeshShape* ConvexMeshShape;
typedef rp3d::Material* ColliderMaterial;

DATA_WRITE_FUNC_INIT(CapsuleShape);
    WRITE_FUNC_RESULT(height, data->getHeight());
    WRITE_FUNC_RESULT(radius, data->getRadius());
DATA_WRITE_END_FUNC

DATA_READ_FUNC_INITI(CapsuleShape, data = PG::common.createCapsuleShape(1, 1))
    IF_MEMBER_READ_VALUE(height)
        data->setHeight(FastTextParser::read<float>(value));   
    else IF_MEMBER_READ_VALUE(radius)
        data->setRadius(FastTextParser::read<float>(value));   
DATA_READ_END_FUNC

DATA_WRITE_FUNC_INIT(BoxShape);
    auto aabb = data->getLocalBounds();
    WRITE_FUNC_RESULT(extent, PG::toglm(aabb.getMax()));
DATA_WRITE_END_FUNC

DATA_READ_FUNC_INITI(BoxShape, data = nullptr);
    IF_MEMBER_READ_VALUE(extent)
        data = PG::common.createBoxShape(PG::torp3d(FastTextParser::read<vec3>(value)));    
DATA_READ_END_FUNC

DATA_WRITE_FUNC_INIT(SphereShape);
    WRITE_FUNC_RESULT(radius, data->getRadius());
DATA_WRITE_END_FUNC

DATA_READ_FUNC_INITI(SphereShape, data = nullptr)
    IF_MEMBER_READ_VALUE(radius)
        data = PG::common.createSphereShape(FastTextParser::read<float>(value));
DATA_READ_END_FUNC

DATA_WRITE_FUNC_INIT(ConvexMeshShape);
    uint size = data->getNbVertices();
    WRITE_FUNC_RESULT(size, size);
    for(int i = 0; i < size; i++)
    {
        WRITE_FUNC_RESULT(vertex, PG::toglm(data->getVertexPosition(i)));
    }
DATA_WRITE_END_FUNC

DATA_READ_FUNC(ConvexMeshShape)
{ 
    DATA_READ_INIT(ConvexMeshShape) 
    std::vector<vec3> vertexBuffer; 
    WHILE_NEW_VALUE
        IF_MEMBER_READ_VALUE(size)
            vertexBuffer.reserve(FastTextParser::read<uint>(value));
        else
        IF_MEMBER_READ_VALUE(vertex)
            vertexBuffer.push_back(FastTextParser::read<vec3>(value));
    WHILE_NEW_VALUE_END 
    
    std::vector<rp3d::Message> error;
    data = PG::common.createConvexMeshShape(
        PG::common.createConvexMesh(
            rp3d::VertexArray(vertexBuffer.data(), sizeof(vec3), vertexBuffer.size(), rp3d::VertexArray::DataType::VERTEX_FLOAT_TYPE),
            error
        )
    );

    DATA_READ_END
}

DATA_WRITE_FUNC_INIT(ColliderMaterial);
    WRITE_FUNC_RESULT(bounciness, data->getBounciness());
    WRITE_FUNC_RESULT(friction, data->getFrictionCoefficient());
    WRITE_FUNC_RESULT_COND(density, data->getMassDensity(), != 1.f);
DATA_WRITE_END_FUNC

/*
    Thanks to the uncovenient way Materials are created, we can't even use the default constructor.
    This method renders the DataLoaders non thread safe :(
*/
ColliderMaterial RP3DmaterialReadTMP = nullptr;
DATA_READ_FUNC_INITI(ColliderMaterial, data = RP3DmaterialReadTMP)
    IF_MEMBER_READ_VALUE(bounciness)
        data->setBounciness(FastTextParser::read<float>(value));
    else IF_MEMBER_READ_VALUE(friction)
        data->setFrictionCoefficient(FastTextParser::read<float>(value));
    else IF_MEMBER_READ_VALUE(density)
        data->setMassDensity(FastTextParser::read<float>(value));
DATA_READ_END_FUNC

DATA_WRITE_FUNC_INIT(Collider)

    auto cs = data->getCollisionShape();
    out->write(CONST_CSTRING_SIZED("*Shape must always be given first*"));

    switch (cs->getName())
    {
        case rp3d::CollisionShapeName::CAPSULE :
                DataLoader<CapsuleShape>::write((CapsuleShape)cs, out);
            break;
        
        case rp3d::CollisionShapeName::BOX :
                DataLoader<BoxShape>::write((BoxShape)cs, out);
            break;

        case rp3d::CollisionShapeName::SPHERE :
                DataLoader<SphereShape>::write((SphereShape)cs, out);
            break;
        
        case rp3d::CollisionShapeName::CONVEX_MESH : 
                DataLoader<ConvexMeshShape>::write((ConvexMeshShape)cs, out);
            break;

        default: out->write(CONST_CSTRING_SIZED("*!! Unsuported Collision Shape !!*"));
        break;
    }

    Transform t = data->getLocalToBodyTransform();
    if(t != Transform::identity())
        DataLoader<Transform>::write(t, out);

    uint16 cc = data->getCollisionCategoryBits();
    if(cc)
    {
        out->Entry();
        WRITE_NAME(CollisionCategory, out);
        out->Tabulate();
        writeCollisionCategory(out, cc);
        out->Break();
    }
    uint16 cw = data->getCollideWithMaskBits();
    if(cw)
    {
        out->Entry();
        WRITE_NAME(CollideWith, out);
        out->Tabulate();
        writeCollisionCategory(out, cw);
        out->Break();
    }
    
    DataLoader<ColliderMaterial>::write(&data->getMaterial(), out);

    WRITE_FUNC_RESULT(isTrigger, data->getIsTrigger());

DATA_WRITE_END_FUNC

/*
    Thanks to the uncovenient way Collider are created, we can't create a new Collider without a 
    reference to the body. The actual creation and body type reading will be done in the rigidbody
    loader fonction.
*/
Collider newColliderTMP = nullptr;

DATA_READ_FUNC_INITI(Collider, data = newColliderTMP)
    IF_MEMBER(Transform)
        data->setLocalToBodyTransform(DataLoader<Transform>::read(buff));

    else IF_MEMBER(ColliderMaterial)
    {
        RP3DmaterialReadTMP = &data->getMaterial();
        DataLoader<ColliderMaterial>::read(buff);
    }

    else IF_MEMBER(CollisionCategory)
        data->setCollisionCategoryBits(readCollisionCategory(buff));

    else IF_MEMBER(CollideWith)
        data->setCollideWithMaskBits(readCollisionCategory(buff));

    else IF_MEMBER_READ_VALUE(isTrigger)
        data->setIsTrigger(FastTextParser::read<bool>(value));

DATA_READ_END_FUNC 


GENERATE_ENUM_FAST_REVERSE(rp3dBodyType, STATIC, KINEMATIC, DYNAMIC);

DATA_WRITE_FUNC_INIT(RigidBody);

    out->Entry();
    WRITE_NAME(type, out);
    out->write(CONST_STRING_SIZED(rp3dBodyTypeReverseMap[(int)data->getType()]));

    WRITE_FUNC_RESULT_COND(mass, data->getMass(), != 1.f);

    WRITE_FUNC_RESULT_COND(angularVelocity, PG::toglm(data->getAngularVelocity()), != vec3(0));

    WRITE_FUNC_RESULT_COND(angularLockFactor, PG::toglm(data->getAngularLockAxisFactor()), != vec3(1));

    WRITE_FUNC_RESULT_COND(angularDampingFactor, data->getAngularDamping(), != 0.f);

    WRITE_FUNC_RESULT_COND(linearVelocity, PG::toglm(data->getLinearVelocity()), != vec3(0));

    WRITE_FUNC_RESULT_COND(linearLockFactor, PG::toglm(data->getLinearLockAxisFactor()), != vec3(1));

    WRITE_FUNC_RESULT_COND(linearDampingFactor, data->getLinearDamping(), != 0.f);

    WRITE_FUNC_RESULT(canSleep, data->isAllowedToSleep());

    WRITE_FUNC_RESULT_COND(sleeping, data->isSleeping(), && data->isAllowedToSleep());

    WRITE_FUNC_RESULT(gravity, data->isGravityEnabled());

    int cnb = data->getNbColliders();
    if(cnb)
    {
        out->Entry();
        out->Tabulate();
        WRITE_NAME(Colliders, out);
        for(int i = 0; i < cnb; i++)
            DataLoader<Collider>::write(data->getCollider(i), out);
        out->Break();
    }

    WRITE_FUNC_RESULT(active, data->isActive());
    

    /* remove later*/
    // DataLoader<Transform>::write(data->getTransform(), out);
DATA_WRITE_END_FUNC

DATA_READ_FUNC_ENTITY(RigidBody)
{ 
    DATA_READ_INIT(RigidBody)

    if(e->hasComp<EntityState3D>())
    {
        auto &s = e->comp<EntityState3D>();
        rp3d::Vector3 pos = PG::torp3d(s.position);
        rp3d::Quaternion quat = s.usequat ? PG::torp3d(s.quaternion) : DEFQUAT;
        data = PG::world->createRigidBody(Transform(pos, quat));
    }
    else
    {
        // TODO : put a warning message here
        FILE_ERROR_MESSAGE(
            buff->getSource(), 
            "Entity " << e->comp<EntityInfos>().name << " has empty EntityState3D when creating RigidBody"
            )
        data = PG::world->createRigidBody(rp3d::Transform::identity());
    }

    WHILE_NEW_VALUE
        IF_MEMBER_READ_VALUE(type)
            MAP_SAFE_READ_FC(rp3dBodyTypeMap, buff, data->setType, rp3d::BodyType, value)

        else IF_MEMBER_READ_VALUE(mass)
            data->setMass(FastTextParser::read<float>(value));

        else IF_MEMBER_READ_VALUE(linearVelocity)
            data->setLinearVelocity(PG::torp3d(FastTextParser::read<vec3>(value)));

        else IF_MEMBER_READ_VALUE(linearLockFactor)
            data->setLinearLockAxisFactor(PG::torp3d(FastTextParser::read<vec3>(value)));

        else IF_MEMBER_READ_VALUE(linearDampingFactor)
            data->setLinearDamping(FastTextParser::read<float>(value));

        else IF_MEMBER_READ_VALUE(angularVelocity)
            data->setAngularVelocity(PG::torp3d(FastTextParser::read<vec3>(value)));

        else IF_MEMBER_READ_VALUE(angularLockFactor)
            data->setAngularLockAxisFactor(PG::torp3d(FastTextParser::read<vec3>(value)));

        else IF_MEMBER_READ_VALUE(angularDampingFactor)
            data->setAngularDamping(FastTextParser::read<float>(value));

        else IF_MEMBER_READ_VALUE(active)
            data->setIsActive(FastTextParser::read<bool>(value));

        else IF_MEMBER_READ_VALUE(sleeping)
            data->setIsSleeping(FastTextParser::read<bool>(value));

        else IF_MEMBER_READ_VALUE(canSleep)
            data->setIsAllowedToSleep(FastTextParser::read<bool>(value));

        else IF_MEMBER_READ_VALUE(gravity)
            data->enableGravity(FastTextParser::read<bool>(value));

        else IF_MEMBER(Colliders)
        {
            while(NEW_VALUE)
            {
                member = buff->read();
                IF_MEMBER(Collider)
                {
                    buff->read();
                    member = buff->read();
                    IF_MEMBER(CapsuleShape)
                        newColliderTMP = data->addCollider(DataLoader<CapsuleShape>::read(buff), Transform::identity());
                    else
                    IF_MEMBER(BoxShape)
                        newColliderTMP = data->addCollider(DataLoader<BoxShape>::read(buff), Transform::identity());
                    else
                    IF_MEMBER(SphereShape)
                        newColliderTMP = data->addCollider(DataLoader<SphereShape>::read(buff), Transform::identity());
                    else
                    IF_MEMBER(ConvexMeshShape)
                        newColliderTMP = data->addCollider(DataLoader<ConvexMeshShape>::read(buff), Transform::identity());
                    
                    newColliderTMP->setCollideWithMaskBits(0);
                    newColliderTMP->setCollisionCategoryBits(0);

                    DataLoader<Collider>::read(buff);
                }
            }

            data->updateLocalCenterOfMassFromColliders();
            data->updateLocalInertiaTensorFromColliders();
            data->updateMassFromColliders();
            data->updateMassPropertiesFromColliders();
        }
    WHILE_NEW_VALUE_END 

    DATA_READ_END
}

/******* IA COMPONENT *******/
DATA_WRITE_FUNC_INIT(DeplacementBehaviour)
    out->write(CONST_STRING_SIZED(DeplacementBehaviourReverseMap[data]));
DATA_WRITE_END_FUNC

DATA_READ_FUNC(DeplacementBehaviour)
{ 
    DATA_READ_INIT(DeplacementBehaviour)
    data = DeplacementBehaviour::STAND_STILL;
    const char *value = buff->read();
    MAP_SAFE_READ(DeplacementBehaviourMap, buff, data, value)
    buff->read();
    DATA_READ_END
}

DATA_WRITE_FUNC_INIT(AgentState__old)
    out->Entry();
    WRITE_NAME(state, out)
    out->write(CONST_STRING_SIZED(AgentState__old::StateReverseMap[data.state]));
    FTXTP_WRITE_ELEMENT(data, timeSinceLastState)
    FTXTP_WRITE_ELEMENT(data, randomTime)
DATA_WRITE_END_FUNC

DATA_READ_FUNC_INIT(AgentState__old)
    IF_MEMBER_READ_VALUE(state)
        MAP_SAFE_READ(AgentState__old::StateMap, buff, data.state, value)
    else IF_MEMBER_FTXTP_LOAD(data, timeSinceLastState)
    else IF_MEMBER_FTXTP_LOAD(data, randomTime)
DATA_READ_END_FUNC

DATA_WRITE_FUNC_INIT(Script)
{
    for (int i = 0; i < ScriptHook::HOOK_END; i++)
    {
        std::string hookName = ScriptHookReverseMap[i];
        if (data.scripts[i].size())
        {
            out->Entry();
            out->write(CONST_STRING_SIZED(hookName));
            out->Tabulate();
            for (const std::string &script : data.scripts[i])
            {
                out->Entry();
                out->write("\"", 1);
                out->write(CONST_STRING_SIZED(script));
                out->write("\"", 1);
            }
            out->Break();
        }
    }
}
DATA_WRITE_END_FUNC

template <> 
Script DataLoader<Script>::read(VulpineTextBuffRef buff) 
{
    Script data;

    while (NEW_VALUE) 
    {                                                          
        const char* hookNameStr = buff->read();
        ScriptHook hook = ScriptHook::HOOK_END;
        MAP_SAFE_READ(ScriptHookMap, buff, hook, hookNameStr);
        if (hook == ScriptHook::HOOK_END)
            continue;

        while (NEW_VALUE)
        {
            const char *scriptName = buff->read();
            data.scripts[hook].push_back(std::string(scriptName));
        }
    }

    return data;
}