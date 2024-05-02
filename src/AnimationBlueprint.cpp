#include <AnimationBlueprint.hpp>
#include <SanctiaEntity.hpp>
#include <AssetManager.hpp>
#include <Animation.hpp>
#include <Skeleton.hpp>
#include <Utils.hpp>
#include <MathsUtils.hpp>
#include <Constants.hpp>

#define ACT(animA, animB, time, cond) AnimationControllerTransition(animA, animB, COND_CUSTOM, time, TRANSITION_SMOOTH, cond)

#define ACT_TO_ALLDIR(animA, animB, time, cond) \
    ACT(animA, animB##F, time, cond##_AND_switchDepFront),\
    ACT(animA, animB##B, time, cond##_AND_switchDepBack),\
    ACT(animA, animB##L, time, cond##_AND_switchDepLeft),\
    ACT(animA, animB##R, time, cond##_AND_switchDepRight)

#define ACT_FROM_ALLDIR(animA, animB, time, cond) \
    ACT(animA##F, animB, time, cond),\
    ACT(animA##B, animB, time, cond),\
    ACT(animA##L, animB, time, cond),\
    ACT(animA##R, animB, time, cond)

#define ACT_TO_FROM_ALLDIR(animA, animB, time, cond) \
    ACT_TO_ALLDIR(animA##F, animB, time, cond),\
    ACT_TO_ALLDIR(animA##B, animB, time, cond),\
    ACT_TO_ALLDIR(animA##L, animB, time, cond),\
    ACT_TO_ALLDIR(animA##R, animB, time, cond)

#define ACT_DEP_ALLDIR(anim, time, cond) \
    ACT(anim##B, anim##F, time, cond##_AND_switchDepFront),\
    ACT(anim##L, anim##F, time, cond##_AND_switchDepFront),\
    ACT(anim##R, anim##F, time, cond##_AND_switchDepFront),\
    \
    ACT(anim##F, anim##B, time, cond##_AND_switchDepBack),\
    ACT(anim##L, anim##B, time, cond##_AND_switchDepBack),\
    ACT(anim##R, anim##B, time, cond##_AND_switchDepBack),\
    \
    ACT(anim##F, anim##L, time, cond##_AND_switchDepLeft),\
    ACT(anim##B, anim##L, time, cond##_AND_switchDepLeft),\
    ACT(anim##R, anim##L, time, cond##_AND_switchDepLeft),\
    \
    ACT(anim##F, anim##R, time, cond##_AND_switchDepRight),\
    ACT(anim##B, anim##R, time, cond##_AND_switchDepRight),\
    ACT(anim##L, anim##R, time, cond##_AND_switchDepRight)


/* ANIMATION SWITCH */
ANIMATION_SWITCH_ENTITY(switchAttackCond, 
    return e->comp<EntityActionState>().isTryingToAttack;
)

ANIMATION_SWITCH_ENTITY(switchWalk, 
    return e->comp<EntityState3D>().speed > 0.1;
)

ANIMATION_SWITCH_ENTITY(switchRun, 
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

ANIMATION_SWITCH_AND(switchWalk, switchDepFront)
ANIMATION_SWITCH_AND(switchWalk, switchDepBack)
ANIMATION_SWITCH_AND(switchWalk, switchDepLeft)
ANIMATION_SWITCH_AND(switchWalk, switchDepRight)

ANIMATION_SWITCH_AND(inv_switchWalk, switchDepFront)
ANIMATION_SWITCH_AND(inv_switchWalk, switchDepBack)
ANIMATION_SWITCH_AND(inv_switchWalk, switchDepLeft)
ANIMATION_SWITCH_AND(inv_switchWalk, switchDepRight)

ANIMATION_SWITCH_AND(switchRun, switchDepFront)
ANIMATION_SWITCH_AND(switchRun, switchDepBack)
ANIMATION_SWITCH_AND(switchRun, switchDepLeft)
ANIMATION_SWITCH_AND(switchRun, switchDepRight)

ANIMATION_SWITCH_AND(inv_switchRun, switchDepFront)
ANIMATION_SWITCH_AND(inv_switchRun, switchDepBack)
ANIMATION_SWITCH_AND(inv_switchRun, switchDepLeft)
ANIMATION_SWITCH_AND(inv_switchRun, switchDepRight)

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

    AnimationRef walkF = Loader<AnimationRef>::get(prefix + "_WALK_F");
    AnimationRef walkB = Loader<AnimationRef>::get(prefix + "_WALK_B");
    AnimationRef walkL = Loader<AnimationRef>::get(prefix + "_WALK_L");
    AnimationRef walkR = Loader<AnimationRef>::get(prefix + "_WALK_R");

    AnimationRef runF = Loader<AnimationRef>::get(prefix + "_RUN_F");
    AnimationRef runB = Loader<AnimationRef>::get(prefix + "_RUN_B");
    AnimationRef runL = Loader<AnimationRef>::get(prefix + "_RUN_L");
    AnimationRef runR = Loader<AnimationRef>::get(prefix + "_RUN_R");

    const float D_walkDep = 0.25;
    const float D_runDep = 0.15;
    const float D_idleWalk = 0.25; 
    const float D_WalkRun = 0.25; 
    const float D_toAttack = 0.025;

    AnimationControllerRef ac( new AnimationController(
        {
            /* IDLE */
            ACT_TO_ALLDIR(idle, walk, D_idleWalk, switchWalk),
            ACT(idle, attack, D_toAttack, switchAttackCond),

            /* WALK */
            ACT_DEP_ALLDIR(walk, D_walkDep, switchWalk),
            ACT_FROM_ALLDIR(walk, idle, D_idleWalk, inv_switchWalk),
            ACT_TO_FROM_ALLDIR(walk, run, D_WalkRun, switchRun),
            ACT_FROM_ALLDIR(walk, attack, D_toAttack, switchAttackCond),

            /* RUN */
            ACT_DEP_ALLDIR(run, D_runDep, switchRun),
            ACT_TO_FROM_ALLDIR(run, walk, D_WalkRun, inv_switchRun),

            /* ATTACK */
            AnimationControllerTransition(attack, idle, COND_ANIMATION_FINISHED, 0.f, TRANSITION_SMOOTH)
        }, idle, e
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