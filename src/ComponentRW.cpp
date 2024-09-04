#include <SanctiaEntity.hpp>
#include <AssetManager.hpp>
#include <AssetManagerUtils.hpp>
#include <MappedEnum.hpp>



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

// DATA_WRITE_FUNC(ActionState) 
// {
//     DATA_WRITE_INIT(ActionState);
    
//     FTXTP_WRITE_ELEMENT(data, isTryingToAttack)
//     FTXTP_WRITE_ELEMENT(data, isTryingToBlock)
//     FTXTP_WRITE_ELEMENT(data, hasBlockedAttack)
//     FTXTP_WRITE_ELEMENT(data, stun)
//     FTXTP_WRITE_ELEMENT(data, blocking)
//     FTXTP_WRITE_ELEMENT(data, attacking)
//     FTXTP_WRITE_ELEMENT(data, lockedMaxSpeed)
//     FTXTP_WRITE_ELEMENT(data, lockedDirection)

//     DATA_WRITE_END;
// }

// DATA_READ_FUNC(ActionState)
// {
//     DATA_READ_INIT(ActionState);

//     while (NEW_VALUE)
//     {
//         const char *member = buff->read();
//         const char *value = buff->read();

//         IF_MEMBER_FTXTP_LOAD(data, isTryingToAttack)
//         else IF_MEMBER_FTXTP_LOAD(data, isTryingToBlock)
//         else IF_MEMBER_FTXTP_LOAD(data, hasBlockedAttack)
//         else IF_MEMBER_FTXTP_LOAD(data, stun)
//         else IF_MEMBER_FTXTP_LOAD(data, blocking)
//         else IF_MEMBER_FTXTP_LOAD(data, attacking)
//         else IF_MEMBER_FTXTP_LOAD(data, lockedMaxSpeed)
//         else IF_MEMBER_FTXTP_LOAD(data, lockedDirection)
        
//     }

//     DATA_READ_END;
// }

AUTOGEN_DATA_RW_FUNC(ActionState, 
    lockedDirection,
    isTryingToAttack, 
    isTryingToBlock,
    stun,
    blocking,
    attacking,
    lockedMaxSpeed
    )


// struct PhysicRigidBody
// {
//     rp3d::RigidBody *ptr;
// };



typedef rp3d::Transform Transform;

DATA_WRITE_FUNC_INIT(Transform);
    
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

    WRITE_FUNC_RESULT(active, data->isActive());

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

    

    /* remove later*/
    // DataLoader<Transform>::write(data->getTransform(), out);
DATA_WRITE_END_FUNC

DATA_READ_FUNC_INITI(RigidBody, data = PG::world->createRigidBody(rp3d::Transform::identity()))
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
                
                newColliderTMP->setCollideWithMaskBits(0);
                newColliderTMP->setCollisionCategoryBits(0);

                DataLoader<Collider>::read(buff);
            }
        }

        data->updateLocalCenterOfMassFromColliders();
        data->updateLocalInertiaTensorFromColliders();
    }

DATA_READ_END_FUNC 