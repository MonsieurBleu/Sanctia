#pragma once
#include "glm/glm.hpp"
#define GLM_ENABLE_EXPERIMENTAL
#include "glm/gtx/quaternion.hpp"

using namespace glm;

// maybe a whole camera animation class would work better for what we're trying to do
// also it would be more versatile
class PlayerViewController
{
private:
    static inline quat targetDir = quat(0, 0, 0, 1);
    static inline float forceFactor = 0.1f;
    static inline bool enabled = false;
public:
    static quat apply(const quat& view);
    static vec3 apply(const vec3& view); // cause for some reason the camera and state3d uses a vec3 direction

    static void setTargetDir(const quat& newTargetDir)
    {
        targetDir = newTargetDir;
    }

    static void setTargetDir(const vec3& newTargetDir)
    {
        targetDir = quat(newTargetDir);
    }

    static void setForceFactor(float newForceFactor)
    {
        forceFactor = newForceFactor;
    }

    static void setEnabled(float newEnabled)
    {
        enabled = newEnabled;
    }

    static bool getEnabled()
    {
        return enabled;
    }
};