#include <AnimationBlueprint.hpp>
#include <SanctiaEntity.hpp>
#include <AssetManager.hpp>
#include <Animation.hpp>
#include <Skeleton.hpp>
#include <Utils.hpp>
#include <MathsUtils.hpp>
#include <Constants.hpp>

#define ACT(animA, animB, time, cond) AnimationControllerTransition(animA, animB, COND_CUSTOM, time, TRANSITION_SMOOTH, cond)

#define ACT_TO_ATTACK_ANGLE(animA, animB, time, cond) \
    ACT(animA, animB##S, time, cond##Special),\
    ACT(animA, animB##L, time, cond##Left),\
    ACT(animA, animB##R, time, cond##Right)

#define ACT_FROM_ATTACK_ANGLE(animA, animB, time, cond) \
    ACT(animA##S, animB, time, cond),\
    ACT(animA##L, animB, time, cond),\
    ACT(animA##R, animB, time, cond)

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

#define ACT_FROM_ALLDIR_TO_ATTACK_ANGLE(animA, animB, time, cond) \
    ACT_FROM_ALLDIR(animA, animB##S, time, cond##Special),\
    ACT_FROM_ALLDIR(animA, animB##L, time, cond##Left),\
    ACT_FROM_ALLDIR(animA, animB##R, time, cond##Right)

/* ANIMATION SWITCH */
ANIMATION_SWITCH_ENTITY(switchAttackLeft, 
    auto &s = e->comp<EntityActionState>();
    return s.isTryingToAttack && s.stance == EntityActionState::Stance::LEFT;
)

ANIMATION_SWITCH_ENTITY(switchAttackRight, 
    auto &s = e->comp<EntityActionState>();
    return s.isTryingToAttack && s.stance == EntityActionState::Stance::RIGHT;
)

ANIMATION_SWITCH_ENTITY(switchAttackSpecial, 
    auto &s = e->comp<EntityActionState>();
    return s.isTryingToAttack && s.stance == EntityActionState::Stance::SPECIAL;
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

ANIMATION_SWITCH_ENTITY(switchDeath, 
    return !e->comp<EntityStats>().alive;
)

ANIMATION_SWITCH_ENTITY(switchStun, 
    return e->comp<EntityActionState>().stun;
)

ANIMATION_SWITCH_ENTITY(switchGuard, 
    return e->comp<EntityActionState>().blocking && !switchStun(e);
)

ANIMATION_SWITCH(switchRandom2, 
    return ((int)(globals.appTime.getElapsedTime()*1000.f))%2 == 0;
)

#define ACT_TO_RANDOM(maccro, animA, animB, time, cond) \
    maccro(animA, animB##F, time, cond##_AND_switchRandom2), \
    maccro(animA, animB##B, time, cond##_AND_inv_switchRandom2) 

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

ANIMATION_SWITCH_AND(switchDeath, switchRandom2)
ANIMATION_SWITCH_AND(switchDeath, inv_switchRandom2)

ANIMATION_SWITCH_AND(switchStun, switchRandom2)
ANIMATION_SWITCH_AND(switchStun, inv_switchRandom2)

float AnimBlueprint::weaponAttackCallback(
    float prct, 
    Entity *e, 
    float begin, 
    float end, 
    float dmgMult, 
    int maxTrigger,
    EntityActionState::LockedDeplacement lockDep,
    float acceleration,
    float maxSpeed
    )
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
        newEffect.maxTrigger = maxTrigger;
        newEffect.enable = true;

        w->set<Effect>(newEffect);
    }



    auto &actionState = e->comp<EntityActionState>();
    if(prct >= end)
    {
        actionState.lockDirection = EntityActionState::LockedDeplacement::NONE;
    }
    else if(actionState.lockDirection != lockDep)
    {
        switch (lockDep)
        {
        case EntityActionState::LockedDeplacement::SPEED_ONLY :
            e->comp<B_DynamicBodyRef>()->v *= vec3(0, 1, 0);
            break;
        
        case EntityActionState::LockedDeplacement::DIRECTION :
            actionState.lockedDirection = normalize(e->comp<EntityState3D>().lookDirection * vec3(1, 0, 1));
            break;

        default:
            break;
        }

        actionState.lockDirection = lockDep;
        actionState.lockedMaxSpeed = maxSpeed;
        actionState.lockedAcceleration = acceleration;
    }

    return 1.f;
}

std::function<void (void *)> AnimBlueprint::weaponAttackExit = [](void * usr){
    Entity *e = (Entity*)usr;
    e->comp<EntityActionState>().isTryingToAttack = false;
    e->comp<EntityActionState>().lockDirection = EntityActionState::LockedDeplacement::NONE;
    e->comp<Items>().equipped[WEAPON_SLOT].item->comp<Effect>().enable = false;
};

std::function<void (void *)> AnimBlueprint::weaponStunExit = [](void * usr){
    Entity *e = (Entity*)usr;
    e->comp<EntityActionState>().stun = false;
};

AnimationControllerRef AnimBlueprint::bipedMoveset(const std::string & prefix, Entity *e)
{
    AnimationRef idle   = Loader<AnimationRef>::get(prefix + "_IDLE");

    AnimationRef walkF = Loader<AnimationRef>::get(prefix + "_WALK_F");
    AnimationRef walkB = Loader<AnimationRef>::get(prefix + "_WALK_B");
    AnimationRef walkL = Loader<AnimationRef>::get(prefix + "_WALK_L");
    AnimationRef walkR = Loader<AnimationRef>::get(prefix + "_WALK_R");

    AnimationRef runF = Loader<AnimationRef>::get(prefix + "_RUN_F");
    AnimationRef runB = Loader<AnimationRef>::get(prefix + "_RUN_B");
    AnimationRef runL = Loader<AnimationRef>::get(prefix + "_RUN_L");
    AnimationRef runR = Loader<AnimationRef>::get(prefix + "_RUN_R");

    AnimationRef attackL = Loader<AnimationRef>::get(prefix + "_ATTACK_L");
    AnimationRef attackR = Loader<AnimationRef>::get(prefix + "_ATTACK_R");
    AnimationRef attackS = Loader<AnimationRef>::get(prefix + "_ATTACK_S");

    AnimationRef runAttackL = Loader<AnimationRef>::get(prefix + "_RUN_ATTACK_L");
    AnimationRef runAttackR = Loader<AnimationRef>::get(prefix + "_RUN_ATTACK_R");
    AnimationRef runAttackS = Loader<AnimationRef>::get(prefix + "_RUN_ATTACK_S");

    AnimationRef deathB = Loader<AnimationRef>::get(prefix + "_DEATH_B");
    AnimationRef deathF = Loader<AnimationRef>::get(prefix + "_DEATH_F");

    AnimationRef impactB = Loader<AnimationRef>::get(prefix + "_IMPACT_B");
    AnimationRef impactF = Loader<AnimationRef>::get(prefix + "_IMPACT_F");

    AnimationRef guardL = Loader<AnimationRef>::get(prefix + "_GUARD_L");
    AnimationRef guardR = Loader<AnimationRef>::get(prefix + "_GUARD_R");

    AnimationRef guardImpactL = Loader<AnimationRef>::get(prefix + "_GUARD_IMPACT_L");
    AnimationRef guardImpactR = Loader<AnimationRef>::get(prefix + "_GUARD_IMPACT_R");

    const float D_walkDep = 0.25;
    const float D_runDep = 0.15;
    const float D_idleWalk = 0.25; 
    const float D_WalkRun = 0.25; 
    const float D_toAttack = 0.15;
    const float D_toDeath = 0.15;
    const float D_toStun = 0.15;

    AnimationControllerRef ac( new AnimationController(
        {
            /* DEATH*/
            ACT_TO_RANDOM(ACT, idle, death, D_toDeath, switchDeath),
            ACT_TO_RANDOM(ACT_FROM_ALLDIR, walk, death, D_toDeath, switchDeath),
            ACT_TO_RANDOM(ACT_FROM_ALLDIR, run, death, D_toDeath, switchDeath),
            ACT_TO_RANDOM(ACT_FROM_ATTACK_ANGLE, attack, death, D_toDeath, switchDeath),
            ACT_TO_RANDOM(ACT_FROM_ATTACK_ANGLE, runAttack, death, D_toDeath, switchDeath),

            /* IDLE */
            ACT_TO_ALLDIR(idle, walk, D_idleWalk, switchWalk),
            ACT_TO_ATTACK_ANGLE(idle, attack, D_toAttack, switchAttack),

            /* WALK */
            ACT_DEP_ALLDIR(walk, D_walkDep, switchWalk),
            ACT_FROM_ALLDIR(walk, idle, D_idleWalk, inv_switchWalk),
            ACT_TO_FROM_ALLDIR(walk, run, D_WalkRun, switchRun),
            ACT_FROM_ALLDIR_TO_ATTACK_ANGLE(walk, attack, D_toAttack, switchAttack),

            /* RUN */
            ACT_DEP_ALLDIR(run, D_runDep, switchRun),
            ACT_TO_FROM_ALLDIR(run, walk, D_WalkRun, inv_switchRun),
            ACT_FROM_ALLDIR_TO_ATTACK_ANGLE(run, runAttack, D_toAttack, switchAttack),

            /* ATTACK */
            AnimationControllerTransition(attackL, idle, COND_ANIMATION_FINISHED, D_toAttack, TRANSITION_SMOOTH),
            AnimationControllerTransition(attackR, idle, COND_ANIMATION_FINISHED, D_toAttack, TRANSITION_SMOOTH),
            AnimationControllerTransition(attackS, idle, COND_ANIMATION_FINISHED, D_toAttack, TRANSITION_SMOOTH),

            AnimationControllerTransition(runAttackL, idle, COND_ANIMATION_FINISHED, D_toAttack, TRANSITION_SMOOTH),
            AnimationControllerTransition(runAttackR, idle, COND_ANIMATION_FINISHED, D_toAttack, TRANSITION_SMOOTH),
            AnimationControllerTransition(runAttackS, idle, COND_ANIMATION_FINISHED, D_toAttack, TRANSITION_SMOOTH),

            /* IMPACT */
            ACT_TO_RANDOM(ACT, idle, impact, D_toStun, switchStun),
            ACT_TO_RANDOM(ACT_FROM_ALLDIR, walk, impact, D_toStun, switchStun),
            ACT_TO_RANDOM(ACT_FROM_ALLDIR, run, impact, D_toStun, switchStun),
            ACT_TO_RANDOM(ACT, attackL, impact, D_toStun, switchStun),
            ACT_TO_RANDOM(ACT, attackR, impact, D_toStun, switchStun),
            AnimationControllerTransition(impactB, idle, COND_ANIMATION_FINISHED, D_toStun, TRANSITION_SMOOTH),
            AnimationControllerTransition(impactF, idle, COND_ANIMATION_FINISHED, D_toStun, TRANSITION_SMOOTH),

        }, idle, e
    ));

    return ac;
}

void AnimBlueprint::PrepareAnimationsCallbacks()
{
    {
        AnimationRef a = Loader<AnimationRef>::get("65_2HSword_ATTACK_R");
        a->onExitAnimation = AnimBlueprint::weaponAttackExit;
        a->speedCallback = ANIMATION_CALLBACK(return AnimBlueprint::weaponAttackCallback(prct, e, 37.5, 70, 1, 1, EntityActionState::SPEED_ONLY, 12, 4););
    }
    {
        AnimationRef a = Loader<AnimationRef>::get("65_2HSword_ATTACK_L");
        a->onExitAnimation = AnimBlueprint::weaponAttackExit;
        a->speedCallback = ANIMATION_CALLBACK(return AnimBlueprint::weaponAttackCallback(prct, e, 37.5, 70, 1, 1, EntityActionState::SPEED_ONLY, 12, 4););
    }
    {
        AnimationRef a = Loader<AnimationRef>::get("65_2HSword_ATTACK_S");
        a->onExitAnimation = AnimBlueprint::weaponAttackExit;
        a->speedCallback = ANIMATION_CALLBACK(return AnimBlueprint::weaponAttackCallback(prct, e, 37.5, 70, 2, 1, EntityActionState::SPEED_ONLY, 2, 3););
    }
    {
        AnimationRef a = Loader<AnimationRef>::get("65_2HSword_RUN_ATTACK_R");
        a->onExitAnimation = AnimBlueprint::weaponAttackExit;
        a->speedCallback = ANIMATION_CALLBACK(return AnimBlueprint::weaponAttackCallback(prct, e, 37.5, 70, 1, 3, EntityActionState::DIRECTION, 10, 5););
    }
    {
        AnimationRef a = Loader<AnimationRef>::get("65_2HSword_RUN_ATTACK_L");
        a->onExitAnimation = AnimBlueprint::weaponAttackExit;
        a->speedCallback = ANIMATION_CALLBACK(return AnimBlueprint::weaponAttackCallback(prct, e, 37.5, 70, 1, 3, EntityActionState::DIRECTION, 10, 5););
    }
    {
        AnimationRef a = Loader<AnimationRef>::get("65_2HSword_RUN_ATTACK_S");
        a->onExitAnimation = AnimBlueprint::weaponAttackExit;
        a->speedCallback = ANIMATION_CALLBACK(return AnimBlueprint::weaponAttackCallback(prct, e, 37.5, 70, 2, 1, EntityActionState::DIRECTION, 10, 4););
    }
    {
        AnimationRef a = Loader<AnimationRef>::get("65_2HSword_DEATH_B");
        a->repeat = false;
    }
    {
        AnimationRef a = Loader<AnimationRef>::get("65_2HSword_DEATH_F");
        a->repeat = false;
    }
    {
        AnimationRef a = Loader<AnimationRef>::get("65_2HSword_IMPACT_F");
        a->onExitAnimation = AnimBlueprint::weaponStunExit;
    }
    {
        AnimationRef a = Loader<AnimationRef>::get("65_2HSword_IMPACT_B");
        a->onExitAnimation = AnimBlueprint::weaponStunExit;
    }
}