#include "Graphics/Textures.hpp"
#include "PhysicsGlobals.hpp"
#include "Timer.hpp"
#include "Utils.hpp"
#include <EntityBlueprint.hpp>
#include <AssetManager.hpp>
#include <Helpers.hpp>
#include <Constants.hpp>
#include <EntityStats.hpp>
#include <AnimationBlueprint.hpp>
#include <GameConstants.hpp>
#include <GameGlobals.hpp>
#include <reactphysics3d/collision/shapes/HeightFieldShape.h>

#include <JoltIntegration/PhysicsCommons.hpp>
#include <Jolt/Physics/Body/BodyCreationSettings.h>
#include <Jolt/Physics/Collision/Shape/Shape.h>
#include <Jolt/Physics/Collision/Shape/HeightFieldShape.h>

EntityRef Blueprint::SpawnMainGameTerrain()
{
    return Blueprint::Terrain(terrainConst::mapFileName, terrainConst::terrainSize, vec3(0), terrainConst::cellSize);
}


void Blueprint::terrainChunk(
    EntityRef chunk, 
    int i, 
    int j, 
    vec3 terrainSize, 
    vec3 terrainPosition,
    ivec2 textureSize, 
    const float *src, 
    int cellSize, 
    bool addModel,
    ModelRef terrainBaseModel
)
{
    /*........ Preaparing Chunk Data ........*/
    ivec2 gridDim = ivec2(terrainSize.x, terrainSize.z)/cellSize;

    vec2 uvmin = vec2(i, j)/vec2(gridDim);
    vec2 uvmax = vec2(i+1, j+1)/vec2(gridDim);
    vec2 uvhalf = (vec2((float)i+0.5f, (float)j+0.5f)/vec2(gridDim)) - 0.5f;

    ivec2 iuvmin = round(uvmin*vec2(textureSize));
    ivec2 iuvmax = round(uvmax*vec2(textureSize));
    int dsize = max(iuvmax.x - iuvmin.x, iuvmax.y - iuvmin.y) + 1;

    std::vector<float> heightData(dsize*dsize);

    float minV = 1e6;
    float maxV = -1e6;

    for(int j = 0; j < dsize; j++)
    {
        for(int i = 0; i < dsize; i++)
        {
            int id = i * dsize + j;
            int id2 = ((i + iuvmin.y)*textureSize.x + j + iuvmin.x);
            id2 = min(id2, textureSize.x*textureSize.y);
            heightData[id] = src[id2];

            minV = min(heightData[id], minV);
            maxV = max(heightData[id], maxV);
        }
    }

    /*........ Adding Model ........*/
    if(addModel)
    {
        ModelRef t = terrainBaseModel->copy();

        t->defaultMode = GL_PATCHES;
        t->noBackFaceCulling = false;
        t->state.frustumCulled = true;
        t->tessActivate(vec2(1, 16), vec2(25, 250));
        t->tessHeighFactors(1, terrainSize.y/terrainSize.x);

        t->tessHeightTextureRange(uvmin, uvmax);

        EntityModel model = EntityModel{newObjectGroup()};
        model->add(t);

        model->update();
        model->updateMeshesBoundingBox();

        vec3 cellPos = terrainPosition + vec3(terrainSize.x*uvhalf.x, 0, terrainSize.z*uvhalf.y);

        model->setStaticAABB(
            vec3(cellPos + model->getMeshesBoundingBox().first) *vec3(1, 0, 1) + vec3(0, minV*terrainSize.y, 0), 
            vec3(cellPos + model->getMeshesBoundingBox().second)*vec3(1, 0, 1) + vec3(0, maxV*terrainSize.y, 0)
        );

        
        chunk->set<EntityModel>(model);
    }

    /*........ Adding Jolt Body ........*/
    JPH::BodyCreationSettings settings;
    JPH::HeightFieldShapeSettings jshape(
        heightData.data(),
        Vvec3(-cellSize/2.f, 0, -cellSize/2.f),
        // Vvec3(cellHscale/(float)(dsize-1.f), terrainSize.y, cellHscale/(float)(dsize-1.f)),
        Vvec3(cellSize/(float)(dsize-1.f), terrainSize.y, cellSize/(float)(dsize-1.f)),
        dsize
    );
    float a, b, c = 1.0;
    jshape.mBitsPerSample = 16;
    jshape.mBlockSize = 8;
    jshape.DetermineMinAndMaxSample(a, b, c);
    jshape.mMaxHeightValue = maxV;
    jshape.mMinHeightValue = minV;

    JPH::Shape::ShapeResult result = jshape.Create();
    if(result.IsValid())
        settings.SetShape(result.Get());
    else
        ERROR_MESSAGE("Non-valid shape ", result.GetError())

    settings.mMotionType = JPH::EMotionType::Static;
    // settings.mPosition = Vvec3(terrainPosition + vec3(terrainSize.x*uvhalf.x, 0, terrainSize.z*uvhalf.y));
    settings.mRotation = JPH::Quat::sIdentity();
    settings.mRestitution = 0;
    settings.mFriction = 1.0;
    settings.mObjectLayer = JPH::ObjectLayerPairFilterMask::sGetObjectLayer(1<<JoltVulpine::Layers::ENVIRONEMENT, 1<<JoltVulpine::Layers::ENVIRONEMENT);

    settings.mPosition = Vvec3(chunk->comp<State3D>().position);

    JPH::Body *body = JoltVulpine::jPhysicsSystem->GetBodyInterface().CreateBody(settings);

    JoltVulpine::bodiesToAddMutex.lock();
    JoltVulpine::bodiesToAdd.push_back(body->GetID());
    JoltVulpine::bodiesToAddMutex.unlock();

    chunk->set<JoltBody>({body->GetID()});

    // return body->GetID();
}


EntityRef Blueprint::Terrain(
    const char *mapName, 
    vec3 terrainSize,
    vec3 terrainPosition,
    int cellSize
)
{
    NAMED_TIMER(HeightMap_loading)
    NAMED_TIMER(TerrainEntityCreation)


    EntityRef terrainRoot = newEntity("Terrain Root", state3D(true));

    // Texture2D HeightMap = Texture2D()
    //     .loadFromFileHDR(mapPath)
    //     .setFormat(GL_RGB)
    //     .setInternalFormat(GL_RGB32F)
    //     .setPixelType(GL_FLOAT)
    //     .setWrapMode(GL_REPEAT) 
    //     .setFilter(GL_LINEAR)
    //     .generate();

    HeightMap_loading.start();

    /* Make Sure to refresh the heightmap */
    // Loader<Texture2D>::erase(mapName);

    // if(Loader<Texture2D>::loadedAssets.find(mapName) != Loader<Texture2D>::loadedAssets.end())
    // {
    //     NOTIF_MESSAGE("DELETING HEIGHTMAP")

    //     Loader<Texture2D>::loadedAssets.erase(mapName);
    //     auto buff = Loader<Texture2D>::loadingInfos[mapName]->buff; 
    //     buff->resetData();
    //     Loader<Texture2D>::addInfos(buff);
    // }

    Texture2D &HeightMap = Loader<Texture2D>::get(mapName);

    if(
        HeightMap.getPixelType() != GL_FLOAT
        &&
        HeightMap.getFormat() != GL_DEPTH_COMPONENT
    )
    {
        ERROR_MESSAGE("Can't create terrain entity, requested heightmap '" ,  mapName ,  "' isn't a single channel floating point texture. The function will return an empty entity, have fun with your level !");
        return  terrainRoot;
    }
    
    HeightMap.generate();


    HeightMap_loading.stop();
    // std::cout << HeightMap_loading;

    for(auto i : PG::heightFields)
    {
        PG::common.destroyHeightFieldShape(i.second);
        PG::common.destroyHeightField(i.first);
    }
    PG::heightFields.clear();

    TerrainEntityCreation.start();

    float *src = ((float *)HeightMap.getPixelSource());
    ivec2 textureSize = HeightMap.getResolution();

    ivec2 gridDim = ivec2(terrainSize.x, terrainSize.z)/cellSize;
    
    float cellHscale = cellSize;

    ModelRef terrain = newModel(Loader<MeshMaterial>::get("terrain_paintPBR"), 
    // Loader<MeshVao>::get("terrainPlane")
    Loader<MeshVao>::get("4x4_terrainPlane")
    );

    terrain->state.setScale(vec3(cellHscale, terrainSize.y, cellHscale));
    terrain->noBackFaceCulling = false;
    terrain->defaultMode = GL_PATCHES;
    terrain->setMap(HeightMap, 2);
    terrain->setMap(Loader<Texture2D>::get("Grassyness"), 3);

    vec3 aabmin = terrain->getVao()->getAABBMin();
    vec3 aabmax = terrain->getVao()->getAABBMax();

    
    aabmin.y = 0.f;
    aabmax.y = 1.0;

    terrain->getVao()->setAABB(aabmin, aabmax);

    for(int i = 0; i < gridDim.x; i++)
    for(int j = 0; j < gridDim.y; j++)
    {
        vec2 uvhalf = (vec2((float)i+0.5f, (float)j+0.5f)/vec2(gridDim)) - 0.5f;
        vec3 pos = Vvec3(terrainPosition + vec3(terrainSize.x*uvhalf.x, 0, terrainSize.z*uvhalf.y));

        EntityRef chunk = newEntity(
            "Terrain cell" + std::to_string(i) + "x" + std::to_string(j), 
            HeightFieldDummyFlag(),
            State3D({pos})
        );

        terrainChunk(chunk, i, j, terrainSize, terrainPosition, textureSize, src, cellSize, true, terrain);

        ComponentModularity::addChild(*terrainRoot, chunk);        
    }

    // HeightMap.freeSource();

    TerrainEntityCreation.stop();
    // std::cout << TerrainEntityCreation;

    return terrainRoot;
}


void Blueprint::Assembly::AddEntityBodies(
    rp3d::RigidBody *body, 
    void *usrData,
    const std::vector<std::pair<rp3d::CollisionShape *, rp3d::Transform>> &environementals,
    const std::vector<std::pair<rp3d::CollisionShape *, rp3d::Transform>> &hitboxes
    )
{
    for(auto &i : environementals)
    {
        auto c = body->addCollider(i.first, i.second);
        c->getMaterial().setBounciness(0.f);
        // c->getMaterial().setFrictionCoefficient(1.f);:
        c->getMaterial().setFrictionCoefficient(0.5);
        c->setCollisionCategoryBits(1<<CollideCategory::ENVIRONEMENT);
        c->setCollideWithMaskBits(1<<CollideCategory::ENVIRONEMENT);
        c->setUserData(usrData);
    }

    for(auto &i : hitboxes)
    {
        auto c = body->addCollider(i.first, i.second);
        c->setIsTrigger(true);
        c->setCollisionCategoryBits(1<<CollideCategory::HITZONE);
        c->setCollideWithMaskBits(1<<CollideCategory::HITZONE);
        c->setUserData(usrData);
    }
}

RigidBody Blueprint::Assembly::CapsuleBody(float height, vec3 position, EntityRef entity)
{
    rp3d::RigidBody *body = PG::world->createRigidBody(rp3d::Transform(PG::torp3d(position), DEFQUAT));

    body->setAngularLockAxisFactor(rp3d::Vector3(0, 1, 0));

    const float capsuleHeight = height;
    const float capsuleRadius = height*0.25;
    const float capsuleLength = capsuleHeight - capsuleRadius*2.f;

    AddEntityBodies(body, entity.get(), 
        {
            {
                PG::common.createCapsuleShape(capsuleRadius, capsuleLength), 
                rp3d::Transform({0.f, capsuleHeight*0.5f, 0.f}, DEFQUAT)
            }
        }, 
        {
            {
                PG::common.createCapsuleShape(capsuleRadius*0.95, capsuleLength), 
                rp3d::Transform({0.f, capsuleHeight*0.5f, 0.f}, DEFQUAT)
            }
        });
    
    body->setMass(75);

    return body;
}