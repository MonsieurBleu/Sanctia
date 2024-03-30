#include <EntityBlueprint.hpp>
#include <AssetManager.hpp>
#include <Helpers.hpp>
#include <Constants.hpp>
#include <EntityStats.hpp>

EntityRef Blueprint::TestManequin()
{
    ObjectGroupRef newGroup = Loader<ObjectGroup>::get("HumanMale").copy();
    int i = GG::entities.size();
    B_DynamicBodyRef body(new B_DynamicBody);
    vec3 position = vec3(-5*(i/10), 5, -5*(i%10)) + vec3(-5, 0, 0);

    // float radius = 0.85;
    body->boundingCollider.setCapsule(0.5, vec3(0, 0.5, 0), vec3(0, 1.25, 0));
    body->position = position;
    body->applyForce(vec3(0, -G, 0));

    // auto sh(SphereHelperRef(new SphereHelper(vec3(1, 1, 0), radius)));
    // sh->state.setPosition(vec3(0, radius, 0));
    // newGroup->add(sh);


    Effect testEffectZone;
    // testEffectZone.zone.setSphere(0.5, vec3(1, 0, 0));
    testEffectZone.zone.setCapsule(0.1, vec3(-1, 1, 1), vec3(1, 1, 1));
    testEffectZone.type = EffectType::Damage;
    testEffectZone.valtype = DamageType::Pure;
    testEffectZone.value = 100;
    testEffectZone.maxTrigger = 5;


    EntityStats stats;

    GG::entities.push_back(
        newEntity(
            "HumanMale number " + std::to_string(i), 
            EntityModel{newGroup}, 
            EntityState3D({position, vec3(0)}),
            B_DynamicBodyRef(body),
            stats,
            // testEffectZone,
            // PhysicsHelpers{},
            DeplacementBehaviour{DEMO}
            ));



    return GG::entities.back();
}

EntityRef Blueprint::DamageBox(vec3 position, float size)
{
    EntityState3D state;
    state.position = position;

    Effect e;
    e.zone.settAABB(vec3(-size/2), vec3(size/2));
    e.type = EffectType::Damage;
    e.valtype = DamageType::Pure;
    e.value = 100;
    e.maxTrigger = 5;

    ObjectGroupRef newGroup = newObjectGroup();
    newGroup->add(CubeHelperRef(new CubeHelper(vec3(-size/2), vec3(size/2), vec3(1, 0, 0))));

    GG::entities.push_back(
        newEntity(
            "Damage Box",
            EntityModel{newGroup},
            state,
            e
    ));

    return GG::entities.back();
};