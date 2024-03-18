#pragma once 

#include <deque>
#include <vector>
#include <memory>
#include <list>
#include <glm/glm.hpp>
using namespace glm;

/*
    The initial demo can winstand : 
        10k static collider & 1 dynamic at 0,23ms
        100k static collider & 1 dynamic at 2,30ms
    
    The reaction force simulated throught velocity 
    gives value as close as 0 < x < 1,5e-8 from 
    the expected result.
*/

#include <mutex>
extern std::mutex physicsMutex;

struct CollisionInfo;

enum B_ColliderType
{
    Sphere = 0,
    Capsule = 1,
    AABB = 2,
    OBB = 3
};

class B_Collider
{
    private : 
        vec3 v1 = vec3(0);
        vec3 v2 = vec3(0);
        vec3 v3 = vec3(0);
        B_ColliderType type;

    public : 
        void setSphere(float radius, vec3 position = vec3(0));
        void setCapsule(float radius, vec3 p1, vec3 p2);
        void settAABB(vec3 min, vec3 max);
        void setOBB(vec3 p1, vec3 p2, vec3 p3);

        static CollisionInfo collideSphereSphere(B_Collider& sphere, vec3 ps, B_Collider& sphere2, vec3 ps2);
        static CollisionInfo collideSphereAABB(B_Collider& sphere, vec3 ps, B_Collider& AABB, vec3 paabb);
        static CollisionInfo collideSphereCapsule(B_Collider& sphere, vec3 ps, B_Collider& Capsule, vec3 pc);
        static CollisionInfo collideSphereOBB(B_Collider& sphere, vec3 ps, B_Collider& OBB, vec3 pobb);

        static CollisionInfo collideCapsuleCapsule(B_Collider& Capsule, vec3 pc, B_Collider& Capsule2, vec3 pc2);
        static CollisionInfo collideCapsuleAABB(B_Collider& Capsule, vec3 pc, B_Collider AABB, vec3 paabb);
        static CollisionInfo collideCapsuleOBB(B_Collider& Capsule, vec3 pc, B_Collider OBB, vec3 poob);

        static CollisionInfo collideAABBAABB(B_Collider AABB, vec3 paabb, B_Collider AABB2, vec3 paabb2);
        static CollisionInfo collideAABBOBB(B_Collider AABB, vec3 paabb, B_Collider OBB, vec3 poob);

        static CollisionInfo collideOBBOBB(B_Collider OBB, vec3 poob, B_Collider OBB2, vec3 poob2);

        static CollisionInfo collide(B_Collider& c1, vec3 p1, B_Collider &c2, vec3 p2);

        vec3 AABB_getMin(){return v1;};
        vec3 AABB_getMax(){return v2;};
};

class B_Body
{
    private : 

    public : 
        vec3 position = vec3(0);
        B_Collider boundingCollider;
        // std::vector<B_Collider> listener;

};

struct B_Force : vec3
{
    bool clear = false;
};

class B_PhysicsScene;

class B_DynamicBody : public B_Body
{
    friend B_PhysicsScene;

    private : 
        vec3 a = vec3(0);
        std::vector<B_Force> forces;

    public : 
        vec3 v = vec3(0);
        float mass = 1.f;

        B_Force& applyForce(vec3 force);
        void update(float delta);

        vec3 getAcceleration() const {return a;};
        vec3 getVelocity() const {return v;};
};

/*
    TODO : implement
*/
class B_ListenerBody : public B_Body
{
    
};

typedef std::shared_ptr<B_Body> B_BodyRef;
typedef std::shared_ptr<B_DynamicBody> B_DynamicBodyRef;

struct CollisionInfo
{
    B_DynamicBody* db1 = nullptr; 
    B_DynamicBody* db2 = nullptr; 
    vec3 normal = vec3(1, 0, 0);
    float penetration = 0.f;
};


class B_PhysicsScene
{
    private : 
        std::list<CollisionInfo> collisions;
        void checkCollision(B_DynamicBody *b1, B_DynamicBody *b2);
        void checkCollision(B_DynamicBody *b1, B_Body *b2);
        void checkCollision(B_Body *b1, B_Body *b2);

    public : 

        std::deque<B_BodyRef> level;
        std::deque<B_DynamicBodyRef> dynamics;

        void update(float deltaTime);
        void manageCollisions();
};