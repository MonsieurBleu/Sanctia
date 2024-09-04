#pragma once 

#include <reactphysics3d/reactphysics3d.h>

typedef  rp3d::RigidBody* RigidBody;
typedef rp3d::Collider* Collider;

extern std::mutex physicsMutex;

GENERATE_ENUM_FAST_REVERSE(
    CollideCategory,
    HITZONE,
    ENVIRONEMENT
);

GENERATE_ENUM_FAST_REVERSE(CollisionShapeName,
    TRIANGLE, SPHERE, CAPSULE, BOX, CONVEX_MESH, TRIANGLE_MESH, HEIGHTFIELD
);

// GENERATE_ENUM_FAST_REVERSE(CollisionShapeType,
//     SPHERE, CAPSULE, CONVEX_POLYHEDRON, CONCAVE_SHAPE
// );

// enum CollideCategory
// {
//     HITZONE      = 1<<0,
//     ENVIRONEMENT = 1<<1
// };

class PG 
{
    public : 

        static inline rp3d::PhysicsCommon common;
        static inline rp3d::PhysicsWorld* world = nullptr;


        static inline vec3 toglm(const rp3d::Vector3 &v){return vec3(v.x, v.y, v.z);};
        static inline rp3d::Vector3 torp3d(const vec3 &v){return rp3d::Vector3(v.x, v.y, v.z);}; 

        static inline quat toglm(const rp3d::Quaternion &v){return quat(v.w, v.x, v.y, v.z);};
        static inline rp3d::Quaternion torp3d(const quat &v){return rp3d::Quaternion(v.x, v.y, v.z, v.w);}; 

        static inline float currentPhysicFreq = 0.f;
};
