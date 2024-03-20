#include <BluePhysics.hpp>
#include <iostream>

#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtx/string_cast.hpp>


void B_Collider::setSphere(float radius, vec3 position)
{
    v1.x = radius;
    v2 = position;
    v3 = position;
    type = B_ColliderType::Sphere;
}

void B_Collider::setCapsule(float radius, vec3 p1, vec3 p2)
{
    v1.x = radius;
    v2 = p1;
    v3 = p2;
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

CollisionInfo B_Collider::collide(const B_Collider& c1, vec3 p1, const B_Collider &c2, vec3 p2)
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

CollisionInfo B_Collider::collideSphereSphere(const B_Collider& sphere, vec3 ps, const B_Collider& sphere2, vec3 ps2)
{
    ps += sphere.v2;
    ps2 += sphere2.v2;

    float distance = glm::distance(ps, ps2);

    CollisionInfo res;
    res.penetration = max(0.f, sphere.v1.x+sphere2.v1.x-distance);
    res.normal = distance >= 1e-6f ? (ps-ps2)/distance : vec3(1, 0, 0);
    
    return res;
}

CollisionInfo B_Collider::collideSphereAABB(const B_Collider& sphere, vec3 ps, const B_Collider& AABB, vec3 paabb)
{
    vec3 minP = AABB.v1 + paabb;
    vec3 maxP = AABB.v2 + paabb;
    ps += sphere.v2;
    vec3 closest = max(minP, min(maxP, ps));
    vec3 sToAABB = closest-ps;

    float distance = length(sToAABB);
    
    CollisionInfo res;
    res.penetration = sphere.v1.x-distance;
    res.normal = distance >= 1e-6f ? normalize(sToAABB) : normalize(closest+ps);

    // if(res.penetration > 1e-5)
    // std::cout << "collision " 
    // << to_string(closest) << "\t" 
    // << to_string(ps) << "\t" 
    // << to_string(sToAABB) << "\t" 
    // << to_string(closest+ps) << "\t" 
    // << to_string(res.normal) << "\n";

    return res;
}

CollisionInfo B_Collider::collideSphereCapsule(const B_Collider& sphere, vec3 ps, const B_Collider& Capsule, vec3 pc)
{
    vec3 origin = Capsule.v2 + pc;
    vec3 direction = normalize(Capsule.v3-Capsule.v2);
    float len = distance(Capsule.v2, Capsule.v3);
    ps += sphere.v2;

    vec3 lineToCenter = ps - origin;
    vec3 closest = origin + clamp(dot(lineToCenter, direction), 0.f, len)*direction;

    CollisionInfo res;
    res.penetration = Capsule.v1.x + sphere.v1.x - distance(closest, ps);
    res.normal = res.penetration > 0.f ? normalize(ps-closest) : vec3(1, 0, 0);

    return res;
}

CollisionInfo B_Collider::collideSphereOBB(const B_Collider& sphere, vec3 ps, const B_Collider& OBB, vec3 pobb){return CollisionInfo();}

CollisionInfo B_Collider::collideCapsuleCapsule(const B_Collider& Capsule, vec3 pc, const B_Collider& Capsule2, vec3 pc2)
{
    vec3 origin1_1 = Capsule.v2 + pc;
    vec3 origin1_2 = Capsule.v3 + pc;
    vec3 direction = normalize(Capsule.v3-Capsule.v2);
    float len = distance(Capsule.v2, Capsule.v3);

    vec3 origin2_1 = Capsule2.v2 + pc2;
    vec3 origin2_2 = Capsule2.v3 + pc2;
    vec3 direction2 = normalize(Capsule2.v3-Capsule2.v2);
    float len2 = distance(Capsule2.v2, Capsule2.v3);

    vec3 u = origin1_2 - origin1_1;
    vec3 v = origin2_2 - origin2_1;
    vec3 w0 = origin1_1 - origin2_1;

    float a = dot(u, u); float b = dot(u, v);
    float c = dot(v, v); float d = dot(u, w0);
    float e = dot(v, w0);

    float dem = a*c - b*b;

    float t1 = dem > 0.f ? (b*e-c*d)/dem : 0.f;
    float t2 = dem > 0.f ? (a*e-b*d)/dem : dot(-w0, u)/a;

    fvec3 closest1 = origin1_1 + clamp(t1, 0.f, len)*direction;
    fvec3 closest2 = origin2_1 + clamp(t2, 0.f, len2)*direction2;

    CollisionInfo res;
    res.penetration = Capsule.v1.x + Capsule2.v1.x - distance(closest1, closest2);
    res.normal = res.penetration > 0.f ? normalize(closest1-closest2) : vec3(1, 0, 0);

    return res;
}

CollisionInfo B_Collider::collideCapsuleAABB(const B_Collider& Capsule, vec3 pc, const B_Collider AABB, vec3 paabb)
{
    vec3 minP = AABB.v1 + paabb;
    vec3 maxP = AABB.v2 + paabb;

    vec3 origin = Capsule.v2 + pc;
    vec3 direction = normalize(Capsule.v3-Capsule.v2);
    float len = distance(Capsule.v2, Capsule.v3);

    vec3 toBox = (AABB.v1+AABB.v2)*0.5f + paabb - origin;
    vec3 closest = origin + clamp(dot(toBox, direction), 0.f, len)*direction;
    
    vec3 closestAABB = clamp(closest, minP, maxP);
    CollisionInfo res;
    res.penetration = Capsule.v1.x - distance(closest, closestAABB);
    res.normal = res.penetration > 0.f ? normalize(closestAABB-closest) : vec3(1, 0, 0);

    return res;
}

CollisionInfo B_Collider::collideCapsuleOBB(const B_Collider& Capsule, vec3 pc, const B_Collider OBB, vec3 poob){return CollisionInfo();}

CollisionInfo B_Collider::collideAABBAABB(const B_Collider AABB, vec3 paabb, const B_Collider AABB2, vec3 paabb2){return CollisionInfo();}

CollisionInfo B_Collider::collideAABBOBB(const B_Collider AABB, vec3 paabb, const B_Collider OBB, vec3 poob){return CollisionInfo();}

CollisionInfo B_Collider::collideOBBOBB(const B_Collider OBB, vec3 poob, const B_Collider OBB2, vec3 poob2){return CollisionInfo();}
