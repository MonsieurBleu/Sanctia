#include <EntityBlueprint.hpp>
#include <AssetManager.hpp>
#include <Helpers.hpp>
#include <Constants.hpp>

EntityRef Blueprint::TestManequin()
{
    ObjectGroupRef newGroup = Loader<ObjectGroup>::get("HumanMale").copy();
    int i = GG::entities.size();
    B_DynamicBodyRef body(new B_DynamicBody);
    vec3 position = vec3(-5*i/20, 5, -5*i%20) + vec3(-5, 0, 0);

    float radius = 0.85;
    body->boundingCollider.setCapsule(0.5, vec3(0, 0.5, 0), vec3(0, 1.25, 0));
    body->position = position;
    body->applyForce(vec3(0, -G, 0));

    // auto sh(SphereHelperRef(new SphereHelper(vec3(1, 1, 0), radius)));
    // sh->state.setPosition(vec3(0, radius, 0));
    // newGroup->add(sh);

    GG::entities.push_back(
        newEntity(
            "HumanMale number " + std::to_string(i), 
            EntityModel{newGroup}, 
            EntityState3D({position, vec3(0)}),
            B_DynamicBodyRef(body)
            ));



    return GG::entities.back();
}