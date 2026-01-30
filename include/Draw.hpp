#pragma once

#include <glm/glm.hpp>
#include <list>

#include <SanctiaEntity.hpp>
#include <Graphics/ObjectGroup.hpp>
#include <Graphics/Mesh.hpp>
#include <Utils.hpp>
#include <queue>

using namespace glm;

struct DrawElementBase : public MeshModel3D
{
    float duration = 0;
    float startTime = 0;
    vec3 color = "#ff0000"_rgb;
    bool drawnOnce = false;
    DrawElementBase(
        float startTime, 
        ModelState3D _state,
        float duration = 0.0f, 
        vec3 color = "#ff0000"_rgb) : 
              MeshModel3D(Loader<MeshMaterial>::get("basicHelper"))
            , duration(duration)
            , startTime(startTime)
            , color(color) 
        {
            state = _state;
        }


    virtual void initData() = 0;

    bool isTimeElapsed();
};

typedef std::shared_ptr<DrawElementBase> DrawElementBasePtr;

struct DrawLine : DrawElementBase
{
    vec3 start, end;
    DrawLine(
        vec3 start,
        vec3 end,
        float startTime, 
        float duration = 0.0f, 
        ModelState3D state = ModelState3D(),
        vec3 color = "#ff0000"_rgb) : 
              DrawElementBase(startTime, state, duration, color)
              , start(start)
              , end(end) 
        {}

    void initData() override;
};

struct DrawSphere : DrawElementBase
{
    vec3 center;
    float radius;
    int n_slices, n_stacks;
    DrawSphere(
        vec3 center,
        float radius,
        int n_slices,
        int n_stacks,
        float startTime, 
        float duration = 0.0f, 
        ModelState3D state = ModelState3D(),
        vec3 color = "#ff0000"_rgb) : 
              DrawElementBase(startTime, state, duration, color)
              , center(center)
              , radius(radius)
              , n_slices(n_slices) 
              , n_stacks(n_stacks)
        {}
    void initData() override;
};

struct DrawBox : DrawElementBase
{
    vec3 min, max;
    DrawBox(
    vec3 min,
    vec3 max,
    float startTime, 
    float duration = 0.0f, 
    ModelState3D state = ModelState3D(),
    vec3 color = "#ff0000"_rgb) : 
            DrawElementBase(startTime, state, duration, color)
            , min(min)
            , max(max)
    {}
    void initData() override;
};

class Draw
{
private:
    std::list<EntityRef> drawElements;
    std::queue<DrawElementBasePtr> toDrawQueue;
    std::mutex queueMutex;

    void addElement(DrawElementBasePtr element);

    EntityRef entity = nullptr;

public:
    void update();
    void reset();

    Draw(EntityRef parent) {
        entity = newEntity("Draw Element Group"
        );

        ComponentModularity::addChild(*parent, 
            entity
        );
    }

    Draw() {};

    void drawLine(
        vec3 start, 
        vec3 end, 
        float duration = 0.0f, 
        ModelState3D state = ModelState3D(),
        vec3 color = "#ff0000"_rgb
    );
    
    void drawSphere(
        vec3 center, 
        float radius, 
        float duration = 0.0f, 
        ModelState3D state = ModelState3D(),
        vec3 color = "#ff0000"_rgb, 
        int n_slices = 16, 
        int n_stacks = 24
    );

    void drawBox(
        vec3 min, 
        vec3 max, 
        float duration = 0.0f, 
        ModelState3D state = ModelState3D(),
        vec3 color = "#ff0000"_rgb
    );

    void drawBoxFromHalfExtents(
        vec3 center, 
        vec3 halfExtents, 
        float duration = 0.0f, 
        ModelState3D state = ModelState3D(),
        vec3 color = "#ff0000"_rgb
    );

    void drawBox(
        rp3d::BoxShape* box,
        const rp3d::Transform& transform,
        float duration = 0.0f, 
        ModelState3D state = ModelState3D(),
        vec3 color = "#ff0000"_rgb
    );
};