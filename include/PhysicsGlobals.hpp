#pragma once 

#include "reactphysics3d/collision/RaycastInfo.h"
#include <reactphysics3d/collision/HeightField.h>
#include <reactphysics3d/collision/shapes/HeightFieldShape.h>
#include <reactphysics3d/reactphysics3d.h>
#include <glm/glm.hpp>
#include <utility>


#define DEFQUAT rp3d::Quaternion::identity()

typedef rp3d::RigidBody* RigidBody;

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

    static inline std::vector<std::pair<rp3d::HeightField*, rp3d::HeightFieldShape*>> heightFeilds;

    static inline vec3 toglm(const rp3d::Vector3 &v){return vec3(v.x, v.y, v.z);};
    static inline rp3d::Vector3 torp3d(const vec3 &v){return rp3d::Vector3(v.x, v.y, v.z);}; 

    static inline quat toglm(const rp3d::Quaternion &v){return quat(v.w, v.x, v.y, v.z);};
    static inline rp3d::Quaternion torp3d(const quat &v){return rp3d::Quaternion(v.x, v.y, v.z, v.w);}; 

    static inline mat3 toglm(const rp3d::Matrix3x3 &m){return mat3(
        m[0].x, m[0].y, m[0].z,
        m[1].x, m[1].y, m[1].z,
        m[2].x, m[2].y, m[2].z
        );};
    static inline rp3d::Matrix3x3 torp3d(const mat3 &m){return rp3d::Matrix3x3(
        m[0].x, m[0].y, m[0].z,
        m[1].x, m[1].y, m[1].z,
        m[2].x, m[2].y, m[2].z
        );}; 

    static inline float currentPhysicFreq = 0.f;

    static inline TickTimer physicInterpolationTick;
    static inline std::mutex physicInterpolationMutex;
    static inline bool doPhysicInterpolation = true;
    
    class GroundedRayCastCallback : public rp3d::RaycastCallback
    {
    public:
        glm::vec3 hitPoint;
        glm::vec3 hitNormal;
        glm::vec3 startPoint;
        // rp3d::Body* hitBody = nullptr;
        // rp3d::Collider* hitCollider = nullptr;

        GroundedRayCastCallback(glm::vec3 startPoint) : startPoint(startPoint)
        {
        }

        GroundedRayCastCallback() 
        {
        }


        bool hit = false;
        float minDistance = std::numeric_limits<float>::max();

        rp3d::decimal notifyRaycastHit(const rp3d::RaycastInfo& info) override
        {
            float distance = glm::length(startPoint - PG::toglm(info.worldPoint));
            if (distance < minDistance)
            {
                hitPoint = PG::toglm(info.worldPoint);
                hitNormal = PG::toglm(info.worldNormal);
                // hitBody = info.body;
                // hitCollider = info.collider;
            }
            hit = true;
            return 1.0f;
        }

        GroundedRayCastCallback& operator=(const GroundedRayCastCallback& other)
        {
            this->hitPoint = other.hitPoint;
            this->hitNormal = other.hitNormal;
            this->startPoint = other.startPoint;
            this->hit = other.hit;
            this->minDistance = other.minDistance;
            return *this;
        }
    };
};

