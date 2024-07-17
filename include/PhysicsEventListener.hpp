#pragma once 

#include <SanctiaEntity.hpp>

#define _OverlapPair_ reactphysics3d::OverlapCallback::OverlapPair

class PhysicsEventListener : rp3d::EventListener
{

    virtual void onTrigger(const rp3d::OverlapCallback::CallbackData& callbackData) override;
};


