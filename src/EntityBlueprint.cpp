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
    static int i = -1;
    i++;
    B_DynamicBodyRef body(new B_DynamicBody);
    vec3 position = 3.f*vec3(-1*(i/15), 1, -1*(i%15)) + vec3(-5, 0, 0);
 
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
            , ActionState{}
            , Faction{Faction::Type::ENEMY}
            , Items{}
            );

    // e->comp<EntityActionState>().isTryingToBlock = true;

    e->set<AnimationControllerRef>(AnimBlueprint::bipedMoveset("65_2HSword", e.get()));

    e->comp<Items>().equipped[WEAPON_SLOT] = {24, Blueprint::Zweihander()};
    e->comp<Items>().equipped[LEFT_FOOT_SLOT] = {7, Blueprint::Foot()};

    GG::entities.push_back(e);

    return GG::entities.back();
}

EntityRef Blueprint::Zweihander()
{
    EntityRef zweihander(new Entity("ZweiHander"
        , ItemInfos{100, 10, DamageType::Slash, B_Collider().setCapsule(0.1, vec3(0, 0, 0), vec3(1.23, 0, 0))}
        , Effect()
        , EntityModel{Loader<ObjectGroup>::get("Zweihander").copy()}
        , ItemTransform{}
        , PhysicsHelpers{}
    ));

    GG::entities.push_back(zweihander);

    return zweihander;
}

EntityRef Blueprint::Foot()
{
    EntityRef feet(new Entity("right foot"
        , ItemInfos{0, 5, DamageType::Blunt, B_Collider().setCapsule(0.2, vec3(0, 0, 0), vec3(0, 0, 0.2))}
        , Effect()
        , ItemTransform{}
        , PhysicsHelpers{}
    ));

    GG::entities.push_back(feet);

    return feet;
}
