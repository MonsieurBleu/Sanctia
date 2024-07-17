#include <EntityBlueprint.hpp>
#include <AssetManager.hpp>
#include <Helpers.hpp>
#include <Constants.hpp>
#include <EntityStats.hpp>
#include <AnimationBlueprint.hpp>

void Blueprint::Assembly::ConfigureBody(rp3d::RigidBody *body, reactphysics3d::Collider *collider)
{
    body->setAngularLockAxisFactor(rp3d::Vector3(0, 1, 0));
    collider->getMaterial().setBounciness(0.f);
    collider->getMaterial().setFrictionCoefficient(1.f);
    collider->setCollisionCategoryBits(CollideCategory::ENVIRONEMENT);
    collider->setCollideWithMaskBits(CollideCategory::ENVIRONEMENT);
}


rp3d::RigidBody* Blueprint::Assembly::CapsuleBody(float height, vec3 position, EntityRef entity)
{
    rp3d::RigidBody *body = PG::world->createRigidBody(rp3d::Transform(
        PG::torp3d(position), 
        rp3d::Quaternion::identity()));

    const float capsuleHeight = height;
    const float capsuleRadius = height*0.25;
    const float capsuleLength = capsuleHeight - capsuleRadius*2.f;
    
    auto collider = body->addCollider(
        PG::common.createCapsuleShape(capsuleRadius, capsuleLength), 
        rp3d::Transform({0.f, capsuleHeight*0.5f, 0.f}, rp3d::Quaternion::identity()));

    Blueprint::Assembly::ConfigureBody(body, collider);

    /* TODO : remove */
    auto hitbox = body->addCollider(
        PG::common.createCapsuleShape(capsuleRadius*2, capsuleLength), 
        rp3d::Transform({0.f, capsuleHeight*0.5f, 0.f}, rp3d::Quaternion::identity()));
    hitbox->setIsTrigger(true);
    hitbox->setCollisionCategoryBits(CollideCategory::HITZONE);
    hitbox->setCollideWithMaskBits(CollideCategory::HITZONE);
    hitbox->setUserData(entity.get());

    return body;
}

EntityRef Blueprint::TestManequin()
{
    ObjectGroupRef newGroup = Loader<ObjectGroup>::get("NpcTest").copy();
    static int i = 0;
    vec3 position = 3.f*vec3(-1*(i/15), 1, -1*(i%15)) + vec3(-5, 0, 0);
 
    EntityStats stats;

    EntityRef e = newEntity("HumanMale number " + std::to_string(i)
            , EntityModel{newGroup}
            , EntityState3D({position})
            // , Blueprint::Assembly::CapsuleBody(1.75f, position)
            , stats
            , CharacterDialogues("ressources/dialogues/Fariah Grisnier.md", "Fariah Grisnier")
            , DeplacementBehaviour{DEMO}
            , SkeletonAnimationState(Loader<SkeletonRef>::get("Xbot"))
            , NpcPcRelation()
            , ActionState{}
            , Faction{Faction::Type::ENEMY}
            , Items{}
            );

    e->set<rp3d::RigidBody*>(Blueprint::Assembly::CapsuleBody(1.75f, position, e));

    e->set<AnimationControllerRef>(AnimBlueprint::bipedMoveset("65_2HSword", e.get()));

    e->comp<Items>().equipped[WEAPON_SLOT] = {24, Blueprint::Zweihander()};
    e->comp<Items>().equipped[LEFT_FOOT_SLOT] = {7, Blueprint::Foot()};

    GG::entities.push_back(e);

    i++;
    return GG::entities.back();
}

EntityRef Blueprint::Zweihander()
{
    EntityRef zweihander(new Entity("ZweiHander"
        , ItemInfos{100, 10, DamageType::Slash, B_Collider().setCapsule(0.1, vec3(0, 0, 0), vec3(1.23, 0, 0))}
        , EntityState3D()
        , Effect()
        , EntityModel{Loader<ObjectGroup>::get("Zweihander").copy()}
        , ItemTransform{}
        , PhysicsHelpers{}
    ));


    rp3d::RigidBody *body = PG::world->createRigidBody(rp3d::Transform::identity());

    auto hitbox = body->addCollider(
        PG::common.createCapsuleShape(0.1, 1.23), 
        rp3d::Transform(rp3d::Vector3(0.62, 0, 0), PG::torp3d(quat(radians(vec3(0, 0, 90)))))
        );
    hitbox->setIsTrigger(true);
    hitbox->setCollisionCategoryBits(CollideCategory::HITZONE);
    hitbox->setCollideWithMaskBits(CollideCategory::HITZONE);
    hitbox->setUserData(zweihander.get());
    zweihander->set<rp3d::RigidBody*>(body);

    body->setType(rp3d::BodyType::KINEMATIC);

    GG::entities.push_back(zweihander);

    return zweihander;
}

EntityRef Blueprint::Foot()
{
    EntityRef feet(new Entity("right foot"
        , ItemInfos{0, 5, DamageType::Blunt, B_Collider().setCapsule(0.2, vec3(0, 0, 0), vec3(0, 0, 0.2))}
        , Effect()
        , EntityState3D()
        , ItemTransform{}
        , PhysicsHelpers{}
    ));

    GG::entities.push_back(feet);

    return feet;
}
