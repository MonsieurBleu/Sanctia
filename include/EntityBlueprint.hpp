#pragma once
#include <SanctiaEntity.hpp>

namespace Blueprint
{
    namespace Assembly
    {
        void AddEntityBodies(
            rp3d::RigidBody *body, 
            void *usrData,
            const std::vector<std::pair<rp3d::CollisionShape *, rp3d::Transform>> &environementals,
            const std::vector<std::pair<rp3d::CollisionShape *, rp3d::Transform>> &hitboxes
            );

        rp3d::RigidBody *CapsuleBody(float height, vec3 position, EntityRef entity);
    };

    EntityRef TestManequin();

    EntityRef Zweihander();

    EntityRef Foot();
};
