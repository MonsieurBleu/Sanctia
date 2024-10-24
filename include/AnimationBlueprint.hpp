#include <Graphics/AnimationController.hpp>
#include <SanctiaEntity.hpp>

class Entity;

namespace AnimBlueprint
{
    AnimationControllerRef bipedMoveset(const std::string & prefix, Entity *e);

    float weaponAttackCallback(
        float prct, 
        Entity *e, 
        float begin, 
        float end, 
        float dmgMult, 
        int maxTrigger,
        ActionState::LockedDeplacement lockDep,
        float maxSpeed,
        EquipementSlots slot = EquipementSlots::WEAPON_SLOT
        );

    extern std::function<void (void *)> weaponAttackExit;
    extern std::function<void (void *)> weaponStunExit;
    extern std::function<void (void *)> weaponGuardExit;
    extern std::function<void (void *)> weaponGuardEnter;

    extern std::function<void (void *)> weaponBlockExit;
    // extern std::function<void (void *)> weaponBlockExit;

    void PrepareAnimationsCallbacks();
}


#define INV_ANIMATION_SWITCH(func) \
    auto inv_##func = [](void * usr) {return !func(usr);};

#define ANIMATION_SWITCH_ENTITY(name, code) \
    auto name = [](void * usr){\
    Entity *e = (Entity*)usr; \
    code \
    }; \
    INV_ANIMATION_SWITCH(name)

#define ANIMATION_SWITCH(name, code) \
    auto name = [](void * usr){\
    code \
    }; \
    INV_ANIMATION_SWITCH(name)

#define ANIMATION_CALLBACK(code) \
    [](float prct, void * usr){\
    Entity *e = (Entity*)usr; \
    code \
    }; \

#define ANIMATION_SWITCH_AND(func1, func2) \
    std::function<bool(void*)> func1##_AND_##func2 = [](void * usr) {return func1(usr) && func2(usr);};

