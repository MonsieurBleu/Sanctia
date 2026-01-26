#include "PlayerUtils.hpp"

#include "Globals.hpp"

quat PlayerViewController::apply(const quat& view)
{
    if (!enabled) return view;

    return slerp(view, targetDir, forceFactor * globals.appTime.getDelta() * 60.0f); 
}

vec3 PlayerViewController::apply(const vec3& view)
{
    if (!enabled) return view;

    quat q = quat(view);
    return eulerAngles(apply(q));
}