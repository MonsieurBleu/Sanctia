#include "NavLocomotion.hpp"

#include <SanctiaEntity.hpp>
#include <ECS/ComponentTypeNavigation.hpp>
namespace SanctiaNavigation
{
void UpdateNavLocomotion()
{
    using VulpineNav::NavServer::PathStatus;

    System<AgentNavigation, State3D, Movement>([](Entity& entity, AgentNavigation& nav, State3D& state, Movement& mov)
    {
        if (!nav.hasGoal)
            return;

        const auto& d = nav.detail;

        const vec3 toGoal(nav.goal.x - state.position.x, 0.0f, nav.goal.z - state.position.z);
        const float toGoalLen = length(toGoal);
        const vec3 toGoalDir = toGoalLen > 1e-4f ? toGoal / toGoalLen : vec3(0);

        // look also aims the climb probe
        auto face = [&](vec3 dir)
        {
            mov.look.goal = dir;
            mov.look.current = dir;
        };

        if (d.arrived)
        {
            if (toGoalLen > 1e-4f)
                face(toGoalDir);
            mov.speed.goal = 0;
            return;
        }

        if (length(d.moveDirection) > 0.0f)
        {
            const vec3 dir(d.moveDirection.x, 0.0f, d.moveDirection.y);
            mov.direction.goal = dir;
            face(dir);
            mov.speed.goal = nav.moveSpeed * d.speedScale;
        }
        else
        {
            mov.speed.goal = 0;

            // the end of a partial path is usually the foot of whatever is in the way
            if (d.status == PathStatus::Partial && toGoalLen > 1e-4f)
                face(toGoalDir);
        }

        // TEMP until climb links: climb whatever the probe finds every 3s
        if (entity.has<ComplexMovements>())
        {
            ComplexMovements& cpx = entity.comp<ComplexMovements>();
            if (!cpx.climb.getCurrent()
                && cpx.climb.timeSinceGoalMet() > 3.0
                && cpx.closestSurface.y >= state.position.y + 0.5f)
            {
                cpx.climb.setGoal(true);
            }
        }
    });
}
} // namespace SanctiaNavigation