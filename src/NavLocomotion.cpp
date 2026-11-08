#include "NavLocomotion.hpp"

#include <vector>

#include <SanctiaEntity.hpp>
#include <ECS/ComponentTypeNavigation.hpp>

namespace SanctiaNavigation
{
namespace
{
using VulpineNav::LinkKind;
using VulpineNav::NavServer::LinkAgentsAhead;

constexpr float kClimbAgreeHeight = 0.5f;

enum ClimbPhase : uint8_t { kProbeHere, kAlign, kProbeAligned, kJump, kClimbing };
constexpr uint16_t kProbeFrames = 10;
constexpr uint16_t kAlignFrames = 45;
constexpr float kAlignDistance = 0.1f;
constexpr float kAlignSpeedScale = 0.3f;
constexpr uint16_t kJumpFrames = 40;
constexpr uint16_t kClimbAcceptFrames = 20;

// local avoidance
constexpr float kAvoidRange = 3.0f;
constexpr float kPredictHorizon = 1.0f;
constexpr float kPredictWeight = 1.0f;
constexpr float kFollowCone = 0.8f;
constexpr float kFollowGap = 0.8f;
constexpr float kFollowMinScale = 0.25f;

constexpr float kBentDot = 0.87f; // 30 degrees
constexpr uint16_t kStuckFrames = 60;
constexpr uint16_t kUnstickFrames = 90;
constexpr float kProgressEpsilon = 0.05f;
constexpr float kAvoidHeight = 1.5f;
constexpr float kSeparationMargin = 0.1f;
constexpr float kSeparationSoftZone = 0.6f;
constexpr float kSeparationStrength = 1.5f;
constexpr float kCrowdMinSpeedScale = 0.3f;
constexpr float kGoalBodyDistance = 0.3f;
constexpr float kCrowdHoldScale = 0.5f;
constexpr float kCrowdReleaseScale = 0.8f;
constexpr float kCrowdHoldRadius = 4.0f;
constexpr uint16_t kCrowdMinHoldFrames = 20;

constexpr float kSteerSmoothing = 0.3f;

constexpr float kEdgeRoomMin = 0.1f;
constexpr float kEdgeRoomFull = 0.8f;
constexpr float kLinkQueueDistance = 1.2f;

constexpr float kDropSpeedScale = 0.5f;
constexpr float kLandedHeight = 0.6f;
constexpr float kLandedDistance = 2.0f;
constexpr uint16_t kLandedElsewhereFrames = 60;

float xzDistance(vec3 a, vec3 b)
{
    return length(vec2(a.x - b.x, a.z - b.z));
}

vec3 flatDirection(vec3 from, vec3 to)
{
    const vec3 d(to.x - from.x, 0.0f, to.z - from.z);
    const float len = length(d);
    return len > 1e-4f ? d / len : vec3(0);
}

void climbLink(Entity& entity, AgentNavigation& nav, const State3D& state, Movement& mov)
{
    auto& t = nav.detail.traversal;

    if (!entity.has<ComplexMovements>())
    {
        FinishLinkTraversal(nav, false);
        return;
    }
    ComplexMovements& cpx = entity.comp<ComplexMovements>();

    const vec3 across = flatDirection(t.start, t.end);
    const vec3 facing = length(t.facing) > 0.0f ? t.facing : across;
    const uint16_t inPhase = t.frames - t.phaseFrame;

    auto enter = [&](ClimbPhase phase)
    {
        t.phase = phase;
        t.phaseFrame = t.frames;
    };

    // the probe looks along look.current
    auto face = [&](vec3 dir)
    {
        if (length(dir) > 0.0f)
        {
            mov.look.goal = dir;
            mov.look.current = dir;
        }
    };

    auto agrees = [&](vec3 dir)
    {
        const vec3 toTop = cpx.closestSurface - state.position;
        return glm::abs(cpx.closestSurface.y - t.end.y) < kClimbAgreeHeight
            && dot(vec2(toTop.x, toTop.z), vec2(dir.x, dir.z)) > 0.0f;
    };

    auto climbIfAgrees = [&](vec3 dir)
    {
        if (cpx.climb.getCurrent() || !agrees(dir))
            return false;
        cpx.climb.setGoal(true);
        enter(kClimbing);
        return true;
    };

    mov.speed.goal = 0;

    switch ((ClimbPhase)t.phase)
    {
    case kProbeHere:
        face(across);
        if (!climbIfAgrees(across) && inPhase > kProbeFrames)
            enter(kAlign);
        break;

    case kAlign:
    {
        face(facing);
        if (climbIfAgrees(facing))
            break;
        const vec3 toStart = flatDirection(state.position, t.start);
        if (xzDistance(state.position, t.start) < kAlignDistance || length(toStart) == 0.0f || inPhase > kAlignFrames)
        {
            enter(kProbeAligned);
            break;
        }
        mov.direction.goal = toStart;
        mov.speed.goal = nav.moveSpeed * kAlignSpeedScale;
        break;
    }

    case kProbeAligned:
        face(facing);
        if (climbIfAgrees(facing) || inPhase <= kProbeFrames)
            break;
        if (mov.grounded.get())
        {
            // try jumping
            cpx.jump.setGoal(true);
            enter(kJump);
        }
        else
            FinishLinkTraversal(nav, false);
        break;

    case kJump:
        face(facing);
        if (!climbIfAgrees(facing) && inPhase > kJumpFrames)
            FinishLinkTraversal(nav, false);
        break;

    case kClimbing:
        if (cpx.climb.getCurrent())
            t.climbSeen = true;
        else if (t.climbSeen)
            FinishLinkTraversal(nav, glm::abs(state.position.y - t.end.y) < kClimbAgreeHeight);
        else if (inPhase > kClimbAcceptFrames)
            FinishLinkTraversal(nav, false); // never accepted
        break;
    }
}

void dropLink(AgentNavigation& nav, const State3D& state, Movement& mov)
{
    auto& t = nav.detail.traversal;

    const vec3 dir = flatDirection(state.position, t.end);
    if (length(dir) > 0.0f)
    {
        mov.direction.goal = dir;
        mov.look.goal = dir;
        mov.look.current = dir;
    }
    mov.speed.goal = nav.moveSpeed * kDropSpeedScale;

    const bool down = mov.grounded.get() && state.position.y < t.start.y - nav.profile.maxClimb;
    if (!down)
    {
        t.landedFrames = 0;
        return;
    }

    if (glm::abs(state.position.y - t.end.y) < kLandedHeight && xzDistance(state.position, t.end) < kLandedDistance)
    {
        FinishLinkTraversal(nav, true);
        return;
    }

    if (++t.landedFrames > kLandedElsewhereFrames)
        FinishLinkTraversal(nav, true);
}

// might get expensive if we have a lot of agents :/
struct Neighbour
{
    vec3 pos;
    vec2 vel;
    float radius;
};
std::vector<Neighbour> neighbours;

// bends a horizontal unit direction away from nearby agents, returns the speed scale to apply.
// `following` gets the part of it that comes from slower agents ahead of us
float avoid(const AgentNavigation& nav, vec3 pos, vec3& dir, float speed, float& following)
{
    const vec2 desired(dir.x, dir.z);
    const vec2 right(desired.y, -desired.x);
    const vec2 vel = desired * speed;
    vec2 push(0);
    float follow = 1.0f;

    for (const Neighbour& o : neighbours)
    {
        const vec2 d(pos.x - o.pos.x, pos.z - o.pos.z);
        const float dist = length(d);
        if (dist < 1e-4f || dist > kAvoidRange || glm::abs(pos.y - o.pos.y) > kAvoidHeight)
            continue; // ourselves or too far
        if (xzDistance(o.pos, nav.goal) < kGoalBodyDistance)
            continue;

        const float minDist = nav.profile.radius + o.radius + kSeparationMargin;
        const float w = glm::clamp((minDist + kSeparationSoftZone - dist) / kSeparationSoftZone, 0.0f, 1.0f);

        const vec2 relVel = vel - o.vel;
        const float relSpeed2 = dot(relVel, relVel);
        const bool sameWay = dot(o.vel, desired) > 0.5f * length(o.vel) && length(o.vel) > 0.1f;
        if (relSpeed2 > 1e-4f && !sameWay)
        {
            const float t = glm::clamp(-dot(d, relVel) / relSpeed2, 0.0f, kPredictHorizon);
            const vec2 closest = d + relVel * t;
            const float closestDist = length(closest);
            if (t > 0.0f && closestDist < minDist)
            {
                const vec2 aside = closestDist > 1e-3f ? closest / closestDist : right;
                push += aside * (1.0f - t / kPredictHorizon) * kPredictWeight;
            }
        }

        // behind someone slower going our way => match them
        const vec2 toOther = -d / dist;
        const float gap = dist - (nav.profile.radius + o.radius);
        if (dot(toOther, desired) > kFollowCone && gap < kFollowGap && speed > 1e-3f)
        {
            const float theirs = glm::max(dot(o.vel, desired), 0.0f) / speed;
            follow = glm::min(follow, glm::max(glm::max(theirs, gap / kFollowGap), kFollowMinScale));
        }

        if (w <= 0.0f)
            continue;

        vec2 away = d / dist;
        if (dot(away, desired) < -0.7f)
            away += right;
        push += away * w;
    }

    following = follow;
    if (length(push) < 1e-4f)
        return follow;

    const float sideways = dot(push, right);
    const float room = sideways > 0.0f ? nav.detail.pathRoom.y : nav.detail.pathRoom.x;
    push -= right * sideways * (1.0f - smoothstep(kEdgeRoomMin, kEdgeRoomFull, room));

    vec2 out = desired + push * kSeparationStrength;
    const float len = length(out);
    if (len < 1e-4f)
        return glm::min(follow, kCrowdMinSpeedScale);
    out /= len;


    dir = vec3(out.x, 0.0f, out.y);
    return glm::min(follow, glm::clamp(dot(out, desired), kCrowdMinSpeedScale, 1.0f));
}

// remaining distance along the corners we can see, which shrinks as long as we make progress
float pathAhead(const AgentNavigation& nav, vec3 pos)
{
    const auto& d = nav.detail;
    float total = 0.0f;
    vec3 prev = pos;
    for (int i = 0; i < d.cornerCount; i++)
    {
        total += xzDistance(prev, d.corners[i].pos);
        prev = d.corners[i].pos;
    }
    return total;
}

// true while avoidance's sideways push should be dropped because it kept us from making progress
bool unsticking(AgentNavigation& nav, vec3 pos)
{
    auto& u = nav.detail.unstick;
    if (u.framesLeft > 0)
    {
        u.framesLeft--;
        return true;
    }

    const float ahead = pathAhead(nav, pos);
    if (!u.bent || ahead < u.best - kProgressEpsilon || ahead > u.best + 1.0f)
    {
        u.best = ahead;
        u.stuckFrames = 0;
        return false;
    }
    if (++u.stuckFrames < kStuckFrames)
        return false;

    u.stuckFrames = 0;
    u.best = ahead;
    u.framesLeft = kUnstickFrames;
    return true;
}

// close to the next link while someone ahead of us on it is still using it
bool waitForLink(const AgentNavigation& nav)
{
    const auto& d = nav.detail;
    return d.reservedLink && d.reservedDistance < kLinkQueueDistance
        && LinkAgentsAhead(nav.profile, d.reservation, d.reservedLink, d.reservedDistance) > 0;
}
} // namespace

void UpdateNavLocomotion()
{
    using VulpineNav::NavServer::PathStatus;

    neighbours.clear();
    System<AgentNavigation, State3D, Movement>([](Entity&, AgentNavigation& nav, State3D& state, Movement& mov)
    {
        const vec3 v = mov.direction.goal * mov.speed.goal;
        neighbours.push_back({state.position, vec2(v.x, v.z), nav.profile.radius});
    });

    System<AgentNavigation, State3D, Movement>([](Entity& entity, AgentNavigation& nav, State3D& state, Movement& mov)
    {
        if (!nav.hasGoal)
            return;

        auto& d = nav.detail;

        if (d.traversal.active)
        {
            if (d.traversal.kind == LinkKind::Drop || d.traversal.kind == LinkKind::Slide)
                dropLink(nav, state, mov);
            else
                climbLink(entity, nav, state, mov);
            return;
        }

        const vec3 toGoalDir = flatDirection(state.position, nav.goal);

        // look also aims the climb probe
        auto face = [&](vec3 dir)
        {
            mov.look.goal = dir;
            mov.look.current = dir;
        };

        if (d.arrived)
        {
            if (length(toGoalDir) > 0.0f)
                face(toGoalDir);
            mov.speed.goal = 0;
            return;
        }

        if (length(d.moveDirection) > 0.0f)
        {
            if (waitForLink(nav))
            {
                mov.speed.goal = 0;
                return;
            }

            vec3 dir(d.moveDirection.x, 0.0f, d.moveDirection.y);
            const vec3 desired = dir;
            float following = 1.0f;
            float crowd = avoid(nav, state.position, dir, nav.moveSpeed * d.speedScale, following);
            if (unsticking(nav, state.position))
            {
                dir = desired;
                crowd = following;
            }

            auto& st = d.steering;
            const float stopDistance = nav.arriveTolerance + nav.goalRadius;
            const bool nearGoal = xzDistance(state.position, nav.goal) < stopDistance + kCrowdHoldRadius
                               && glm::abs(state.position.y - nav.goal.y) < nav.profile.height * 0.5f;
            if (st.held)
            {
                st.heldFrames++;
                st.held = nearGoal && (crowd < kCrowdReleaseScale || st.heldFrames < kCrowdMinHoldFrames);
            }
            else if (nearGoal && crowd < kCrowdHoldScale)
            {
                st.held = true;
                st.heldFrames = 0;
            }
            if (st.held)
            {
                if (length(toGoalDir) > 0.0f)
                    face(toGoalDir);
                mov.speed.goal = 0;
                st.dir = vec2(toGoalDir.x, toGoalDir.z);
                return;
            }

            // smoothing lags behind turns, which on a narrow path carries us over the edge
            const vec2 wanted(dir.x, dir.z);
            const float smoothing = mix(kSteerSmoothing, 1.0f, PathNarrowness(nav));
            const vec2 blended = length(st.dir) > 0.0f ? mix(st.dir, wanted, smoothing) : wanted;
            st.dir = length(blended) > 1e-3f ? normalize(blended) : wanted;
            dir = vec3(st.dir.x, 0.0f, st.dir.y);
            d.unstick.bent = dot(dir, desired) < kBentDot;

            mov.direction.goal = dir;
            face(dir);
            mov.speed.goal = nav.moveSpeed * d.speedScale * crowd;
        }
        else
        {
            mov.speed.goal = 0;
            if (d.status == PathStatus::Partial && length(toGoalDir) > 0.0f)
                face(toGoalDir);
        }
    });
}
} // namespace SanctiaNavigation
