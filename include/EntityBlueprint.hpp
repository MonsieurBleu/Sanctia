#pragma once
#include <SanctiaEntity.hpp>

namespace Blueprint
{
    namespace Assembly
    {
        void ConfigureBody(rp3d::RigidBody *body, reactphysics3d::Collider *collider);

        rp3d::RigidBody *CapsuleBody(float height, vec3 position, EntityRef entity);
    };

    EntityRef TestManequin();

    EntityRef Zweihander();

    EntityRef Foot();
};
