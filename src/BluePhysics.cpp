#include <BluePhysics.hpp>
#include <iostream>
#include <glm/gtx/string_cast.hpp>

B_Force& B_DynamicBody::applyForce(vec3 force)
{
    for(auto &f : forces)
    if(f.clear)
    {
        f = {force, false};
        return f;
    }

    forces.push_back({force, false});
    return forces.back();
}

void B_DynamicBody::update(float delta)
{
    float imass = 1.f/mass;
    
    a = vec3(0);

    for(auto &f : forces)
        if(!f.clear)
            a += (vec3)f;

    v += a*delta*imass;
    position += v*delta;

    // std::cout << to_string(v) << "\n";
}

void B_PhysicsScene::checkCollision(B_DynamicBody *b1, B_DynamicBody *b2)
{
    CollisionInfo c = B_Collider::collide(b1->boundingCollider, b1->position, b2->boundingCollider, b2->position);
    c.db1 = b1;
    c.db2 = b2;
    if(c.penetration > 1e-5) collisions.push_back(c);
}

void B_PhysicsScene::checkCollision(B_DynamicBody *b1, B_Body *b2)
{
    CollisionInfo c = B_Collider::collide(b1->boundingCollider, b1->position, b2->boundingCollider, b2->position);
    c.db1 = b1;
    if(c.penetration > 1e-5) collisions.push_back(c);
}

void B_PhysicsScene::checkCollision(B_Body *b1, B_Body *b2)
{
    CollisionInfo c = B_Collider::collide(b1->boundingCollider, b1->position, b2->boundingCollider, b2->position);
    if(c.penetration > 1e-5) collisions.push_back(c);
}

void B_PhysicsScene::update(float deltaTime)
{
    for(auto i : dynamics)
        i->update(deltaTime);

    manageCollisions();

    for(auto i = dynamics.begin(); i != dynamics.end(); i++)
    {
        for(auto j = level.begin(); j != level.end(); j++)
            checkCollision(i->get(), j->get());

        for(auto j = i+1; j != dynamics.end(); j++)
            checkCollision(i->get(), j->get());
    }
}

void B_PhysicsScene::manageCollisions()
{
    for(auto &c : collisions)
    {
        if(!c.db1) continue;

        vec3 relativeVelocity = c.db1->v - (c.db2 ? c.db2->v : vec3(0));
        float normalVelocity = max(dot(c.normal, relativeVelocity), 0.f);

        c.db1->position -= c.normal * c.penetration;

        c.db1->v -= c.normal * normalVelocity;

        // std::cout << to_string(c.db1->v) << "\t" << to_string(c.normal) << "\t" << to_string(vec2(normalVelocity, c.penetration)) << "\n";
    }

    collisions.clear();
}