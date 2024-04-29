#include <AnimationController.hpp>
#include <SanctiaEntity.hpp>

class Entity;

namespace AnimBlueprint
{
    AnimationControllerRef bipedMoveset(const std::string & prefix, Entity *e);

    float weaponAttackCallback(float prct, Entity *e, float begin, float end, Effect effect);
}


#define INV_ANIMATION_SWITCH(func) \
    auto inv_##func = [](void * usr) {return !func(usr);};

#define ANIMATION_SWITCH_ENTITY(name, code) \
    auto name = [](void * usr){\
    Entity *e = (Entity*)usr; \
    code \
    }; \
    INV_ANIMATION_SWITCH(name)

#define ANIMATION_CALLBACK(name, code) \
    auto name = [](float prct, void * usr){\
    Entity *e = (Entity*)usr; \
    code \
    }; \

