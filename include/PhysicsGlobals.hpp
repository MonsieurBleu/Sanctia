#pragma once 

#include <reactphysics3d/reactphysics3d.h>


enum CollideCategory
{
    HITZONE      = 1<<0,
    ENVIRONEMENT = 1<<1
};

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
