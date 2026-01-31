#include <PhysicsGlobals.hpp>

rp3d::decimal PG::RayCastCallback::notifyRaycastHit(const rp3d::RaycastInfo& info)
{
    float distance = glm::length(startPoint - PG::toglm(info.worldPoint));
    // std::cout << "info.worldPoint: (" << info.worldPoint.x << ", " << info.worldPoint.y << ", " << info.worldPoint.z << ")" << std::endl;
    if (distance < minDistance)
    {
        hitPoint = PG::toglm(info.worldPoint);
        hitNormal = PG::toglm(info.worldNormal);
        minDistance = distance;
        // hitBody = info.body;
        // hitCollider = info.collider;
    }
    hit = true;
    return info.hitFraction;
}

PG::RayCastCallback& PG::RayCastCallback::operator=(const PG::RayCastCallback& other)
{
    this->hitPoint = other.hitPoint;
    this->hitNormal = other.hitNormal;
    this->startPoint = other.startPoint;
    this->hit = other.hit;
    this->minDistance = other.minDistance;
    return *this;
}