#include <AnimationBlueprint.hpp>
#include <SanctiaEntity.hpp>
#include <AssetManager.hpp>
#include <Animation.hpp>
#include <Skeleton.hpp>
#include <Utils.hpp>
#include <MathsUtils.hpp>
#include <Constants.hpp>

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

ANIMATION_SWITCH_ENTITY(switchDepLeft, 
    vec2 d = normalize(toHvec2(e->comp<EntityState3D>().lookDirection));
    vec2 b = normalize(toHvec2(e->comp<EntityState3D>().deplacementDirection));
    float a = angle(b, d);
    return a > PI/4.f && a < 3.f*PI/4.f;
)

ANIMATION_SWITCH_ENTITY(switchDepRight, 
    vec2 d = normalize(toHvec2(e->comp<EntityState3D>().lookDirection));
    vec2 b = normalize(toHvec2(e->comp<EntityState3D>().deplacementDirection));
    float a = angle(b, d);
    return a < PI/-4.f && a > 3.f*PI/-4.f;
)

ANIMATION_SWITCH_ENTITY(switchDepFront, 
    vec2 d = normalize(toHvec2(e->comp<EntityState3D>().lookDirection));
    vec2 b = normalize(toHvec2(e->comp<EntityState3D>().deplacementDirection));
    float a = angle(b, d);
    return a < PI/4.f && a > PI/-4.f;
)

ANIMATION_SWITCH_ENTITY(switchDepBack, 
    vec2 d = normalize(toHvec2(e->comp<EntityState3D>().lookDirection));
    vec2 b = normalize(toHvec2(e->comp<EntityState3D>().deplacementDirection));
    float a = angle(b, d);
    return a < -3.f*PI/4.f || a > 3.f*PI/4.f;
)


float AnimBlueprint::weaponAttackCallback(float prct, Entity *e, float begin, float end, float dmgMult)
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
        
        if(!w->hasComp<ItemInfos>())
        {
            WARNING_MESSAGE("Entity " << e->comp<EntityInfos>().name << " doesn't have item infos for applying effect zone.");
            return 1e12;
        }

        Effect newEffect;
        ItemInfos &infos = w->comp<ItemInfos>();
        
        newEffect.type = EffectType::Damage;
        newEffect.zone = infos.dmgZone;
        newEffect.value = dmgMult*infos.dmgMult;
        newEffect.valtype = (int)infos.dmgType;
        newEffect.maxTrigger = 1;
        newEffect.enable = true;

        w->set<Effect>(newEffect);
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
    AnimationRef attack = Loader<AnimationRef>::get(prefix + "_ATTACK");
    AnimationRef run    = Loader<AnimationRef>::get(prefix + "_RUN");

    AnimationRef walkF   = Loader<AnimationRef>::get(prefix + "_WALK_F");
    AnimationRef walkB   = Loader<AnimationRef>::get(prefix + "_WALK_B");
    AnimationRef walkL   = Loader<AnimationRef>::get(prefix + "_WALK_L");
    AnimationRef walkR   = Loader<AnimationRef>::get(prefix + "_WALK_R");

    AnimationControllerRef ac( new AnimationController(
        0,
        {
            AnimationControllerTransition(idle, walkF, COND_CUSTOM, 0.25f, TRANSITION_SMOOTH, switchWalkCond),
            AnimationControllerTransition(idle, attack, COND_CUSTOM, 0.025f, TRANSITION_SMOOTH, switchAttackCond),

            AnimationControllerTransition(walkF, walkL, COND_CUSTOM, 0.1f, TRANSITION_SMOOTH, switchDepLeft),
            AnimationControllerTransition(walkF, walkR, COND_CUSTOM, 0.25f, TRANSITION_SMOOTH, switchDepRight),
            AnimationControllerTransition(walkF, walkB, COND_CUSTOM, 0.25f, TRANSITION_SMOOTH, switchDepBack),
            AnimationControllerTransition(walkF, idle, COND_CUSTOM, 0.25f, TRANSITION_SMOOTH, inv_switchWalkCond), 
            AnimationControllerTransition(walkF, run, COND_CUSTOM, 0.25f, TRANSITION_SMOOTH, switchRunCond), 
            AnimationControllerTransition(walkF, attack, COND_CUSTOM, 0.025f, TRANSITION_SMOOTH, switchAttackCond),

            AnimationControllerTransition(walkL, walkF, COND_CUSTOM, 0.25f, TRANSITION_SMOOTH, switchDepFront),
            AnimationControllerTransition(walkL, walkR, COND_CUSTOM, 0.25f, TRANSITION_SMOOTH, switchDepRight),
            AnimationControllerTransition(walkL, walkB, COND_CUSTOM, 0.25f, TRANSITION_SMOOTH, switchDepBack),

            AnimationControllerTransition(walkR, walkF, COND_CUSTOM, 0.25f, TRANSITION_SMOOTH, switchDepFront),
            AnimationControllerTransition(walkR, walkL, COND_CUSTOM, 0.25f, TRANSITION_SMOOTH, switchDepLeft),
            AnimationControllerTransition(walkR, walkB, COND_CUSTOM, 0.25f, TRANSITION_SMOOTH, switchDepBack),

            AnimationControllerTransition(walkB, walkL, COND_CUSTOM, 0.25f, TRANSITION_SMOOTH, switchDepLeft),
            AnimationControllerTransition(walkB, walkR, COND_CUSTOM, 0.25f, TRANSITION_SMOOTH, switchDepRight),
            AnimationControllerTransition(walkB, walkF, COND_CUSTOM, 0.25f, TRANSITION_SMOOTH, switchDepFront),

            AnimationControllerTransition(run, walkF, COND_CUSTOM, 0.25f, TRANSITION_SMOOTH, inv_switchRunCond), 

            AnimationControllerTransition(attack, idle, COND_ANIMATION_FINISHED, 0.f, TRANSITION_SMOOTH)
        },
        {   
            idle, walkF, attack, run
        },
        e
    ));

    return ac;
}

void AnimBlueprint::PrepareAnimationsCallbacks()
{

    /* 2H Moveset */
    {
        AnimationRef slash = Loader<AnimationRef>::get("65_2HSword_ATTACK");
        slash->onExitAnimation = AnimBlueprint::weaponAttackExit;
        slash->speedCallback = ANIMATION_CALLBACK(return AnimBlueprint::weaponAttackCallback(prct, e, 37.5, 70, 1););
    }


}