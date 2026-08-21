#include <AnimationBlueprint2.hpp>
#include <MathsUtils.hpp>
#include <Constants.hpp>

#define INV_ANIMATION_SWITCH(func) \
    [](void * usr) {return !func(usr);}

#define AND_ANIMATION_SWITCH(func1, func2) \
    [](void * usr) {return func1(usr) and func2(usr);}

#define OR_ANIMATION_SWITCH \
    [](void * usr) {return func1(usr) or func2(usr);}

#define ANIMATION_SWITCH_ENTITY(name, code) \
    auto name = [](void * usr){\
    Entity *e = (Entity*)usr; \
    code \
    };

#define LOAD_ANIM_FROM_PREFIX(name) AnimationRef name = Loader<AnimationRef>::get(prefix + #name);

#define ACT(animA, animB, time, cond) AnimationControllerTransition(animA, animB, COND_CUSTOM, time, TRANSITION_SMOOTH, cond)

bool isEntityViable(Entity *e)
{
    return e and e->has<Movement>();
};

ANIMATION_SWITCH_ENTITY(switchIdle, 
    if(!isEntityViable(e)) return false;
    float speed = e->comp<Movement>().speed.goal * length(e->comp<Movement>().direction.goal);
    return speed <= 1e-3f;
)


ANIMATION_SWITCH_ENTITY(switchWalk, 
    if(!isEntityViable(e)) return false;
    float speed = e->comp<Movement>().speed.goal * length(e->comp<Movement>().direction.goal);
    // float speed = e->comp<Movement>().speed.current;
    auto &dep = e->comp<Movement>();
    return speed > 0.1f and speed <= mix(dep.walkSpeed, dep.jogSpeed, 0.6f);
)

ANIMATION_SWITCH_ENTITY(switchJog, 
    if(!isEntityViable(e)) return false;
    float speed = e->comp<Movement>().speed.goal * length(e->comp<Movement>().direction.goal);
    // float speed = e->comp<Movement>().speed.current;
    auto &dep = e->comp<Movement>();
    return speed > mix(dep.walkSpeed, dep.jogSpeed, 0.6f) and speed <= e->comp<Movement>().sprintSpeed*0.9f;
)

ANIMATION_SWITCH_ENTITY(switchRun, 
    if(!isEntityViable(e)) return false;
    float speed = e->comp<Movement>().speed.goal * length(e->comp<Movement>().direction.goal);
    // float speed = e->comp<Movement>().speed.current;
    return speed >= e->comp<Movement>().sprintSpeed*0.9f;
)

ANIMATION_SWITCH_ENTITY(switchClimb, 
    if(!isEntityViable(e)) return false;
    return e->comp<ComplexMovements>().climb.getCurrent();
)

ANIMATION_SWITCH_ENTITY(switchClimbHigh, 
    if(!isEntityViable(e)) return false;
    return e->comp<ComplexMovements>().climbType == ComplexMovements::ClimbTypeEnum::High;
)

ANIMATION_SWITCH_ENTITY(switchClimbLow, 
    if(!isEntityViable(e)) return false;
    return e->comp<ComplexMovements>().climbType == ComplexMovements::ClimbTypeEnum::Low;
)

ANIMATION_SWITCH_ENTITY(switchClimbPlatform, 
    if(!isEntityViable(e)) return false;
    return e->comp<ComplexMovements>().climbType == ComplexMovements::ClimbTypeEnum::Platform;
)

ANIMATION_SWITCH_ENTITY(switchJump, 
    if(!isEntityViable(e)) return false;
    return e->comp<ComplexMovements>().jump.getCurrent() and e->comp<ComplexMovements>().jump.timeSinceGoalMet() < 0.1;
)

ANIMATION_SWITCH_ENTITY(switchJumpEnd, 
    if(!isEntityViable(e)) return false;

    // WARNING_MESSAGE(
    //     PRINTVAR(e->comp<Movement>().grounded.get()),
    //     PRINTVAR(e->comp<ComplexMovements>().jump.timeSinceGoalMet()),
    //     PRINTVAR(e->comp<ComplexMovements>().jump.timeSinceGoalChange())
    // )

    return e->comp<Movement>().grounded.get() and e->comp<ComplexMovements>().jump.timeSinceGoalMet() > 0.25;
)


ANIMATION_SWITCH_ENTITY(switchFalling, 
    if(!isEntityViable(e)) return false;
    return !e->comp<Movement>().grounded.get() and e->comp<Movement>().grounded.timeSinceChange() > 0.1;
)

ANIMATION_SWITCH_ENTITY(switchLand, 
    if(!isEntityViable(e)) return false;
    return e->comp<Movement>().grounded.get() and e->comp<Movement>().grounded.previousTimeSinceChange() > 0.75;
)

ANIMATION_SWITCH_ENTITY(switchLandEnd, 
    if(!isEntityViable(e)) return false;
    // return e->comp<Movement>().grounded.get() and e->comp<Movement>().grounded.timeSinceChange() > 1.0;
    return e->comp<Movement>().grounded.get() and e->comp<Movement>().grounded.timeSinceChange() > 0.20;
)

AnimationControllerRef AnimBlueprint::Human::ParkourMoveset(const std::string & prefix, Entity *e)
{
    LOAD_ANIM_FROM_PREFIX(Idle)
    LOAD_ANIM_FROM_PREFIX(Walk)
    LOAD_ANIM_FROM_PREFIX(Jog)
    LOAD_ANIM_FROM_PREFIX(Run)
    LOAD_ANIM_FROM_PREFIX(Climb)
    LOAD_ANIM_FROM_PREFIX(ClimbLow)
    LOAD_ANIM_FROM_PREFIX(ClimbPlatform)

    LOAD_ANIM_FROM_PREFIX(Jump)
    LOAD_ANIM_FROM_PREFIX(JumpF)
    LOAD_ANIM_FROM_PREFIX(Falling)
    LOAD_ANIM_FROM_PREFIX(Land)

    float deplacementTT = 0.25;
    float stuntTT = 0.25;
    float jumpTT = 0.125;

    auto walkCallback = [](float f, void *usr)
    {
        if(!usr)
        {
            // WARNING_MESSAGE("Empty USR for walking animation !")
            return 1.f;
        }
        Entity *e = (Entity*)usr;
        auto &dep = e->comp<Movement>();

        float s = sign(dot(dep.direction.current, dep.look.current) + 0.5f);

        return s * max(0.25f, 
                linearstep(0.f, dep.walkSpeed, dep.speed.current) + 
                0.75f*linearstep(dep.walkSpeed, dep.jogSpeed, dep.speed.current)
        );
    };

    Walk->speedCallback = walkCallback;
    Walk->repeat = true;

    auto JogCallback = [](float f, void *usr)
    {
        if(!usr)
        {
            // WARNING_MESSAGE("Empty USR for walking animation !")
            return 1.f;
        }
        Entity *e = (Entity*)usr;
        auto &dep = e->comp<Movement>();

        float s = sign(dot(dep.direction.current, dep.look.current) + 0.5f);

        return 
            s*0.75f + s*0.5f*
            linearstep(
                mix(dep.walkSpeed, dep.jogSpeed, 0.6f), 
                dep.jogSpeed, 
                dep.speed.current
            )
        ;
    };
    Jog->speedCallback = JogCallback;
    Jog->repeat = true;


    auto RunCallback = [](float f, void *usr)
    {
        if(!usr)
        {
            // WARNING_MESSAGE("Empty USR for walking animation !")
            return 1.f;
        }
        Entity *e = (Entity*)usr;
        auto &dep = e->comp<Movement>();

        float s = sign(dot(dep.direction.current, dep.look.current) + 0.5f);

        return s*dep.speed.current/dep.sprintSpeed;
    };
    Run->speedCallback = RunCallback;
    Run->repeat = true;

    {
        float l = Climb->getLength();
        Climb->speedCallback = [l](float f, void *usr)
        {
            Entity *e = (Entity*)usr;
            if(!e or !e->has<ComplexMovements>()) return l/2.5f;
            return e->comp<ComplexMovements>().climbAnimSpeed * l/2.5f;
        };
        Climb->repeat = false;
    }

    {
        float l = ClimbLow->getLength();
        ClimbLow->speedCallback = [l](float f, void *usr)
        {
            Entity *e = (Entity*)usr;
            if(!e or !e->has<ComplexMovements>()) return l/1.f;
            return e->comp<ComplexMovements>().climbAnimSpeed * l/1.f;
        };
        ClimbLow->repeat = false;
    }

    {
        float l = ClimbPlatform->getLength();
        ClimbPlatform->speedCallback = [l](float f, void *usr)
        {
            Entity *e = (Entity*)usr;
            if(!e or !e->has<ComplexMovements>()) return l/2.5f;
            return e->comp<ComplexMovements>().climbAnimSpeed * l/2.5f;
        };
        ClimbPlatform->repeat = false;
    }
    
    {
        float l = Jump->getLength();
        Jump->speedCallback = [l](float f, void *usr)
        {
            Entity *e = (Entity*)usr;
            return l/0.6f;
        };
        Jump->repeat = false;
    }
    
    {
        float l = JumpF->getLength();
        JumpF->speedCallback = [l](float f, void *usr)
        {
            Entity *e = (Entity*)usr;
            return l/0.6f;
        };
        JumpF->repeat = false;
    }

    {
        float l = Land->getLength();
        Land->speedCallback = [l](float f, void *usr)
        {
            Entity *e = (Entity*)usr;
            return l/1.0f;
        };
        Land->repeat = false;
    }
    


    std::vector<AnimationControllerTransition> graph
    {
        /* FROM IDLE */
        ACT(Idle, Walk, deplacementTT, switchWalk),
        ACT(Idle, Jog, deplacementTT, switchJog),
        ACT(Idle, Run, deplacementTT, switchRun),
        ACT(Idle, Climb, stuntTT, AND_ANIMATION_SWITCH(switchClimb, switchClimbHigh)),
        ACT(Idle, ClimbLow, stuntTT, AND_ANIMATION_SWITCH(switchClimb, switchClimbLow)),
        ACT(Idle, ClimbPlatform, stuntTT, AND_ANIMATION_SWITCH(switchClimb, switchClimbPlatform)),
        ACT(Idle, Jump, jumpTT, switchJump),
        ACT(Idle, Falling, stuntTT, switchFalling),

        /* FROM WALK */
        ACT(Walk, Idle, deplacementTT, switchIdle),
        ACT(Walk, Jog, deplacementTT, switchJog),
        ACT(Walk, Run, deplacementTT, switchRun),
        ACT(Walk, Climb, stuntTT, AND_ANIMATION_SWITCH(switchClimb, switchClimbHigh)),
        ACT(Walk, ClimbLow, stuntTT, AND_ANIMATION_SWITCH(switchClimb, switchClimbLow)),
        ACT(Walk, ClimbPlatform, stuntTT, AND_ANIMATION_SWITCH(switchClimb, switchClimbPlatform)),
        ACT(Walk, Jump, jumpTT, switchJump),
        ACT(Walk, Falling, stuntTT, switchFalling),

        /* FROM JOG */
        ACT(Jog, Idle, deplacementTT, switchIdle),
        ACT(Jog, Walk, deplacementTT, switchWalk),
        ACT(Jog, Run, deplacementTT, switchRun),
        ACT(Jog, Climb, stuntTT, AND_ANIMATION_SWITCH(switchClimb, switchClimbHigh)),
        ACT(Jog, ClimbLow, stuntTT, AND_ANIMATION_SWITCH(switchClimb, switchClimbLow)),
        ACT(Jog, ClimbPlatform, stuntTT, AND_ANIMATION_SWITCH(switchClimb, switchClimbPlatform)),
        ACT(Jog, JumpF, jumpTT, switchJump),
        ACT(Jog, Falling, stuntTT, switchFalling),

        /* FROM RUN */
        ACT(Run, Idle, deplacementTT, switchIdle),
        ACT(Run, Walk, deplacementTT, switchWalk),
        ACT(Run, Jog, deplacementTT, switchJog),
        ACT(Run, Climb, stuntTT, AND_ANIMATION_SWITCH(switchClimb, switchClimbHigh)),
        ACT(Run, ClimbLow, stuntTT, AND_ANIMATION_SWITCH(switchClimb, switchClimbLow)),
        ACT(Run, ClimbPlatform, stuntTT, AND_ANIMATION_SWITCH(switchClimb, switchClimbPlatform)),
        ACT(Run, JumpF, jumpTT, switchJump),
        ACT(Run, Falling, stuntTT, switchFalling),

        /* FROM CLIMB */
        ACT(Climb, Idle, stuntTT, AND_ANIMATION_SWITCH(switchIdle, INV_ANIMATION_SWITCH(switchClimb))),
        ACT(Climb, Walk, stuntTT, AND_ANIMATION_SWITCH(switchWalk, INV_ANIMATION_SWITCH(switchClimb))),
        ACT(Climb, Jog, stuntTT,  AND_ANIMATION_SWITCH(switchJog,  INV_ANIMATION_SWITCH(switchClimb))),
        ACT(Climb, Run, stuntTT,  AND_ANIMATION_SWITCH(switchRun,  INV_ANIMATION_SWITCH(switchClimb))),

        /* FROM CLIMB LOW */
        ACT(ClimbLow, Idle, stuntTT, AND_ANIMATION_SWITCH(switchIdle, INV_ANIMATION_SWITCH(switchClimb))),
        ACT(ClimbLow, Walk, stuntTT, AND_ANIMATION_SWITCH(switchWalk, INV_ANIMATION_SWITCH(switchClimb))),
        ACT(ClimbLow, Jog,  stuntTT,  AND_ANIMATION_SWITCH(switchJog,  INV_ANIMATION_SWITCH(switchClimb))),
        ACT(ClimbLow, Run,  stuntTT,  AND_ANIMATION_SWITCH(switchRun,  INV_ANIMATION_SWITCH(switchClimb))),

        /* FROM CLIMB PLATFORM */
        ACT(ClimbPlatform, Idle, stuntTT, AND_ANIMATION_SWITCH(switchIdle, INV_ANIMATION_SWITCH(switchClimb))),
        ACT(ClimbPlatform, Walk, stuntTT, AND_ANIMATION_SWITCH(switchWalk, INV_ANIMATION_SWITCH(switchClimb))),
        ACT(ClimbPlatform, Jog, stuntTT,  AND_ANIMATION_SWITCH(switchJog,  INV_ANIMATION_SWITCH(switchClimb))),
        ACT(ClimbPlatform, Run, stuntTT,  AND_ANIMATION_SWITCH(switchRun,  INV_ANIMATION_SWITCH(switchClimb))),


        /* FROM JUMP */
        ACT(Jump, Land, stuntTT, AND_ANIMATION_SWITCH(switchLand, switchJumpEnd)),
        ACT(Jump, Idle, stuntTT, AND_ANIMATION_SWITCH(switchIdle, switchJumpEnd)),
        ACT(Jump, Run,  stuntTT, AND_ANIMATION_SWITCH(switchRun,  switchJumpEnd)),
        ACT(Jump, Jog,  stuntTT, AND_ANIMATION_SWITCH(switchJog,  switchJumpEnd)),
        ACT(Jump, Walk, stuntTT, AND_ANIMATION_SWITCH(switchWalk, switchJumpEnd)),
        ACT(Jump, Climb, stuntTT, AND_ANIMATION_SWITCH(switchClimb, switchClimbHigh)),
        ACT(Jump, ClimbLow, stuntTT, AND_ANIMATION_SWITCH(switchClimb, switchClimbLow)),
        ACT(Jump, ClimbPlatform, stuntTT, AND_ANIMATION_SWITCH(switchClimb, switchClimbPlatform)),
        AnimationControllerTransition(Jump, Falling, COND_ANIMATION_FINISHED, stuntTT),

        /* FROM JUMP FORWARD */
        ACT(JumpF, Land, stuntTT, AND_ANIMATION_SWITCH(switchLand, switchJumpEnd)),
        ACT(JumpF, Idle, stuntTT, AND_ANIMATION_SWITCH(switchIdle, switchJumpEnd)),
        ACT(JumpF, Run,  stuntTT, AND_ANIMATION_SWITCH(switchRun,  switchJumpEnd)),
        ACT(JumpF, Jog,  stuntTT, AND_ANIMATION_SWITCH(switchJog,  switchJumpEnd)),
        ACT(JumpF, Walk, stuntTT, AND_ANIMATION_SWITCH(switchWalk, switchJumpEnd)),
        ACT(JumpF, Climb, stuntTT, AND_ANIMATION_SWITCH(switchClimb, switchClimbHigh)),
        ACT(JumpF, ClimbLow, stuntTT, AND_ANIMATION_SWITCH(switchClimb, switchClimbLow)),
        ACT(JumpF, ClimbPlatform, stuntTT, AND_ANIMATION_SWITCH(switchClimb, switchClimbPlatform)),
        AnimationControllerTransition(JumpF, Falling, COND_ANIMATION_FINISHED, stuntTT),

        /* FROM FALLING */
        ACT(Falling, Land, stuntTT, AND_ANIMATION_SWITCH(switchLand, INV_ANIMATION_SWITCH(switchFalling))),
        ACT(Falling, Idle, stuntTT, AND_ANIMATION_SWITCH(switchIdle, INV_ANIMATION_SWITCH(switchFalling))),
        ACT(Falling, Run,  stuntTT, AND_ANIMATION_SWITCH(switchRun,  INV_ANIMATION_SWITCH(switchFalling))),
        ACT(Falling, Jog,  stuntTT, AND_ANIMATION_SWITCH(switchJog,  INV_ANIMATION_SWITCH(switchFalling))),
        ACT(Falling, Walk, stuntTT, AND_ANIMATION_SWITCH(switchWalk, INV_ANIMATION_SWITCH(switchFalling))),
        ACT(Falling, Climb, stuntTT, AND_ANIMATION_SWITCH(switchClimb, switchClimbHigh)),
        ACT(Falling, ClimbLow, stuntTT, AND_ANIMATION_SWITCH(switchClimb, switchClimbLow)),
        ACT(Falling, ClimbPlatform, stuntTT, AND_ANIMATION_SWITCH(switchClimb, switchClimbPlatform)),
        // ACT(Falling, Jump, stuntTT, AND_ANIMATION_SWITCH(switchJump, INV_ANIMATION_SWITCH(switchFalling))),

        /* FROM LAND*/
        ACT(Land, Idle, stuntTT, AND_ANIMATION_SWITCH(switchIdle, switchLandEnd)),
        ACT(Land, Run,  stuntTT, AND_ANIMATION_SWITCH(switchRun,  switchLandEnd)),
        ACT(Land, Jog,  stuntTT, AND_ANIMATION_SWITCH(switchJog,  switchLandEnd)),
        ACT(Land, Walk, stuntTT, AND_ANIMATION_SWITCH(switchWalk, switchLandEnd)),
        ACT(Land, Climb, stuntTT, AND_ANIMATION_SWITCH(switchClimb, switchClimbHigh)),
        ACT(Land, ClimbLow, stuntTT, AND_ANIMATION_SWITCH(switchClimb, switchClimbLow)),
        ACT(Land, ClimbPlatform, stuntTT, AND_ANIMATION_SWITCH(switchClimb, switchClimbPlatform)),
        // ACT(Land, Jump, stuntTT, AND_ANIMATION_SWITCH(switchJump, switchLandEnd)),
    };

    return AnimationControllerRef(new AnimationController(graph, Idle, e));
};