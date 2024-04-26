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
    v4 = p1;
    v5 = p2;
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

void B_Collider::applyTranslation(vec3 pos, vec3 direction)
{
    // direction = vec3(1, 0, 0);
    mat4 rot = glm::lookAt(vec3(0), direction, vec3(0, 1, 0));

    switch (type)
    {
    case B_ColliderType::Sphere :
        v2 = v3 + pos + direction*v3.x;
        // std::cout << to_string(b.v2) << "\n";
        break;
    
    case B_ColliderType::Capsule : 
        v4 = vec3(vec4(v2, 1.0) * rot);
        v5 = vec3(vec4(v3, 1.0) * rot);
        v4 += pos;
        v5 += pos;
        break;
    
    default:
        break;
    }

}

void B_Collider::applyTranslation(const mat4 &m)
{
    switch (type)
    {
    case B_ColliderType::Sphere :
        v2 = m * vec4(v2, 1);
        break;
    
    case B_ColliderType::Capsule : 
        v4 = m * vec4(v2, 1);
        v5 = m * vec4(v3, 1);
        break;
    
    default:
        break;
    }

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
    vec3 origin = Capsule.v4;
    vec3 direction = normalize(Capsule.v5-Capsule.v4);
    float len = distance(Capsule.v4, Capsule.v5);
    ps += sphere.v2;

    vec3 lineToCenter = ps - origin;
    vec3 closest = origin + clamp(dot(lineToCenter, direction), 0.f, len)*direction;

    CollisionInfo res;
    res.penetration = Capsule.v1.x + sphere.v1.x - distance(closest, ps);
    res.normal = res.penetration > 0.f ? normalize(ps-closest) : vec3(1, 0, 0);

    return res;
}

CollisionInfo B_Collider::collideSphereOBB(const B_Collider& sphere, vec3 ps, const B_Collider& OBB, vec3 pobb){return CollisionInfo();}

vec3 projectPointOnLine(vec3 A, vec3 B, vec3 point)
{
    vec3 AB = B-A;
    float t = dot(point - A, AB) / dot(AB, AB);
    return A + min(max(t, 0.f), 1.f)*AB;
};

CollisionInfo B_Collider::collideCapsuleCapsule(const B_Collider& Capsule, vec3 pc, const B_Collider& Capsule2, vec3 pc2)
{
    vec3 origin1_1 = Capsule.v4;
    vec3 origin1_2 = Capsule.v5;
 
    vec3 origin2_1 = Capsule2.v4;
    vec3 origin2_2 = Capsule2.v5;

    vec3 v0 = origin2_1 - origin1_1;
    vec3 v1 = origin2_2 - origin1_1;
    vec3 v2 = origin2_1 - origin1_2;
    vec3 v3 = origin2_2 - origin1_2;

    float d0 = dot(v0, v0); 
    float d1 = dot(v1, v1); 
    float d2 = dot(v2, v2); 
    float d3 = dot(v3, v3);

    vec3 closest1 = (d2 < d0 || d2 < d1 || d3 < d0 || d3 < d1) ? origin1_2 : origin1_1;

    vec3 closest2 = projectPointOnLine(origin2_1, origin2_2, closest1);
    closest1 = projectPointOnLine(origin1_1, origin1_2, closest2);

    CollisionInfo res;
    res.penetration = Capsule.v1.x + Capsule2.v1.x - distance(closest1, closest2);
    res.normal = res.penetration > 0.f ? normalize(closest1-closest2) : vec3(1, 0, 0);

    return res;
}

CollisionInfo B_Collider::collideCapsuleAABB(const B_Collider& Capsule, vec3 pc, const B_Collider AABB, vec3 paabb)
{
    vec3 minP = AABB.v1 + paabb;
    vec3 maxP = AABB.v2 + paabb;

    vec3 origin = Capsule.v4;
    vec3 direction = normalize(Capsule.v5-Capsule.v4);
    float len = distance(Capsule.v4, Capsule.v5);

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
