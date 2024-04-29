#include <AnimationBlueprint.hpp>
#include <SanctiaEntity.hpp>
#include <AssetManager.hpp>
#include <Animation.hpp>
#include <Skeleton.hpp>


/* ANIMATION SWITCH */
ANIMATION_SWITCH_ENTITY(switchAttackCond, 
    return e->comp<EntityActionState>().isTryingToAttack;
)

ANIMATION_SWITCH_ENTITY(switchWalkCond, 
    return e->comp<EntityState3D>().speed > 0.1;
)

ANIMATION_SWITCH_ENTITY(switchRunCond, 
    return e->comp<EntityState3D>().speed > 7.5;
)


float AnimBlueprint::weaponAttackCallback(float prct, Entity *e, float begin, float end, Effect effect)
{
    auto &w = e->comp<Items>().equipped[WEAPON_SLOT].item;

    if(!w.get())
        return 1.f;
    
    if( prct >= end && w->comp<Effect>().enable)
    {
        /* Disable effect componment and mark it for removal */
        w->comp<Effect>().enable = false;
    }
    if(prct >= begin && prct < end && !w->comp<Effect>().enable)
    {
        /* Enable effect component */
        w->set<Effect>(effect);
    }

    return 1.f;
}

std::function<void (void *)> AnimBlueprint::weaponAttackExit = [](void * usr){
    Entity *e = (Entity*)usr;
    e->comp<EntityActionState>().isTryingToAttack = false;
    e->comp<Items>().equipped[WEAPON_SLOT].item->comp<Effect>().enable = false;
};

AnimationControllerRef AnimBlueprint::bipedMoveset(const std::string & prefix, Entity *e)
{

    AnimationRef idle   = Loader<AnimationRef>::get(prefix + "_IDLE");
    AnimationRef walk   = Loader<AnimationRef>::get(prefix + "_WALK");
    AnimationRef attack = Loader<AnimationRef>::get(prefix + "_ATTACK");
    AnimationRef run    = Loader<AnimationRef>::get(prefix + "_RUN");

    AnimationControllerRef ac( new AnimationController(
        0,
        {
            AnimationControllerTransition(idle, walk, COND_CUSTOM, 0.25f, TRANSITION_SMOOTH, switchWalkCond),
            AnimationControllerTransition(walk, idle, COND_CUSTOM, 0.25f, TRANSITION_SMOOTH, inv_switchWalkCond), 

            AnimationControllerTransition(walk, run, COND_CUSTOM, 0.25f, TRANSITION_SMOOTH, switchRunCond), 
            AnimationControllerTransition(run, walk, COND_CUSTOM, 0.25f, TRANSITION_SMOOTH, inv_switchRunCond), 

            AnimationControllerTransition(idle, attack, COND_CUSTOM, 0.025f, TRANSITION_SMOOTH, switchAttackCond),
            AnimationControllerTransition(walk, attack, COND_CUSTOM, 0.025f, TRANSITION_SMOOTH, switchAttackCond),

            AnimationControllerTransition(attack, idle, COND_ANIMATION_FINISHED, 0.f, TRANSITION_SMOOTH)
        },
        {   
            idle, walk, attack, run
        },
        e
    ));

    return ac;
}