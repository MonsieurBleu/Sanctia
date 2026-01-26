#include <AnimationBlueprint.hpp>
#include <SanctiaEntity.hpp>
#include <AssetManager.hpp>
#include <Graphics/Animation.hpp>
#include <Graphics/Skeleton.hpp>
#include <Utils.hpp>
#include <MathsUtils.hpp>
#include <Constants.hpp>

#define ACT(animA, animB, time, cond) AnimationControllerTransition(animA, animB, COND_CUSTOM, time, TRANSITION_SMOOTH, cond)

#define ACT_TO_LRS(animA, animB, time, cond) \
    ACT(animA, animB##S, time, cond##Special),\
    ACT(animA, animB##L, time, cond##Left),\
    ACT(animA, animB##R, time, cond##Right)\

#define ACT_FROM_LRS(animA, animB, time, cond) \
    ACT(animA##S, animB, time, cond),\
    ACT(animA##L, animB, time, cond),\
    ACT(animA##R, animB, time, cond)\

#define ACT_TO_LR(animA, animB, time, cond) \
    ACT(animA, animB##L, time, cond##Left),\
    ACT(animA, animB##R, time, cond##Right)\

#define ACT_FROM_LR(animA, animB, time, cond) \
    ACT(animA##L, animB, time, cond),\
    ACT(animA##R, animB, time, cond)\

#define ACT_TO_FBLR(animA, animB, time, cond) \
    ACT(animA, animB##F, time, cond##_AND_switchDepFront),\
    ACT(animA, animB##B, time, cond##_AND_switchDepBack),\
    ACT(animA, animB##L, time, cond##_AND_switchDepLeft),\
    ACT(animA, animB##R, time, cond##_AND_switchDepRight)\

#define ACT_FROM_FBLR(animA, animB, time, cond) \
    ACT(animA##F, animB, time, cond),\
    ACT(animA##B, animB, time, cond),\
    ACT(animA##L, animB, time, cond),\
    ACT(animA##R, animB, time, cond)\

#define ACT_TO_FROM_FBLR(animA, animB, time, cond) \
    ACT_TO_FBLR(animA##F, animB, time, cond),\
    ACT_TO_FBLR(animA##B, animB, time, cond),\
    ACT_TO_FBLR(animA##L, animB, time, cond),\
    ACT_TO_FBLR(animA##R, animB, time, cond)\

#define ACT_DEP_FBLR(anim, time, cond) \
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
    ACT(anim##L, anim##R, time, cond##_AND_switchDepRight)\

#define ACT_FROM_FBLR_TO_LRS(animA, animB, time, cond) \
    ACT_FROM_FBLR(animA, animB##S, time, cond##Special),\
    ACT_FROM_FBLR(animA, animB##L, time, cond##Left),\
    ACT_FROM_FBLR(animA, animB##R, time, cond##Right)\

#define ACT_FROM_FBLR_TO_LR(animA, animB, time, cond) \
        ACT_TO_LR(animA##F, animB, time, cond), \
        ACT_TO_LR(animA##B, animB, time, cond), \
        ACT_TO_LR(animA##L, animB, time, cond), \
        ACT_TO_LR(animA##R, animB, time, cond) \


/* ANIMATION SWITCH */
ANIMATION_SWITCH_ENTITY(switchAttackLeft, 
    auto &s = e->comp<ActionState>();
    return s.isTryingToAttack && s.stance() == ActionState::Stance::LEFT;
)

ANIMATION_SWITCH_ENTITY(switchAttackRight, 
    auto &s = e->comp<ActionState>();
    return s.isTryingToAttack && s.stance() == ActionState::Stance::RIGHT;
)

ANIMATION_SWITCH_ENTITY(switchAttackSpecial, 
    auto &s = e->comp<ActionState>();
    return s.isTryingToAttack && s.stance() == ActionState::Stance::SPECIAL;
)

ANIMATION_SWITCH_ENTITY(switchAttack, 
    auto &s = e->comp<ActionState>();
    return s.isTryingToAttack && s.stance() == ActionState::Stance::RIGHT;
)

ANIMATION_SWITCH_ENTITY(switchCombo, 
    auto &s = e->comp<EntityStats>();
    e->comp<ActionState>()._wantedStance = ActionState::Stance::LEFT;
    // if(s.adrenaline.cur <= 0)
    // {
    //     s.adrenaline.cur -= 25;
    //     s.adrenaline.cur = max(s.adrenaline.cur, s.adrenaline.min);
    // }
    return s.adrenaline.cur > 0;
)


ANIMATION_SWITCH_ENTITY(switchKick, 
    auto &s = e->comp<ActionState>();
    return s.isTryingToAttack && s.stance() == ActionState::Stance::SPECIAL;
)

ANIMATION_SWITCH_ENTITY(switchWalk, 

    if(!e || !e->has<MovementState>() || !e->has<ActionState>())
        return false;

    auto &depstate = e->comp<MovementState>();
    auto &astate = e->comp<ActionState>();

    return depstate.speed >= 0.1 && !astate.isTryingToBlock && !astate.isTryingToAttack && !astate.attacking && !astate.blocking && !astate.climbing;
)

ANIMATION_SWITCH_ENTITY(switchIdle, 

    if(!e || !e->has<MovementState>() || !e->has<ActionState>())
        return false;

    auto &depstate = e->comp<MovementState>();
    auto &astate = e->comp<ActionState>();

    return depstate.speed < 0.1f && !astate.isTryingToBlock && !astate.isTryingToAttack && !astate.attacking && !astate.blocking;
)

ANIMATION_SWITCH_ENTITY(switchRun, 
    auto &s = e->comp<MovementState>();
    return s.speed >= s.walkSpeed + (s.sprintSpeed + s.walkSpeed)*0.25;
)

ANIMATION_SWITCH_ENTITY(switchDepLeft, 
    vec2 d = normalize(toHvec2(e->comp<state3D>().lookDirection));
    vec2 b = normalize(toHvec2(e->comp<MovementState>().deplacementDirection));
    float a = angle(b, d);
    return a > PI/4.f && a < 3.f*PI/4.f;
)

ANIMATION_SWITCH_ENTITY(switchDepRight, 
    vec2 d = normalize(toHvec2(e->comp<state3D>().lookDirection));
    vec2 b = normalize(toHvec2(e->comp<MovementState>().deplacementDirection));
    float a = angle(b, d);
    return a < PI/-4.f && a > 3.f*PI/-4.f;
)

ANIMATION_SWITCH_ENTITY(switchDepFront, 
    vec2 d = normalize(toHvec2(e->comp<state3D>().lookDirection));
    vec2 b = normalize(toHvec2(e->comp<MovementState>().deplacementDirection));
    float a = angle(b, d);
    return a < PI/4.f && a > PI/-4.f;
)

ANIMATION_SWITCH_ENTITY(switchDepBack, 
    vec2 d = normalize(toHvec2(e->comp<state3D>().lookDirection));
    vec2 b = normalize(toHvec2(e->comp<MovementState>().deplacementDirection));
    float a = angle(b, d);
    return a < -3.f*PI/4.f || a > 3.f*PI/4.f;
)

ANIMATION_SWITCH_ENTITY(switchDeath, 
    return !e->comp<EntityStats>().alive;
)

ANIMATION_SWITCH_ENTITY(switchStun, 
    return e->comp<ActionState>().stun;
)

ANIMATION_SWITCH_ENTITY(switchGuard, 
    auto &s = e->comp<ActionState>();
    return s.isTryingToBlock && !s.stun;
)

ANIMATION_SWITCH_ENTITY(switchEndGuard, 
    auto &s = e->comp<ActionState>();
    float time = globals.simulationTime.getElapsedTime();
    return !s.isTryingToBlock && (time-s.blockingTime) > 0.25 && !s.stun;
)


ANIMATION_SWITCH_ENTITY(switchGuard_to_Idle, 
    auto &s = e->comp<ActionState>();
    return s.isTryingToBlock && !s.stun && !s.hasBlockedAttack;
)

ANIMATION_SWITCH_ENTITY(switchGuardLeft, 
    return switchGuard(e) && e->comp<ActionState>().stance() == ActionState::Stance::LEFT;
)

ANIMATION_SWITCH_ENTITY(switchGuardRight, 
    return switchGuard(e) && e->comp<ActionState>().stance() == ActionState::Stance::RIGHT;
)

ANIMATION_SWITCH_ENTITY(switchGuardSpecial, 
    return switchGuard(e) && e->comp<ActionState>().stance() == ActionState::Stance::SPECIAL;
)

ANIMATION_SWITCH_ENTITY(switchBlock, 
    auto &s = e->comp<ActionState>();
    return s.hasBlockedAttack && !s.stun;
)


ANIMATION_SWITCH_ENTITY(switchBlockLeft, 
    return switchBlock(e) && e->comp<ActionState>().stance() == ActionState::Stance::LEFT;
)

ANIMATION_SWITCH_ENTITY(switchBlockRight, 
    return switchBlock(e) && e->comp<ActionState>().stance() == ActionState::Stance::RIGHT;
)

ANIMATION_SWITCH(switchRandom2, 
    return ((int)(globals.appTime.getElapsedTime()*1000.f))%2 == 0;
)

#define ACT_TO_RANDOM(maccro, animA, animB, time, cond) \
    maccro(animA, animB##F, time, cond##_AND_switchRandom2), \
    maccro(animA, animB##B, time, cond##_AND_inv_switchRandom2) 

#define ACT_TO_RANDOM2(maccro, animA, animB, time, cond) \
    maccro(animA, animB##1, time, cond##_AND_switchRandom2), \
    maccro(animA, animB##2, time, cond##_AND_inv_switchRandom2) 

ANIMATION_SWITCH_AND(switchWalk, switchDepFront)
ANIMATION_SWITCH_AND(switchWalk, switchDepBack)
ANIMATION_SWITCH_AND(switchWalk, switchDepLeft)
ANIMATION_SWITCH_AND(switchWalk, switchDepRight)

ANIMATION_SWITCH_AND(inv_switchWalk, switchDepFront)
ANIMATION_SWITCH_AND(inv_switchWalk, switchDepBack)
ANIMATION_SWITCH_AND(inv_switchWalk, switchDepLeft)
ANIMATION_SWITCH_AND(inv_switchWalk, switchDepRight)

ANIMATION_SWITCH_AND(inv_switchGuard, switchDepFront)
ANIMATION_SWITCH_AND(inv_switchGuard, switchDepBack)
ANIMATION_SWITCH_AND(inv_switchGuard, switchDepLeft)
ANIMATION_SWITCH_AND(inv_switchGuard, switchDepRight)

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

ANIMATION_SWITCH_AND(switchAttack, switchCombo)

/*
*   Create a fast-in fast-out effect on an animation by returning a speed.
*   This version ensure that the animation will have an exact length.
*   The middle section of the animation will be slowed down as a result.
*/
float fifoFixedLength(const float prct, const float currentLenght, const float newLenght, const float fifoScale)
{
    // return 1.f;
    float fifo = (1.f - fifoScale) + 2.f*fifoScale*smoothstep(50.f, 100.f*floor(prct/50.f), prct);
    float speed = (1.f/currentLenght)/newLenght;
    return fifo*speed;
}

/*
*   Create a fast-in fast-out effect on an animation by returning a speed.
*   This version ensure that the middle section of the animation have the same speed as the original.
*   Only the anticipation and recovery part of the animation will be faster.
*   The total length of the animation is not preserved as a result, it will be equals to length/(1-f)
*/
float fifoFixedSpeed(const float prct, const float fifoScale)
{
    // return 1.f;
    float fifo = (1.f - fifoScale) + 2.f*fifoScale*smoothstep(50.f, 100.f*floor(prct/50.f), prct);
    return fifo/(1.f-fifoScale);
}


float AnimBlueprint::weaponAttackCallback(
    float prct, 
    Entity *e, 
    float begin, 
    float end, 
    float dmgMult, 
    int maxTrigger,
    ActionState::LockedMovement lockDep,
    float maxSpeed,
    EquipementSlots slot
    )
{
    if(!e) return 1.f;

    auto &w = e->comp<Items>().equipped[slot].item;

    if(!w.get())
    {
        WARNING_MESSAGE(
            "Empty item slot " ,  EquipementSlotsReverseMap[slot] 
            ,  " of entity " ,  e->comp<EntityInfos>().name ,  ". Animation logic can't be applied here!");
        return 1.f;
    }

    auto &b = w->comp<RigidBody>();

    
    /* Disable effect componment and mark it for removal */
    if( prct >= end && b->isActive())
    {
        physicsMutex.lock();
        b->setIsActive(false);
        physicsMutex.unlock();
    }

    if(prct >= begin && prct < end
        && !b->isActive()
        && w->comp<Effect>().affectedEntities.empty()
    )
    {
        physicsMutex.lock();
        if(!w->has<ItemInfos>())
        {
            WARNING_MESSAGE("Entity " ,  e->comp<EntityInfos>().name ,  " doesn't have item infos for applying effect zone.");
            return 1e12;
        }

        // std::cout << TERMINAL_OK << "Creating effect";

        Effect newEffect;
        ItemInfos &infos = w->comp<ItemInfos>();
        newEffect.type = EffectType::Damage;
        newEffect.value = dmgMult*infos.damageMultiplier;
        newEffect.valtype = (int)infos.dmgType;
        newEffect.maxTrigger = maxTrigger;
        newEffect.usr = e;
        w->set<Effect>(newEffect);

        // std::cout << " with effect type" << EffectTypeReverseMap[w->comp<Effect>().type] << "\n" << TERMINAL_RESET;

        b->setIsActive(true);
        physicsMutex.unlock();
    }

    auto &actionState = e->comp<ActionState>();
    actionState.attacking = true;
    actionState.hasBlockedAttack = false;
    actionState.blocking = false;
    // actionState._stance = actionState._wantedStance;
    if(prct >= end)
    {
        actionState.lockType = ActionState::LockedMovement::NONE;
    }
    else if(actionState.lockType != lockDep)
    {
        if(lockDep == ActionState::LockedMovement::DIRECTION)
            actionState.lockedDirection = normalize(e->comp<state3D>().lookDirection * vec3(1, 0, 1));

        actionState.lockType = lockDep;
        actionState.lockedMaxSpeed = maxSpeed;

        // NOTIF_MESSAGE(actionState.lockedMaxSpeed)
    }

    // float fifoScale = 0.5;
    // float fifo = prct > 50.f ? 
    //         (1.f - fifoScale/2.f) + fifoScale*smoothstep(50.f, 100.f, prct)
    //     :
    //         (1.f - fifoScale/2.f) + fifoScale*smoothstep(50.f, 0.f, prct)
    //     ;

    // fifo = 1.f;

    float fifo = fifoFixedSpeed(prct, 0.25f);

    fifo *= 2.f - smoothstep(0.f, 50.f, prct);

    float speed = e->has<EntityStats>() ? 
        fifo + 0.5*(e->comp<EntityStats>().adrenaline.cur/e->comp<EntityStats>().adrenaline.max)
        :
        fifo;

    return max(speed, 0.01f);
}

std::function<void (void *)> AnimBlueprint::weaponAttackExit = [](void * usr){
    Entity *e = (Entity*)usr;
    if(!e || !e->has<ActionState>()) return;
    e->comp<ActionState>().isTryingToAttack = false;
    e->comp<ActionState>().attacking = false;
    e->comp<ActionState>().lockType = ActionState::LockedMovement::NONE;

    if(e->comp<ActionState>().stance() == ActionState::Stance::SPECIAL)
        e->comp<ActionState>().setStance(ActionState::Stance::RIGHT);

    auto &slots = e->comp<Items>().equipped;

    if(slots[WEAPON_SLOT].item && slots[WEAPON_SLOT].item->has<Effect>())
        slots[WEAPON_SLOT].item->comp<Effect>().clear();
        // slots[WEAPON_SLOT].item->remove<Effect>();
    
    if(slots[FOOT_SLOT].item && slots[FOOT_SLOT].item->has<Effect>())
        slots[FOOT_SLOT].item->comp<Effect>().clear();
        // slots[FOOT_SLOT].item->remove<Effect>();
};

std::function<void (void *)> AnimBlueprint::weaponAttackEnter = [](void * usr){
    Entity *e = (Entity*)usr;
    if(!e || !e->has<ActionState>()) return;
    auto &s = e->comp<ActionState>();
    s._stance = s._wantedStance;
    s.isTryingToAttack = false;

    if(e->has<Items>())
    for(auto &i : e->comp<Items>().equipped)
        if(i.item and i.item->has<RigidBody>() and i.item->comp<RigidBody>() and i.item->comp<RigidBody>()->isActive())
            {
                physicsMutex.lock();
                i.item->comp<RigidBody>()->setIsActive(false);
                physicsMutex.unlock();
            }
};

// std::function<void (void *)> AnimBlueprint::weaponKickExit = [](void * usr){
//     Entity *e = (Entity*)usr;
//     e->comp<ActionState>().isTryingToBlock = false;
//     e->comp<ActionState>().kicking = false;
//     e->comp<ActionState>().lockType = ActionState::LockedMovement::NONE;

//     auto &slots = e->comp<Items>().equipped;

//     if(slots[WEAPON_SLOT].item)
//         slots[WEAPON_SLOT].item->comp<Effect>().clear();
//         // slots[WEAPON_SLOT].item->remove<Effect>();
    
//     if(slots[FOOT_SLOT].item)
//         slots[FOOT_SLOT].item->comp<Effect>().clear();
//         // slots[FOOT_SLOT].item->remove<Effect>();
// };


std::function<void (void *)> AnimBlueprint::weaponStunExit = [](void * usr){
    Entity *e = (Entity*)usr;
    if(!e || !e->has<ActionState>()) return;
    e->comp<ActionState>().stun = false;
};

std::function<void (void *)> AnimBlueprint::weaponGuardEnter = [](void * usr){
    Entity *e = (Entity*)usr;
    if(!e || !e->has<ActionState>()) return;
    auto &s = e->comp<ActionState>();
    s.blocking = true;
    s.blockingTime = globals.simulationTime.getElapsedTime();
    // s.isTryingToBlock = false;
    s.lockType = ActionState::LockedMovement::SPEED_ONLY;
    s.lockedMaxSpeed = 0.5;

    // inputLag.stop();
    // NOTIF_MESSAGE("Input Latency : " ,  inputLag.getDeltaMS())
};

std::function<void (void *)> AnimBlueprint::weaponGuardExit = [](void * usr){
    Entity *e = (Entity*)usr;
    if(!e || !e->has<ActionState>()) return;
    auto &s = e->comp<ActionState>();
    s.blocking = s.hasBlockedAttack;
    // s.blocking = false;
    // s.hasBlockedAttack = false;
    s.lockType = ActionState::LockedMovement::NONE;
};

std::function<void (void *)> AnimBlueprint::weaponBlockExit = [](void * usr){
    Entity *e = (Entity*)usr;
    if(!e || !e->has<ActionState>()) return;
    auto &s = e->comp<ActionState>();
    s.hasBlockedAttack = false;
};

std::function<void (void *)> AnimBlueprint::idleEnter = [](void * usr){
    Entity *e = (Entity*)usr;
    if(!e || !e->has<ActionState>()) return;
    auto &s = e->comp<ActionState>();
    s.hasBlockedAttack = false;
    s.blocking = false;
    s.attacking = false;
};

AnimationControllerRef AnimBlueprint::bipedMoveset_POC2024(const std::string & prefix, Entity *e)
{
    e->set<AnimationControllerInfos>({prefix});

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
    AnimationRef guardS = Loader<AnimationRef>::get(prefix + "_GUARD_S");

    AnimationRef guardImpactL = Loader<AnimationRef>::get(prefix + "_GUARD_IMPACT_L");
    AnimationRef guardImpactR = Loader<AnimationRef>::get(prefix + "_GUARD_IMPACT_R");

    const float D_walkDep = 0.25;
    const float D_runDep = 0.15;
    const float D_idleWalk = 0.25; 
    const float D_WalkRun = 0.25; 
    const float D_toAttack = 0.10;
    const float D_toDeath = 0.15;
    const float D_toStun = 0.15;
    const float D_toGuard = 0.075;
    const float D_toBlock = 0.15;

    AnimationControllerRef ac( new AnimationController(
        {
            /* DEATH*/
            ACT_TO_RANDOM(ACT, idle, death, D_toDeath, switchDeath),
            ACT_TO_RANDOM(ACT_FROM_FBLR, walk, death, D_toDeath, switchDeath),
            ACT_TO_RANDOM(ACT_FROM_FBLR, run, death, D_toDeath, switchDeath),
            ACT_TO_RANDOM(ACT_FROM_LRS, attack, death, D_toDeath, switchDeath),
            ACT_TO_RANDOM(ACT_FROM_LRS, runAttack, death, D_toDeath, switchDeath),
            ACT_TO_RANDOM(ACT_FROM_LR, guard, death, D_toDeath, switchDeath),

            /* IDLE */
            ACT_TO_LRS(idle, guard, D_toGuard, switchGuard),
            ACT_TO_FBLR(idle, walk, D_idleWalk, switchWalk),
            ACT_TO_LRS(idle, attack, D_toAttack, switchAttack),

            /* WALK */
            ACT_DEP_FBLR(walk, D_walkDep, switchWalk),
            ACT_FROM_FBLR(walk, idle, D_idleWalk, inv_switchWalk),
            ACT_TO_FROM_FBLR(walk, run, D_WalkRun, switchRun),
            ACT_FROM_FBLR_TO_LRS(walk, attack, D_toAttack, switchAttack),
            ACT_FROM_FBLR_TO_LRS(walk, guard, D_toGuard, switchGuard),

            /* RUN */
            ACT_DEP_FBLR(run, D_runDep, switchRun),
            ACT_TO_FROM_FBLR(run, walk, D_WalkRun, inv_switchRun),
            ACT_FROM_FBLR_TO_LRS(run, runAttack, D_toAttack, switchAttack),

            /* ATTACK */
            AnimationControllerTransition(attackL, idle, COND_ANIMATION_FINISHED, D_toAttack, TRANSITION_SMOOTH),
            AnimationControllerTransition(attackR, idle, COND_ANIMATION_FINISHED, D_toAttack, TRANSITION_SMOOTH),
            AnimationControllerTransition(attackS, idle, COND_ANIMATION_FINISHED, D_toAttack, TRANSITION_SMOOTH),

            AnimationControllerTransition(runAttackL, idle, COND_ANIMATION_FINISHED, D_toAttack, TRANSITION_SMOOTH),
            AnimationControllerTransition(runAttackR, idle, COND_ANIMATION_FINISHED, D_toAttack, TRANSITION_SMOOTH),
            AnimationControllerTransition(runAttackS, idle, COND_ANIMATION_FINISHED, D_toAttack, TRANSITION_SMOOTH),

            /* IMPACT */
            ACT_TO_RANDOM(ACT, idle, impact, D_toStun, switchStun),
            ACT_TO_RANDOM(ACT_FROM_FBLR, walk, impact, D_toStun, switchStun),
            ACT_TO_RANDOM(ACT_FROM_FBLR, run, impact, D_toStun, switchStun),
            ACT_TO_RANDOM(ACT_FROM_LRS, attack, impact, D_toStun, switchStun),
            ACT_TO_RANDOM(ACT_FROM_LR, runAttack, impact, D_toStun, switchStun),
            ACT_TO_RANDOM(ACT_FROM_LRS, guard, impact, D_toStun, switchStun),
            AnimationControllerTransition(impactB, idle, COND_ANIMATION_FINISHED, D_toStun, TRANSITION_SMOOTH),
            AnimationControllerTransition(impactF, idle, COND_ANIMATION_FINISHED, D_toStun, TRANSITION_SMOOTH),

            /* GUARD */
            ACT_FROM_LR(guard, idle, D_toGuard, inv_switchGuard),
            ACT(guardR, idle, D_toGuard*2.f, inv_switchGuardRight),
            ACT(guardL, idle, D_toGuard*2.f, inv_switchGuardLeft),
            ACT_TO_FBLR(guardL, walk, D_toGuard, inv_switchGuard),
            ACT_TO_FBLR(guardR, walk, D_toGuard, inv_switchGuard),
            // ACT_TO_FBLR(guardS, walk, D_toGuard, inv_switchGuard),
            AnimationControllerTransition(guardS, idle, COND_ANIMATION_FINISHED, D_toAttack, TRANSITION_SMOOTH),

            /* BLOCK */
            ACT(guardL, guardImpactL, D_toBlock, switchBlock),
            ACT(guardR, guardImpactR, D_toBlock, switchBlock),
            AnimationControllerTransition(guardImpactL, guardL, COND_ANIMATION_FINISHED, D_toBlock, TRANSITION_SMOOTH),
            AnimationControllerTransition(guardImpactR, guardR, COND_ANIMATION_FINISHED, D_toBlock, TRANSITION_SMOOTH),

        }, idle, e
    ));

    return ac;
}

void AnimBlueprint::PrepareAnimationsCallbacks()
{
    #define WEAPON_CALLBACK(beg, end, dmgMult, maxTrigger, lockDep, maxspeed, slot) \
        a->onEnterAnimation = weaponAttackEnter;\
        a->onExitAnimation = AnimBlueprint::weaponAttackExit;\
        a->speedCallback = ANIMATION_CALLBACK(return AnimBlueprint::weaponAttackCallback(prct, e, beg, end, dmgMult, maxTrigger, ActionState::LockedMovement::lockDep, maxspeed, EquipementSlots::slot););\
    
    {   AnimationRef a = Loader<AnimationRef>::get("65_2HSword_ATTACK_R");
        WEAPON_CALLBACK(37.5, 70, 1, 1, SPEED_ONLY, 0.5, WEAPON_SLOT);
    }
    {   AnimationRef a = Loader<AnimationRef>::get("65_2HSword_ATTACK_L");
        WEAPON_CALLBACK(37.5, 70, 1, 1, SPEED_ONLY, 0.5, WEAPON_SLOT);
    }
    {   AnimationRef a = Loader<AnimationRef>::get("65_2HSword_ATTACK_S");
        WEAPON_CALLBACK(37.5, 70, 2, 1, SPEED_ONLY, 0.25, WEAPON_SLOT);
    }
    {   AnimationRef a = Loader<AnimationRef>::get("65_2HSword_RUN_ATTACK_R");
        a->repeat = false;
        WEAPON_CALLBACK(37.5, 70, 1, 3, DIRECTION, 5, WEAPON_SLOT);
    }
    {   AnimationRef a = Loader<AnimationRef>::get("65_2HSword_RUN_ATTACK_L");
        a->repeat = false;
        WEAPON_CALLBACK(37.5, 70, 1, 3, DIRECTION, 5, WEAPON_SLOT);
    }
    {   AnimationRef a = Loader<AnimationRef>::get("65_2HSword_RUN_ATTACK_S");
        a->repeat = false;
        WEAPON_CALLBACK(37.5, 70, 2, 1, DIRECTION, 4, WEAPON_SLOT);
    }
    {   AnimationRef a = Loader<AnimationRef>::get("65_2HSword_DEATH_B");
        a->repeat = false;
    }
    {   AnimationRef a = Loader<AnimationRef>::get("65_2HSword_DEATH_F");
        a->repeat = false;
    }
    {   AnimationRef a = Loader<AnimationRef>::get("65_2HSword_IMPACT_F");
        a->onExitAnimation = AnimBlueprint::weaponStunExit;
    }
    {   AnimationRef a = Loader<AnimationRef>::get("65_2HSword_IMPACT_B");
        a->onExitAnimation = AnimBlueprint::weaponStunExit;
    }
    {   AnimationRef a = Loader<AnimationRef>::get("65_2HSword_GUARD_L");
        a->onExitAnimation = AnimBlueprint::weaponGuardExit;
        a->onEnterAnimation = AnimBlueprint::weaponGuardEnter;
    }
    {   AnimationRef a = Loader<AnimationRef>::get("65_2HSword_GUARD_R");
        a->onExitAnimation = AnimBlueprint::weaponGuardExit;
        a->onEnterAnimation = AnimBlueprint::weaponGuardEnter;
    }
    {   AnimationRef a = Loader<AnimationRef>::get("65_2HSword_GUARD_S");
        a->onExitAnimation = AnimBlueprint::weaponAttackExit;
        WEAPON_CALLBACK(37.5, 70, 1, 1, SPEED_ONLY, 0, FOOT_SLOT);
    }
    {   AnimationRef a = Loader<AnimationRef>::get("65_2HSword_GUARD_IMPACT_L");
        a->onExitAnimation = AnimBlueprint::weaponBlockExit;
    }
    {   AnimationRef a = Loader<AnimationRef>::get("65_2HSword_GUARD_IMPACT_R");
        a->onExitAnimation = AnimBlueprint::weaponBlockExit;
    }

    // WEAPON_CALLBACK(beg, end, dmgMult, maxTrigger, lockDep, maxspeed, slot)
    
    /* DEMO 2025 */
    {   AnimationRef a = Loader<AnimationRef>::get("(Human) 2H Sword Attack");
        WEAPON_CALLBACK(33.0, 66, 1, 1, SPEED_ONLY, 0.75, WEAPON_SLOT);
        // const float l = a->getLength();
        // a->speedCallback = [l](float f, void *e){return (1./0.75) / l;};
    }
    {   AnimationRef a = Loader<AnimationRef>::get("(Human) 2H Sword Kick");
        WEAPON_CALLBACK(33.0, 66, 0.1, 1, SPEED_ONLY, 1.5, FOOT_SLOT);
        // const float l = a->getLength();
        // a->speedCallback = [l](float f, void *e){return (1./0.5) / l;};
    }
    {   AnimationRef a = Loader<AnimationRef>::get("(Human) 2H Sword Death_1");
        a->repeat = false;
    }
    {   AnimationRef a = Loader<AnimationRef>::get("(Human) 2H Sword Death_2");
        a->repeat = false;
    }
    {   AnimationRef a = Loader<AnimationRef>::get("(Human) 2H Sword Blocking");
        a->onExitAnimation = AnimBlueprint::weaponGuardExit;
        a->onEnterAnimation = AnimBlueprint::weaponGuardEnter;
    }
    {   AnimationRef a = Loader<AnimationRef>::get("(Human) 2H Sword Blocking_Impact");
        a->onExitAnimation = AnimBlueprint::weaponBlockExit;
        a->repeat = false;
        const float duration = 0.5;
        const float l = a->getLength();
        // a->speedCallback = [l, duration](float f, void *e){return 2*1-(f/100.f);};
        a->speedCallback = [l, duration](float f, void *e){return fifoFixedLength(f, l, duration, 0.25);};
    }
    {   AnimationRef a = Loader<AnimationRef>::get("(Human) 2H Sword Impact");
        a->onExitAnimation = AnimBlueprint::weaponStunExit;
        const float duration = 0.5;
        const float l = a->getLength();
        // a->speedCallback = [l, duration](float f, void *e){return (1./duration)/l;};
        a->speedCallback = [l, duration](float f, void *e){return fifoFixedLength(f, l, duration, 0.25);};
    }
    {   AnimationRef a = Loader<AnimationRef>::get("(Human) 2H Sword Idle");
        a->onEnterAnimation = AnimBlueprint::idleEnter;
    }
}

#define LOAD_ANIM_FROM_PREFIX(name) AnimationRef name = Loader<AnimationRef>::get(prefix + #name);

AnimationControllerRef AnimBlueprint::bipedMoveset_PREALPHA_2025(const std::string & prefix, Entity *e)
{
    LOAD_ANIM_FROM_PREFIX(Idle)

    LOAD_ANIM_FROM_PREFIX(Death_1)
    LOAD_ANIM_FROM_PREFIX(Death_2)

    LOAD_ANIM_FROM_PREFIX(Kick)
    LOAD_ANIM_FROM_PREFIX(Attack)
    LOAD_ANIM_FROM_PREFIX(Impact)
    LOAD_ANIM_FROM_PREFIX(Blocking)
    LOAD_ANIM_FROM_PREFIX(Blocking_Impact)

    LOAD_ANIM_FROM_PREFIX(Walk_F)
    LOAD_ANIM_FROM_PREFIX(Walk_B)
    LOAD_ANIM_FROM_PREFIX(Walk_L)
    LOAD_ANIM_FROM_PREFIX(Walk_R)

    auto walkCallback = [](float f, void *usr)
    {
        Entity *e = (Entity*)usr;
        auto &s = e->comp<MovementState>();

        return max(0.25, 
            1.5 * (
                smoothstep(0.f,s.walkSpeed,s.speed) + 
                smoothstep(s.walkSpeed, s.sprintSpeed, s.speed))
        );
    };

    Walk_F->speedCallback = walkCallback;
    Walk_B->speedCallback = walkCallback;
    Walk_L->speedCallback = walkCallback;
    Walk_R->speedCallback = walkCallback;

    LOAD_ANIM_FROM_PREFIX(Run_F)
    LOAD_ANIM_FROM_PREFIX(Run_B)
    LOAD_ANIM_FROM_PREFIX(Run_L)
    LOAD_ANIM_FROM_PREFIX(Run_R)

    const float D_walkDep       = 1.0*0.25;
    const float D_runDep        = 1.0*0.25;
    const float D_idleWalk      = 1.0*0.25; 
    const float D_WalkRun       = 1.0*0.25;
    const float D_toAttack      = 1.0*0.10;
    const float D_toDeath       = 1.0*0.15;
    const float D_toStun        = 1.0*0.15;
    const float D_toGuard       = 1.0*0.15;
    const float D_toBlock       = 1.0*0.15;
    const float D_toBlockImpact = 1.0*0.05;
    const float D_toCombo       = 1.0*0.25;

    AnimationControllerRef ac( new AnimationController(
        {
            /* DEATH */
            ACT_TO_RANDOM2(ACT, Idle, Death_, D_toDeath, switchDeath),
            ACT_TO_RANDOM2(ACT, Kick, Death_, D_toDeath, switchDeath),
            ACT_TO_RANDOM2(ACT, Attack, Death_, D_toDeath, switchDeath),
            ACT_TO_RANDOM2(ACT, Impact, Death_, D_toDeath, switchDeath),
            ACT_TO_RANDOM2(ACT, Blocking, Death_, D_toDeath, switchDeath),
            ACT_TO_RANDOM2(ACT, Blocking_Impact, Death_, D_toDeath, switchDeath),
            ACT_TO_RANDOM2(ACT_FROM_FBLR, Walk_, Death_, D_toDeath, switchDeath),
            ACT_TO_RANDOM2(ACT_FROM_FBLR, Run_, Death_, D_toDeath, switchDeath),
            
            /* IDLE */
            ACT_TO_FBLR(Idle, Walk_, D_idleWalk, switchWalk),
            ACT(Idle, Attack, D_toAttack, switchAttack),
            ACT(Idle, Kick, D_toAttack, switchKick),
            ACT(Idle, Blocking, D_toGuard, switchGuard),
            ACT(Idle, Impact, D_toStun, switchStun),
            
            /* WALK */
            ACT_DEP_FBLR(Walk_, D_walkDep, switchWalk),
            ACT_FROM_FBLR(Walk_, Idle, D_idleWalk, switchIdle),
            ACT_TO_FROM_FBLR(Walk_, Run_, D_WalkRun, switchRun),
            ACT_FROM_FBLR(Walk_, Attack, D_toAttack, switchAttack),
            ACT_FROM_FBLR(Walk_, Kick, D_toAttack, switchKick),
            ACT_FROM_FBLR(Walk_, Blocking, D_toGuard, switchGuard),
            ACT_FROM_FBLR(Walk_, Impact, D_toStun, switchStun),
            
            /* RUN */
            ACT_DEP_FBLR(Run_, D_runDep, switchRun),
            ACT_TO_FROM_FBLR(Run_, Walk_, D_WalkRun, inv_switchRun),
            ACT_FROM_FBLR(Run_, Idle, D_WalkRun, switchIdle),
            ACT_FROM_FBLR(Run_, Kick, D_toAttack, switchKick),
            ACT_FROM_FBLR(Run_, Attack, D_toAttack, switchAttack),
            ACT_FROM_FBLR(Run_, Blocking, D_toGuard, switchGuard),
            ACT_FROM_FBLR(Run_, Impact, D_toStun, switchStun),
            
            /* ATACK */
            AnimationControllerTransition(Attack, Idle, COND_ANIMATION_FINISHED, D_toAttack, TRANSITION_SMOOTH),
            AnimationControllerTransition(Kick, Idle, COND_ANIMATION_FINISHED, D_toAttack, TRANSITION_SMOOTH),
            ACT(Attack, Impact, D_toStun, switchStun),
            ACT(Kick, Impact, D_toStun, switchStun),
            ACT(Kick, Attack, D_toCombo, switchAttack_AND_switchCombo),

            /* BLOCKING */
            ACT(Blocking, Impact, D_toStun, switchStun),
            ACT(Blocking, Idle, D_toBlock, switchEndGuard),
            ACT(Blocking, Blocking_Impact, D_toBlockImpact, switchBlock),

            AnimationControllerTransition(Blocking_Impact, Blocking, COND_ANIMATION_FINISHED, D_toGuard, TRANSITION_SMOOTH),
            // ACT(Blocking_Impact, Blocking, D_toGuard, switchGuard),
            ACT(Blocking_Impact, Attack, D_toAttack, switchAttack),
            ACT(Blocking_Impact, Kick, D_toAttack, switchKick),
            ACT(Blocking_Impact, Impact, D_toStun, switchStun),
            // ACT(Blocking_Impact, Idle, D_toAttack, inv_switchGuard),

            /* STUN */
            AnimationControllerTransition(Impact, Idle, COND_ANIMATION_FINISHED, D_toStun, TRANSITION_SMOOTH),

        }, Idle, e
    ));

    return ac;
}