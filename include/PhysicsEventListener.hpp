#pragma once 

#include <SanctiaEntity.hpp>

#define _OverlapPair_ reactphysics3d::OverlapCallback::OverlapPair
#define _ContactPair rp3d::CollisionCallback::ContactPair

class PhysicsEventListener : rp3d::EventListener
{

    virtual void onTrigger(const rp3d::OverlapCallback::CallbackData& callbackData) override;

    virtual void onContact(const CollisionCallback::CallbackData& callbackData) override;
};


