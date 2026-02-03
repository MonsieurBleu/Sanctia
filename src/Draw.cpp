#include "Draw.hpp"

#include <Constants.hpp>
#include <Globals.hpp>
#include <MathsUtils.hpp>

#include "GameGlobals.hpp"


bool DrawElementBase::isTimeElapsed()
{
    if (!drawnOnce)
    {
        drawnOnce = true;
        return false;
    }
    return globals.appTime.getElapsedTime() - startTime > duration;
}

void DrawLine::initData()
{
    createUniforms();
    uniforms.add(ShaderUniform(&color, 20));

    noBackFaceCulling = true;
    defaultMode = GL_LINES;

    int nbOfPoints = 2;
    GenericSharedBuffer buff(new char[sizeof(vec3)*nbOfPoints]);
    GenericSharedBuffer buffNormal(new char[sizeof(vec3)*nbOfPoints]);

    vec3 *pos = (vec3*)buff.get();
    vec3 *nor = (vec3*)buffNormal.get();

    for(int i = 0; i < nbOfPoints; i++)
    {
        nor[i] = vec3(2);
        pos[i] = vec3(0);
    }

    // Face 1
    pos[0] = start;
    pos[1] = end;


    MeshVao vao(new 
        VertexAttributeGroup({
            VertexAttribute(buff, 0, nbOfPoints, 3, GL_FLOAT, false),
            VertexAttribute(buffNormal, 1, nbOfPoints, 3, GL_FLOAT, false),
            VertexAttribute(buffNormal, 2, nbOfPoints, 3, GL_FLOAT, false)
        })
    );

    setVao(vao);
}

void DrawSphere::initData()
{
    createUniforms();
    uniforms.add(ShaderUniform(&color, 20));


    noBackFaceCulling = true;
    defaultMode = GL_LINES;

    int stepU = n_slices;
    int stepV = n_stacks;
    int nbOfPoints = stepU*stepV*4;
    GenericSharedBuffer buff(new char[sizeof(vec3)*nbOfPoints]);
    GenericSharedBuffer buffNormal(new char[sizeof(vec3)*nbOfPoints]);

    vec3 *pos = (vec3*)buff.get();
    vec3 *nor = (vec3*)buffNormal.get();


    int id = 0;

    // for(int i = 0; i < nbOfPoints; i++)
    for(int j = 0; j < stepV; j++)
    for(int i = 0; i < stepU; i++)
    {
        vec2 uv = vec2(
            PI2*(float)i/(float)stepU,
            PI2*(float)j/(float)stepV 
        );

        vec2 uv2 = vec2(
            PI2*(float)i/(float)stepU,
            PI2*(float)(j+1)/(float)stepV 
        );

        vec2 uv3 = vec2(
            PI2*(float)(i+1)/(float)stepU,
            PI2*(float)j/(float)stepV 
        );  

        pos[id] = PhiThetaToDir(uv)*radius + center;
        nor[id] = vec3(2);
        id++;
        pos[id] = PhiThetaToDir(uv2)*radius + center;
        nor[id] = vec3(2);
        id++;

        pos[id] = PhiThetaToDir(uv)*radius + center;
        nor[id] = vec3(2);
        id++;
        pos[id] = PhiThetaToDir(uv3)*radius + center;
        nor[id] = vec3(2);
        id++;
    }

    MeshVao vao(new 
        VertexAttributeGroup({
            VertexAttribute(buff, 0, nbOfPoints, 3, GL_FLOAT, false),
            VertexAttribute(buffNormal, 1, nbOfPoints, 3, GL_FLOAT, false),
            VertexAttribute(buffNormal, 2, nbOfPoints, 3, GL_FLOAT, false)
        })
    );

    setVao(vao);
}

void DrawBox::initData()
{
    createUniforms();
    uniforms.add(ShaderUniform(&color, 20));

    // state.frustumCulled = false;
    // depthWrite = false;
    noBackFaceCulling = true;
    defaultMode = GL_LINES;

    int nbOfPoints = 48;
    GenericSharedBuffer buff(new char[sizeof(vec3)*nbOfPoints]);
    GenericSharedBuffer buffNormal(new char[sizeof(vec3)*nbOfPoints]);

    vec3 *pos = (vec3*)buff.get();
    vec3 *nor = (vec3*)buffNormal.get();

    for(int i = 0; i < nbOfPoints; i++)
    {
        nor[i] = vec3(2);
        pos[i] = vec3(0);
    }


    // Face 1
    pos[0] = min*vec3(1, 1, 1) + max*(vec3(0, 0, 0));
    pos[1] = min*vec3(1, 0, 1) + max*(vec3(0, 1, 0));

    pos[2] = min*vec3(1, 1, 1) + max*(vec3(0, 0, 0));
    pos[3] = min*vec3(1, 1, 0) + max*(vec3(0, 0, 1));

    pos[4] = min*vec3(1, 0, 0) + max*(vec3(0, 1, 1));
    pos[5] = min*vec3(1, 1, 0) + max*(vec3(0, 0, 1));

    pos[6] = min*vec3(1, 0, 0) + max*(vec3(0, 1, 1));
    pos[7] = min*vec3(1, 0, 1) + max*(vec3(0, 1, 0));

    // Face 2
    pos[8] = min*vec3(0, 1, 1) + max*(vec3(1, 0, 0));
    pos[9] = min*vec3(0, 0, 1) + max*(vec3(1, 1, 0));

    pos[10] = min*vec3(0, 1, 1) + max*(vec3(1, 0, 0));
    pos[11] = min*vec3(0, 1, 0) + max*(vec3(1, 0, 1));

    pos[12] = min*vec3(0, 0, 0) + max*(vec3(1, 1, 1));
    pos[13] = min*vec3(0, 1, 0) + max*(vec3(1, 0, 1));

    pos[14] = min*vec3(0, 0, 0) + max*(vec3(1, 1, 1));
    pos[15] = min*vec3(0, 0, 1) + max*(vec3(1, 1, 0));

    // Connecting faces
    pos[16] = min*vec3(1, 1, 1) + max*(vec3(0, 0, 0));
    pos[17] = min*vec3(0, 1, 1) + max*(vec3(1, 0, 0));

    pos[18] = min*vec3(1, 0, 1) + max*(vec3(0, 1, 0));
    pos[19] = min*vec3(0, 0, 1) + max*(vec3(1, 1, 0));

    pos[20] = min*vec3(1, 1, 0) + max*(vec3(0, 0, 1));
    pos[21] = min*vec3(0, 1, 0) + max*(vec3(1, 0, 1));

    pos[22] = min*vec3(1, 0, 0) + max*(vec3(0, 1, 1));
    pos[23] = min*vec3(0, 0, 0) + max*(vec3(1, 1, 1));


    // Cross inside faces
    // pos[24] = min*vec3(1, 1, 1) + max*(vec3(0, 0, 0));
    // pos[25] = min*vec3(0, 0, 1) + max*(vec3(1, 1, 0));

    // pos[26] = min*vec3(1, 1, 0) + max*(vec3(0, 0, 1));
    // pos[27] = min*vec3(0, 0, 0) + max*(vec3(1, 1, 1));

    // pos[28] = min*vec3(1, 0, 1) + max*(vec3(0, 1, 0));
    // pos[29] = min*vec3(0, 1, 1) + max*(vec3(1, 0, 0));

    // pos[30] = min*vec3(1, 0, 0) + max*(vec3(0, 1, 1));
    // pos[31] = min*vec3(0, 1, 0) + max*(vec3(1, 0, 1));

    // pos[32] = min*vec3(1, 1, 1) + max*(vec3(0, 0, 0));
    // pos[33] = min*vec3(0, 1, 0) + max*(vec3(1, 0, 1));

    // pos[34] = min*vec3(1, 1, 1) + max*(vec3(0, 0, 0));
    // pos[35] = min*vec3(1, 0, 0) + max*(vec3(0, 1, 1));

    // pos[36] = min*vec3(1, 1, 0) + max*(vec3(0, 0, 1));
    // pos[37] = min*vec3(0, 1, 1) + max*(vec3(1, 0, 0));

    // pos[38] = min*vec3(1, 0, 1) + max*(vec3(0, 1, 0));
    // pos[39] = min*vec3(1, 1, 0) + max*(vec3(0, 0, 1));
    
    // pos[40] = min*vec3(1, 0, 1) + max*(vec3(0, 1, 0));
    // pos[41] = min*vec3(0, 0, 0) + max*(vec3(1, 1, 1));

    // pos[42] = min*vec3(1, 0, 0) + max*(vec3(0, 1, 1));
    // pos[43] = min*vec3(0, 0, 1) + max*(vec3(1, 1, 0));

    // pos[44] = min*vec3(0, 0, 0) + max*(vec3(1, 1, 1));
    // pos[45] = min*vec3(0, 1, 1) + max*(vec3(1, 0, 0));

    // pos[46] = min*vec3(0, 0, 1) + max*(vec3(1, 1, 0));
    // pos[47] = min*vec3(0, 1, 0) + max*(vec3(1, 0, 1));

    MeshVao vao(new 
        VertexAttributeGroup({
            VertexAttribute(buff, 0, nbOfPoints, 3, GL_FLOAT, false),
            VertexAttribute(buffNormal, 1, nbOfPoints, 3, GL_FLOAT, false),
            VertexAttribute(buffNormal, 2, nbOfPoints, 3, GL_FLOAT, false)
        })
    );

    setVao(vao);
}

void Draw::addElement(DrawElementBasePtr element)
{
    element->initData();

    // auto chaise = Loader<ObjectGroup>::get("chaise").copy();
    // chaise->state.scaleScalar(100.0f);
    
    EntityModel m(newObjectGroup());
    m->add(element);
    
    EntityRef e = newEntity("Draw Element"
        , m
    );

    drawElements.push_back(e);

    ComponentModularity::addChild(*entity,
        e
    );
}

void Draw::drawLine(vec3 start, vec3 end, float duration, ModelState3D state, vec3 color)
{
    std::shared_ptr<DrawLine> line = std::make_shared<DrawLine>(
          start
        , end
        , globals.appTime.getElapsedTime()
        , duration
        , state
        , color
    );

    queueMutex.lock();
    toDrawQueue.push(line);
    queueMutex.unlock();
}

void Draw::drawSphere(
    vec3 center, 
    float radius, 
    float duration, 
    ModelState3D state,
    vec3 color, 
    int n_slices, 
    int n_stacks)
{
    std::shared_ptr<DrawSphere> sphere = std::make_shared<DrawSphere>(
          center
        , radius
        , n_slices
        , n_stacks
        , globals.appTime.getElapsedTime()
        , duration
        , state
        , color
    );

    queueMutex.lock();
    toDrawQueue.push(sphere);
    queueMutex.unlock();
}

void Draw::drawBox(
    vec3 min, 
    vec3 max, 
    float duration,
    ModelState3D state,
    vec3 color)
{
    std::shared_ptr<DrawBox> box = std::make_shared<DrawBox>(
          min
        , max
        , globals.appTime.getElapsedTime()
        , duration
        , state
        , color
    );

    queueMutex.lock();
    toDrawQueue.push(box);
    queueMutex.unlock();
}

void Draw::drawBox(
    rp3d::BoxShape* boxShape,
    const rp3d::Transform& transform,
    float duration,
    ModelState3D state,
    vec3 color)
{
    state.setRotation(eulerAngles(PG::toglm(transform.getOrientation())));
    state.setPosition(PG::toglm(transform.getPosition()));

    std::shared_ptr<DrawBox> box = std::make_shared<DrawBox>(
          PG::toglm(-boxShape->getHalfExtents())
        , PG::toglm( boxShape->getHalfExtents())
        , globals.appTime.getElapsedTime()
        , duration
        , state
        , color
    );

    queueMutex.lock();
    toDrawQueue.push(box);
    queueMutex.unlock();
}

void Draw::drawBoxFromHalfExtents(
    vec3 center, 
    vec3 halfExtents, 
    float duration,
    ModelState3D state,
    vec3 color)
{
    vec3 min = center - halfExtents;
    vec3 max = center + halfExtents;

    std::shared_ptr<DrawBox> box = std::make_shared<DrawBox>(
          min
        , max
        , globals.appTime.getElapsedTime()
        , duration
        , state
        , color
    );

    queueMutex.lock();
    toDrawQueue.push(box);
    queueMutex.unlock();
}

void Draw::reset()
{
    EntityModel drawElementGroup = EntityModel{newObjectGroup()};
    drawElements.clear();
    drawElements = std::list<EntityRef>();

    entity = nullptr;
}

void Draw::update()
{
    if (entity->comp<EntityGroupInfo>().parent == nullptr)
    {
        if (entity != nullptr)
            reset();
        return;
    }

    queueMutex.lock();

    while (!toDrawQueue.empty())
    {
        DrawElementBasePtr elem = toDrawQueue.front();
        toDrawQueue.pop();

        addElement(elem);
    }

    queueMutex.unlock();

    if (drawElements.size() == 0 || entity == nullptr)
        return;
    
    auto it = drawElements.begin();
    while (it != drawElements.end())
    {
        if (((DrawElementBase*)((*it)->comp<EntityModel>()->getMeshes()[0].get()))->isTimeElapsed())
        {
            ComponentModularity::removeChild(*entity, *it);
            it = drawElements.erase(it);
            continue;
        }
        it++;
    }

    // drawElementGroup->iterateOnAllMesh_Recursive([](ModelRef model){
    //     WARNING_MESSAGE("model info: ", model->state.modelMatrix);
    // });

    ManageGarbage<EntityModel>();
}