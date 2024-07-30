#include <EntityBlueprint.hpp>
#include <AssetManager.hpp>
#include <Helpers.hpp>
#include <Constants.hpp>
#include <EntityStats.hpp>
#include <AnimationBlueprint.hpp>
#include <GameConstants.hpp>

void Blueprint::Assembly::AddEntityBodies(
    rp3d::RigidBody *body, 
    void *usrData,
    const std::vector<std::pair<rp3d::CollisionShape *, rp3d::Transform>> &environementals,
    const std::vector<std::pair<rp3d::CollisionShape *, rp3d::Transform>> &hitboxes
    )
{
    for(auto &i : environementals)
    {
        auto c = body->addCollider(i.first, i.second);
        c->getMaterial().setBounciness(0.f);
        c->getMaterial().setFrictionCoefficient(1.f);
        c->setCollisionCategoryBits(CollideCategory::ENVIRONEMENT);
        c->setCollideWithMaskBits(CollideCategory::ENVIRONEMENT);
    }

    for(auto &i : hitboxes)
    {
        auto c = body->addCollider(i.first, i.second);
        c->setIsTrigger(true);
        c->setCollisionCategoryBits(CollideCategory::HITZONE);
        c->setCollideWithMaskBits(CollideCategory::HITZONE);
        c->setUserData(usrData);
    }
}

rp3d::RigidBody* Blueprint::Assembly::CapsuleBody(float height, vec3 position, EntityRef entity)
{
    rp3d::RigidBody *body = PG::world->createRigidBody(rp3d::Transform(PG::torp3d(position), DEFQUAT));

    body->setAngularLockAxisFactor(rp3d::Vector3(0, 1, 0));

    const float capsuleHeight = height;
    const float capsuleRadius = height*0.25;
    const float capsuleLength = capsuleHeight - capsuleRadius*2.f;

    AddEntityBodies(body, entity.get(), 
        {
            {
                PG::common.createCapsuleShape(capsuleRadius, capsuleLength), 
                rp3d::Transform({0.f, capsuleHeight*0.5f, 0.f}, DEFQUAT)
            }
        }, 
        {
            {
                PG::common.createCapsuleShape(capsuleRadius*0.95, capsuleLength), 
                rp3d::Transform({0.f, capsuleHeight*0.5f, 0.f}, DEFQUAT)
            }
        });

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
            , stats
            , CharacterDialogues("ressources/dialogues/Fariah Grisnier.md", "Fariah Grisnier")
            , DeplacementBehaviour{DEMO}
            , SkeletonAnimationState(Loader<SkeletonRef>::get("Xbot"))
            , NpcPcRelation()
            , ActionState{}
            , Faction{Faction::Type::PLAYER_ENEMY}
            , Items{}
            );

    e->set<rp3d::RigidBody*>(Blueprint::Assembly::CapsuleBody(1.75f, position, e));
    e->set<AnimationControllerRef>(AnimBlueprint::bipedMoveset("65_2HSword", e.get()));

    Items::equip(e, Blueprint::Zweihander(), WEAPON_SLOT, BipedSkeletonID::RIGHT_HAND);
    Items::equip(e, Blueprint::Foot(), LEFT_FOOT_SLOT, BipedSkeletonID::LEFT_FOOT);

    GG::entities.push_back(e);

    i++;
    return GG::entities.back();
}

EntityRef Blueprint::Zweihander()
{
    EntityRef zweihander(new Entity("ZweiHander"
        , ItemInfos{100, 10, DamageType::Slash}
        , EntityState3D()
        , Effect()
        , EntityModel{Loader<ObjectGroup>::get("Zweihander").copy()}
        , ItemTransform{}
    ));

    rp3d::RigidBody *body = PG::world->createRigidBody(rp3d::Transform::identity());

    Assembly::AddEntityBodies(body, zweihander.get(),
    {

    },
    {
        {
            PG::common.createCapsuleShape(0.1, 1.23), 
            rp3d::Transform(rp3d::Vector3(0.62, 0, 0), PG::torp3d(quat(radians(vec3(0, 0, 90)))))
        }
    }
    );


    zweihander->set<rp3d::RigidBody*>(body);

    body->setType(rp3d::BodyType::KINEMATIC);

    GG::entities.push_back(zweihander);

    return zweihander;
}

EntityRef Blueprint::Foot()
{
    EntityRef feet(new Entity("right foot"
        , ItemInfos{0, 5, DamageType::Blunt}
        , Effect()
        , EntityState3D()
        , ItemTransform{}
    ));

    rp3d::RigidBody *body = PG::world->createRigidBody(rp3d::Transform::identity());

    Assembly::AddEntityBodies(body, feet.get(),
    {

    },
    {
        {
            PG::common.createCapsuleShape(0.2, 0.35), 
            rp3d::Transform(rp3d::Vector3(0, 0.25, 0), PG::torp3d(quat(radians(vec3(0, 0, 0)))))
        }
    }
    );

    feet->set<rp3d::RigidBody*>(body);

    body->setType(rp3d::BodyType::KINEMATIC);

    GG::entities.push_back(feet);

    return feet;
}
