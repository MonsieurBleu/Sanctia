#include <AnimationBlueprint.hpp>
#include <SanctiaEntity.hpp>
#include <AssetManager.hpp>
#include <Animation.hpp>
#include <Skeleton.hpp>
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

ANIMATION_SWITCH_ENTITY(switchWalk, 
    return e->comp<EntityState3D>().speed >= 0.1;
)

ANIMATION_SWITCH_ENTITY(switchRun, 
    auto &s = e->comp<EntityState3D>();
    return s.speed >= s.walkSpeed + (s.sprintSpeed + s.walkSpeed)*0.25;
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
    return e->comp<ActionState>().stun;
)

ANIMATION_SWITCH_ENTITY(switchGuard, 
    auto &s = e->comp<ActionState>();
    return s.isTryingToBlock && !s.stun;
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

float AnimBlueprint::weaponAttackCallback(
    float prct, 
    Entity *e, 
    float begin, 
    float end, 
    float dmgMult, 
    int maxTrigger,
    ActionState::LockedDeplacement lockDep,
    float maxSpeed,
    EquipementSlots slot
    )
{
    
    auto &w = e->comp<Items>().equipped[slot].item;
    if(!w.get()) return 1.f;
    auto &b = w->comp<rp3d::RigidBody*>();

    
    /* Disable effect componment and mark it for removal */
    if( prct >= end && b->isActive())
        b->setIsActive(false);

    if(prct >= begin && prct < end
        && !b->isActive()
        && w->comp<Effect>().affectedEntities.empty()
    )
    {
        if(!w->hasComp<ItemInfos>())
        {
            WARNING_MESSAGE("Entity " << e->comp<EntityInfos>().name << " doesn't have item infos for applying effect zone.");
            return 1e12;
        }

        Effect newEffect;
        ItemInfos &infos = w->comp<ItemInfos>();
        newEffect.type = EffectType::Damage;
        newEffect.value = dmgMult*infos.dmgMult;
        newEffect.valtype = (int)infos.dmgType;
        newEffect.maxTrigger = maxTrigger;
        newEffect.usr = e;
        w->set<Effect>(newEffect);

        b->setIsActive(true);
    }

    auto &actionState = e->comp<ActionState>();
    actionState.attacking = true;
    if(prct >= end)
    {
        actionState.lockType = ActionState::LockedDeplacement::NONE;
    }
    else if(actionState.lockType != lockDep)
    {
        if(lockDep == ActionState::LockedDeplacement::DIRECTION)
            actionState.lockedDirection = normalize(e->comp<EntityState3D>().lookDirection * vec3(1, 0, 1));

        actionState.lockType = lockDep;
        actionState.lockedMaxSpeed = maxSpeed;
    }

    return 1.f;
}

std::function<void (void *)> AnimBlueprint::weaponAttackExit = [](void * usr){
    Entity *e = (Entity*)usr;
    e->comp<ActionState>().isTryingToAttack = false;
    e->comp<ActionState>().attacking = false;
    e->comp<ActionState>().lockType = ActionState::LockedDeplacement::NONE;

    auto &slots = e->comp<Items>().equipped;

    if(slots[WEAPON_SLOT].item)
        slots[WEAPON_SLOT].item->comp<Effect>().clear();
        // slots[WEAPON_SLOT].item->removeComp<Effect>();
    
    if(slots[LEFT_FOOT_SLOT].item)
        slots[LEFT_FOOT_SLOT].item->comp<Effect>().clear();
        // slots[LEFT_FOOT_SLOT].item->removeComp<Effect>();
};

std::function<void (void *)> AnimBlueprint::weaponStunExit = [](void * usr){
    Entity *e = (Entity*)usr;
    e->comp<ActionState>().stun = false;
};

std::function<void (void *)> AnimBlueprint::weaponGuardEnter = [](void * usr){
    Entity *e = (Entity*)usr;
    auto &s = e->comp<ActionState>();
    s.blocking = true;
    s.lockType = ActionState::LockedDeplacement::SPEED_ONLY;
    s.lockedMaxSpeed = 0.1;
};

std::function<void (void *)> AnimBlueprint::weaponGuardExit = [](void * usr){
    Entity *e = (Entity*)usr;
    auto &s = e->comp<ActionState>();
    s.blocking = s.hasBlockedAttack;
    s.lockType = ActionState::LockedDeplacement::NONE;
};

std::function<void (void *)> AnimBlueprint::weaponBlockExit = [](void * usr){
    Entity *e = (Entity*)usr;
    auto &s = e->comp<ActionState>();
    s.hasBlockedAttack = false;
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
        a->onExitAnimation = AnimBlueprint::weaponAttackExit;\
        a->speedCallback = ANIMATION_CALLBACK(return AnimBlueprint::weaponAttackCallback(prct, e, beg, end, dmgMult, maxTrigger, ActionState::LockedDeplacement::lockDep, maxspeed, EquipementSlots::slot););\

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
        WEAPON_CALLBACK(37.5, 70, 1, 1, SPEED_ONLY, 0, LEFT_FOOT_SLOT);
    }
    {   AnimationRef a = Loader<AnimationRef>::get("65_2HSword_GUARD_IMPACT_L");
        a->onExitAnimation = AnimBlueprint::weaponBlockExit;
    }
    {   AnimationRef a = Loader<AnimationRef>::get("65_2HSword_GUARD_IMPACT_R");
        a->onExitAnimation = AnimBlueprint::weaponBlockExit;
    }
}