#include <BluePhysics.hpp>
#include <iostream>

void B_Collider::setSphere(float radius)
{
    v1.x = radius;
    type = B_ColliderType::Sphere;
}

void B_Collider::setCapsule(vec3 p1, vec3 p2)
{
    v1 = p1;
    v2 = p2;
    type = B_ColliderType::Capsule;
}

void B_Collider::settAABB(vec3 min, vec3 max)
{
    v1 = min;
    v2 = max;
    type = B_ColliderType::AABB;
}

void B_Collider::setOBB(vec3 p1, vec3 p2, vec3 p3)
{
    v1 = p1;
    v2 = p2;
    v3 = p3;
    type = B_ColliderType::OBB;
}

CollisionInfo B_Collider::collide(B_Collider& c1, vec3 p1, B_Collider &c2, vec3 p2)
{
    B_Collider a = c1.type < c2.type ? c1 : c2;
    B_Collider b = c1.type < c2.type ? c2 : c1;

    vec3 pa = c1.type < c2.type ? p1 : p2;
    vec3 pb = c1.type < c2.type ? p2 : p1;

    switch (a.type)
    {
        case B_ColliderType::Sphere :
            switch(b.type)
            {
                case B_ColliderType::Sphere : return collideSphereSphere(a, pa, b, pb);
                case B_ColliderType::Capsule : return collideSphereCapsule(a, pa, b, pb);
                case B_ColliderType::AABB : return collideSphereAABB(a, pa, b, pb);
                case B_ColliderType::OBB : return collideSphereOBB(a, pa, b, pb);
                default : break;
            }
        
        case B_ColliderType::Capsule :
            switch(b.type)
            {
                case B_ColliderType::Capsule : return collideCapsuleCapsule(a, pa, b, pb);
                case B_ColliderType::AABB : return collideCapsuleAABB(a, pa, b, pb);
                case B_ColliderType::OBB : return collideCapsuleOBB(a, pa, b, pb);
                default : break;
            }

        case B_ColliderType::AABB :
            switch(b.type)
            {
                case B_ColliderType::AABB : return collideAABBAABB(a, pa, b, pb);
                case B_ColliderType::OBB : return collideAABBOBB(a, pa, b, pb);
                default : break;
            }
        
        case B_ColliderType::OBB :
            switch(b.type)
            {
                case B_ColliderType::OBB : return collideOBBOBB(a, pa, b, pb);
                default : break;
            }

        default : break;
    }

    return CollisionInfo();
}

CollisionInfo B_Collider::collideSphereSphere(B_Collider& sphere, vec3 ps, B_Collider& sphere2, vec3 ps2){return CollisionInfo();}

CollisionInfo B_Collider::collideSphereAABB(B_Collider& sphere, vec3 ps, B_Collider& AABB, vec3 paabb)
{
    vec3 minP = AABB.v1 + paabb;
    vec3 maxP = AABB.v2 + paabb;
    vec3 closest = max(minP, min(maxP, ps));
    vec3 sToAABB = closest-ps;

    float distance = length(sToAABB);
    
    CollisionInfo res;
    res.penetration = sphere.v1.x-distance;
    res.normal = distance >= 1e-6f ? normalize(sToAABB) : normalize(closest+ps);

    return res;
}

CollisionInfo B_Collider::collideSphereCapsule(B_Collider& sphere, vec3 ps, B_Collider& Capsule, vec3 pc){return CollisionInfo();}

CollisionInfo B_Collider::collideSphereOBB(B_Collider& sphere, vec3 ps, B_Collider& OBB, vec3 pobb){return CollisionInfo();}

CollisionInfo B_Collider::collideCapsuleCapsule(B_Collider& Capsule, vec3 pc, B_Collider& Capsule2, vec3 pc2){return CollisionInfo();}

CollisionInfo B_Collider::collideCapsuleAABB(B_Collider& Capsule, vec3 pc, B_Collider AABB, vec3 paabb){return CollisionInfo();}

CollisionInfo B_Collider::collideCapsuleOBB(B_Collider& Capsule, vec3 pc, B_Collider OBB, vec3 poob){return CollisionInfo();}

CollisionInfo B_Collider::collideAABBAABB(B_Collider AABB, vec3 paabb, B_Collider AABB2, vec3 paabb2){return CollisionInfo();}

CollisionInfo B_Collider::collideAABBOBB(B_Collider AABB, vec3 paabb, B_Collider OBB, vec3 poob){return CollisionInfo();}

CollisionInfo B_Collider::collideOBBOBB(B_Collider OBB, vec3 poob, B_Collider OBB2, vec3 poob2){return CollisionInfo();}
