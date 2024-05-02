#include <EntityBlueprint.hpp>
#include <AssetManager.hpp>
#include <Helpers.hpp>
#include <Constants.hpp>
#include <EntityStats.hpp>
#include <AnimationBlueprint.hpp>

EntityRef Blueprint::TestManequin()
{
    ObjectGroupRef newGroup = Loader<ObjectGroup>::get("NpcTest").copy();
    // newGroup->add(Loader<ObjectGroup>::get("Zweihander").copy());
    int i = GG::entities.size();
    B_DynamicBodyRef body(new B_DynamicBody);
    vec3 position = vec3(-4*(i/10), 5, -4*(i%10)) + vec3(-5, 0, 0);

    // float radius = 0.85;
    body->boundingCollider.setCapsule(0.5, vec3(0, 0.5, 0), vec3(0, 1.25, 0));
    body->position = position;
    body->applyForce(vec3(0, -G, 0));

    // auto sh(SphereHelperRef(new SphereHelper(vec3(1, 1, 0), radius)));
    // sh->state.setPosition(vec3(0, radius, 0));
    // newGroup->add(sh);


    EntityStats stats;

    EntityRef e = newEntity("HumanMale number " + std::to_string(i)
            , EntityModel{newGroup}
            , EntityState3D({position, vec3(1, 0, 0), 1.f})
            , B_DynamicBodyRef(body)
            , stats
            , CharacterDialogues("ressources/dialogues/Fariah Grisnier.md", "Fariah Grisnier")
            , DeplacementBehaviour{DEMO}
            , SkeletonAnimationState(Loader<SkeletonRef>::get("Xbot"))
            , NpcPcRelation()
            , EntityActionState{}
            , Items{}
            );

    e->set<AnimationControllerRef>(AnimBlueprint::bipedMoveset("65_2HSword", e.get()));

    GG::entities.push_back(e);

    return GG::entities.back();
}

EntityRef Blueprint::Zweihander()
{
    EntityRef zweihander(new Entity("ZweiHander"
        , ItemInfos{100, 50, DamageType::Slash, B_Collider().setCapsule(0.1, vec3(0, 0, 0), vec3(1.23, 0, 0))}
        , Effect()
        , EntityModel{Loader<ObjectGroup>::get("Zweihander").copy()}
        , ItemTransform{}
        , PhysicsHelpers{}
    ));

    GG::entities.push_back(zweihander);

    return zweihander;
}

EntityRef Blueprint::DamageBox(vec3 position, float size)
{
    EntityState3D state;
    state.position = position;

    // EffectList e;
    // e.weapon.zone.settAABB(vec3(-size/2), vec3(size/2));
    // e.weapon.type = EffectType::Damage;
    // e.weapon.valtype = DamageType::Pure;
    // e.weapon.value = 100;
    // e.weapon.maxTrigger = 5;

    // ObjectGroupRef newGroup = newObjectGroup();
    // newGroup->add(CubeHelperRef(new CubeHelper(vec3(-size/2), vec3(size/2), vec3(1, 0, 0))));

    // GG::entities.push_back(
    //     newEntity(
    //         "Damage Box",
    //         EntityModel{newGroup},
    //         state,
    //         e
    // ));

    return GG::entities.back();
};