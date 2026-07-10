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
    return e and e->has<Deplacement>();
};

ANIMATION_SWITCH_ENTITY(switchIdle, 
    if(!isEntityViable(e)) return false;
    float speed = e->comp<Deplacement>().speed.current;
    return speed <= 1e-3f;
)


ANIMATION_SWITCH_ENTITY(switchWalk, 
    if(!isEntityViable(e)) return false;
    // float speed = e->comp<Deplacement>().speed.goal * length(e->comp<Deplacement>().direction.goal);
    float speed = e->comp<Deplacement>().speed.current;
    auto &dep = e->comp<Deplacement>();
    return speed > 0.01f and speed <= mix(dep.walkSpeed, dep.jogSpeed, 0.6f);
)

ANIMATION_SWITCH_ENTITY(switchJog, 
    if(!isEntityViable(e)) return false;
    // float speed = e->comp<Deplacement>().speed.goal * length(e->comp<Deplacement>().direction.goal);
    float speed = e->comp<Deplacement>().speed.current;
    auto &dep = e->comp<Deplacement>();
    return speed > mix(dep.walkSpeed, dep.jogSpeed, 0.6f) and speed <= e->comp<Deplacement>().sprintSpeed*0.9f;
)

ANIMATION_SWITCH_ENTITY(switchRun, 
    if(!isEntityViable(e)) return false;
    // float speed = e->comp<Deplacement>().speed.goal * length(e->comp<Deplacement>().direction.goal);
    float speed = e->comp<Deplacement>().speed.current;
    return speed >= e->comp<Deplacement>().sprintSpeed*0.9f;
)


AnimationControllerRef AnimBlueprint::Human::ParkourMoveset(const std::string & prefix, Entity *e)
{
    LOAD_ANIM_FROM_PREFIX(Idle)
    LOAD_ANIM_FROM_PREFIX(Walk)
    LOAD_ANIM_FROM_PREFIX(Jog)
    LOAD_ANIM_FROM_PREFIX(Run)

    float deplacementTT = 0.25;

    auto walkCallback = [](float f, void *usr)
    {
        if(!usr)
        {
            // WARNING_MESSAGE("Empty USR for walking animation !")
            return 1.f;
        }
        Entity *e = (Entity*)usr;
        auto &dep = e->comp<Deplacement>();

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
        auto &dep = e->comp<Deplacement>();

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
        auto &dep = e->comp<Deplacement>();

        float s = sign(dot(dep.direction.current, dep.look.current) + 0.5f);

        return s*dep.speed.current/dep.sprintSpeed;
    };
    Run->speedCallback = RunCallback;
    Run->repeat = true;

    std::vector<AnimationControllerTransition> graph
    {
        /* FROM IDLE */
        ACT(Idle, Walk, deplacementTT, switchWalk),
        ACT(Idle, Jog, deplacementTT, switchJog),
        ACT(Idle, Run, deplacementTT, switchRun),

        /* FROM WALK */
        ACT(Walk, Idle, deplacementTT, switchIdle),
        ACT(Walk, Jog, deplacementTT, switchJog),
        ACT(Walk, Run, deplacementTT, switchRun),

        /* FROM JOG */
        ACT(Jog, Idle, deplacementTT, switchIdle),
        ACT(Jog, Walk, deplacementTT, switchWalk),
        ACT(Jog, Run, deplacementTT, switchRun),

        /* FROM RUN */
        ACT(Run, Idle, deplacementTT, switchIdle),
        ACT(Run, Walk, deplacementTT, switchWalk),
        ACT(Run, Jog, deplacementTT, switchJog),
    };

    return AnimationControllerRef(new AnimationController(graph, Idle, e));
};