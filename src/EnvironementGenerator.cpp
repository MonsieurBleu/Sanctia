#include <EnvironementGenerator.hpp>
#include <AssetManagerUtils.hpp>
#include <MathsUtils.hpp>

#include <EntityBlueprint.hpp>

void GrassGenerator::init()
{
    models[0] = Loader<ObjectGroup>::get("Grass Patch 4x64x64").copy();
    models[1] = Loader<ObjectGroup>::get("Grass Patch 2x32x32").copy();
    models[2] = Loader<ObjectGroup>::get("Grass Patch 1x16x16").copy();
    models[3] = Loader<ObjectGroup>::get("Grass Patch 1x8x8").copy();

    for(int i = 0; i < 4; i++)
    {
        models[i]->
        getInstances()[0]
            .originalModel
            ->baseUniforms.add(ShaderUniform(float(i), 31));

        globals.getScene()->add(models[i]);
    }
}

void GrassGenerator::update()
{
    for(int i = 0; i < 4; i++)
        models[i]->getInstances()[0].originalModel->resetInstances();

    if(!active) return;

    constexpr float patchSize = 16.f;
    constexpr int gridRes = 23;

    vec2 camDir2D = normalize(vec2(globals.currentCamera->getDirection().x, globals.currentCamera->getDirection().z));
    // float angle = atan2f(camDir2D.y, camDir2D.x);
    float angleMax = cos(globals.currentCamera->getState().FOV*0.75f);
    // float angleMax = cos(globals.currentCamera->getState().FOV*2);

    angleMax = mix(angleMax, -1.f, max(0.f, -globals.currentCamera->getDirection().y*2.f));

    vec2 off[4] = 
    {
        vec2(-.5f, -.5f), 
        vec2(+.5f, -.5f), 
        vec2(-.5f, +.5f), 
        vec2(+.5f, +.5f), 
    };

    for(int i = -gridRes+1; i < gridRes; i++)
        for(int j = -gridRes+1; j < gridRes; j++)
        {
            ivec2 uvi(i, j);
            vec2 uv(uvi);

            bool skip = true;
            int minLod = 4;

            for(auto &o : off)
            {
                vec2 uv2 = uv + o;

                float l = length(uv2);
                skip &= dot(camDir2D, uv2) < angleMax*l and l > 2.0;

                float d = l/(float)gridRes;
                d = log2(max(2.f, 64.f*d))-2.f;
                int lod = max(floor(d), 0.f);

                minLod = min(minLod, lod);

            }
            
            // skip = false;

            if(skip or minLod >= 4) continue;

            auto instance = models[minLod]->getInstances()[0].originalModel->createInstance();
            instance->setPosition(vec3(uv.x, 0, uv.y)*patchSize);
            instance->update();
        }
}

void WaterGenerator::init()
{
    /// ...
}

EntityRef WaterGenerator::generate()
{
    cellSize = 128.f;

    ModelRef terrain = newModel(
        Loader<MeshMaterial>::get("Water"), 
        Loader<MeshVao>::get("8x8_terrainPlane")
    );

    ivec2 gridDim = vec2(Blueprint::terrainConst::terrainSize.x, Blueprint::terrainConst::terrainSize.z)/cellSize;

    Texture2D HeightMap = Loader<Texture2D>::get(Blueprint::terrainConst::mapFileName);
    Texture2D WaterLevel = Loader<Texture2D>::get("Water Level");

    terrain->state.setScale(
        vec3(cellSize, Blueprint::terrainConst::terrainSize.y, cellSize));
    terrain->defaultMode = GL_PATCHES;
    terrain->setMap(HeightMap, 2);
    terrain->setMap(WaterLevel, 3);

    terrain->sorted = false;
    terrain->transparent = true;

    entity = newEntity("Water");

    for(int i = 0; i < gridDim.x; i++)
        for(int j = 0; j < gridDim.y; j++)
        {
            vec2 uvhalf = (vec2((float)i+0.5f, (float)j+0.5f)/vec2(gridDim)) - 0.5f;
            vec3 pos = vec3(vec3(Blueprint::terrainConst::terrainSize.x*uvhalf.x, 0, Blueprint::terrainConst::terrainSize.z*uvhalf.y));

            vec2 uvmin = vec2(i, j)/vec2(gridDim);
            vec2 uvmax = vec2(i+1, j+1)/vec2(gridDim);

            ModelRef t = terrain->copy();

            t->defaultMode = GL_PATCHES;
            t->noBackFaceCulling = true;
            t->state.frustumCulled = true;
            t->sorted = false;
            // t->sorted = true;
            t->tessActivate(vec2(1, 16), vec2(25, 250));
            t->tessHeighFactors(1, Blueprint::terrainConst::terrainSize.y/Blueprint::terrainConst::terrainSize.x);

            t->tessHeightTextureRange(uvmin, uvmax);

            EntityModel model = EntityModel{newObjectGroup()};
            model->add(t);

            model->update();
            model->updateMeshesBoundingBox();

            vec3 cellPos = vec3(Blueprint::terrainConst::terrainSize.x*uvhalf.x, 0, Blueprint::terrainConst::terrainSize.z*uvhalf.y);

            float minV = 0.f; // TODO : fill later
            float maxV = 1.f; // TODO : fill later

            model->setStaticAABB(
                vec3(cellPos + model->getMeshesBoundingBox().first) *vec3(1, 0, 1) + vec3(0, minV*Blueprint::terrainConst::terrainSize.y, 0), 
                vec3(cellPos + model->getMeshesBoundingBox().second)*vec3(1, 0, 1) + vec3(0, maxV*Blueprint::terrainConst::terrainSize.y, 0)
            );

            EntityRef chunk = newEntity(
                "Water Cell" + std::to_string(i) + "x" + std::to_string(j), 
                State3D({pos}),
                model
            );

            ComponentModularity::addChild(*entity, chunk); 
        }

    return entity;
}

void WaterGenerator::clear()
{
    entity = EntityRef();
}

AUTOGEN_DATA_RW_FUNC_AN(BiomeInfos,
    Grassyness,
    ForestDensity,
    Humidity,
    Mystic,
    Corruption,
    Slope,
    Elevation
    )

template<>
BiomeInfos& Loader<BiomeInfos>::loadFromInfos()
{
    EARLY_RETURN_IF_LOADED
    LOADER_ASSERT(NEW_VALUE)

    r = DataLoader<BiomeInfos>::read(buff);

    EXIT_ROUTINE_AND_RETURN
}

AUTOGEN_DATA_RW_FUNC_STRUCT_AN(EntityScatterer::SpawnInfo, 
    (
        scaleRangeMin,
        scaleRangeMax,
        radialRange,
        densityPerCell
    ),
    (
        entities,
        spawnWeightsCenter,
        spawnWeightsRange
    )
)

template<>
EntityScatterer::SpawnInfo& Loader<EntityScatterer::SpawnInfo>::loadFromInfos()
{
    EARLY_RETURN_IF_LOADED

    r = DataLoader<EntityScatterer::SpawnInfo>::read(buff);

    EXIT_ROUTINE_AND_RETURN
}

DATA_WRITE_FUNC(std::vector<EntityScatterer::SpawnInfo>)
{
    out->Tabulate();

    for(auto &d : data)
    {
        out->Entry();
        out->write("SpawnInfo", 9);
        out->Tabulate();
        DataLoader<EntityScatterer::SpawnInfo>::write(d, out);
    }

    DATA_WRITE_END
}

DATA_READ_FUNC_INIT(std::vector<EntityScatterer::SpawnInfo>)

    if(true)
        data.push_back(DataLoader<EntityScatterer::SpawnInfo>::read(buff));

DATA_READ_END_FUNC

AUTOGEN_DATA_RW_FUNC_STRUCT_AN(EntityScatterer,
    (
        rangeMin,
        rangeMax,
    ),
    (
        spawns_old,
        spawnsNames
    )
);

template<>
EntityScatterer& Loader<EntityScatterer>::loadFromInfos()
{
    EARLY_RETURN_IF_LOADED

    r = DataLoader<EntityScatterer>::read(buff);

    EXIT_ROUTINE_AND_RETURN
}


void BiomeInfos::writeToFile(std::string filename)
{
    std::string fileName(filename);
    NOTIF_MESSAGE("Creating file : " ,  fileName)
    auto out  = VulpineTextOutputRef(new VulpineTextOutput(1<<16));
    out->write("~", 1);
    out->Tabulate();
    DataLoader<BiomeInfos>::write(*this, out)->saveAs(fileName.c_str());
}

void EntityScatterer::writeToFile(std::string filename)
{
    std::string fileName(filename);
    NOTIF_MESSAGE("Creating file : " ,  fileName)
    auto out  = VulpineTextOutputRef(new VulpineTextOutput(1<<16));
    out->write("~", 1);
    DataLoader<EntityScatterer>::write(*this, out)->saveAs(fileName.c_str());
}

void EntityScatterer::SpawnInfo::writeToFile(std::string filename)
{
    std::string fileName(filename);
    NOTIF_MESSAGE("Creating file : " ,  fileName)
    auto out  = VulpineTextOutputRef(new VulpineTextOutput(1<<16));
    out->write("~", 1);
    DataLoader<EntityScatterer::SpawnInfo>::write(*this, out)->saveAs(fileName.c_str());
}


EntityScatterer::EntityScatterer(){};

EntityScatterer::EntityScatterer(
    vec2 rangeMin,
    vec2 rangeMax,
    const std::vector<EntityScatterer::SpawnInfo> & spawns
) : 
    // generator(rand_dev()), distx(0, 2048), disty(0, 2048),
    rangeMin(rangeMin), rangeMax(rangeMax), spawns_old(spawns)
{
    
}

// void updateTerrainFromGPU()
// {
//     auto &terrain = Loader<Texture2D>::get("Herault_4096");

//     terrain.updateSourceFromGPU();
// }

float getTerrainHeightBase(vec2 pos)
{
    static auto &terrain = Loader<Texture2D>::get("Herault_4096");

    float *pixels = (float *)terrain.getPixelSource();

    ivec2 pixelPos = ivec2(round(2048.f + pos));

    const ivec2 res = terrain.getResolution();
    pixelPos = clamp(pixelPos, ivec2(0), res-1);

    return pixels[pixelPos.x*res.x + pixelPos.y]*512.f;
}

float getTerrainHeight(vec2 pos)
{
    float h = getTerrainHeightBase(pos);

    // float hx1 = getTerrainHeightBase(pos + vec2(1, 0));
    // float hx2 = getTerrainHeightBase(pos - vec2(1, 0));

    // float hz1 = getTerrainHeightBase(pos + vec2(0, 1));
    // float hz2 = getTerrainHeightBase(pos - vec2(0, 1));

    // // float hd1 = getTerrainHeightBase(pos + vec2(+1, +1));
    // // float hd2 = getTerrainHeightBase(pos + vec2(-1, -1));
    // // float hd3 = getTerrainHeightBase(pos + vec2(-1, +1));
    // // float hd4 = getTerrainHeightBase(pos + vec2(+1, -1));

    // float ax1 = linearstep(.5f, 1.f, fract(pos.x));
    // float ax2 = linearstep(.5f, 0.f, fract(pos.x));

    // float az1 = linearstep(.5f, 1.f, fract(pos.y));
    // float az2 = linearstep(.5f, 0.f, fract(pos.y));

    // h = mix(h, hx1, ax1);
    // h = mix(h, hx2, ax2);
    // h = mix(h, hz1, az1);
    // h = mix(h, hz2, az2);

    return h;
}

float getBiomeMap(vec2 pos, std::string name)
{
    auto &terrain = Loader<Texture2D>::get(name);

    float *pixels = (float *)terrain.getPixelSource();

    const ivec2 res = terrain.getResolution();
    ivec2 pixelPos = ivec2(vec2(res)*round(2048.f + pos)/4096.f);
    pixelPos = clamp(pixelPos, ivec2(0), res);

    // ERROR_MESSAGE(pixelPos);

    return pixels[pixelPos.x*res.x + pixelPos.y];
}


BiomeInfos EntityScatterer::getLocalInfos(vec2 center)
{
    BiomeInfos l;

    l.Elevation = getTerrainHeight(center);

    float bias = 1;
    float h1 = getTerrainHeight(center + vec2(+bias, +bias));
    float h2 = getTerrainHeight(center + vec2(+bias, -bias));
    float h3 = getTerrainHeight(center + vec2(-bias, +bias));
    float h4 = getTerrainHeight(center + vec2(-bias, -bias));
    float slope = 0.25f*(abs(l.Elevation-h1) + abs(l.Elevation-h2) + abs(l.Elevation-h3) + abs(l.Elevation-h4));

    l.Elevation = l.Elevation/512.f;
    l.Slope = slope;

    l.Grassyness = getBiomeMap(center, "Grassyness");
    l.ForestDensity = getBiomeMap(center, "Forest Density");

    return l;
}

float EntityScatterer::evaluateDensity(const SpawnInfo &biome, const BiomeInfos &local)
{
    float d = 1.0;

    int weightCount = 0;

    #define WEIGHT_BIOME_INFO(i) \
        if(biome.spawnWeightsRange.i > 0.f){ \
            weightCount++; \
            d *= smoothstep(1.f, 0.f, abs(local.i-biome.spawnWeightsCenter.i)/biome.spawnWeightsRange.i); \
        }

    WEIGHT_BIOME_INFO(Elevation)
    WEIGHT_BIOME_INFO(Slope)
    WEIGHT_BIOME_INFO(ForestDensity)
    WEIGHT_BIOME_INFO(Grassyness)
    WEIGHT_BIOME_INFO(Humidity)
    WEIGHT_BIOME_INFO(Mystic)
    WEIGHT_BIOME_INFO(Corruption)

    return weightCount > 0 ? d : 0.f;
}


void EntityScatterer::generateCancel()
{
    genDone = true;
    genTimer = BenchTimer();
    genParent = nullptr;

    return;
}

void EntityScatterer::generateInit(vec2 min, vec2 max, EntityRef parent)
{
    genMin = genCurrentCell = min;
    genMax = max;
    genDone = false;

    genTimer = BenchTimer();
    genTimer.start();

    genParent = parent.get();

    return;
}

float EntityScatterer::generateGetProgress()
{
    if(genDone) return 1.f;

    return (genCurrentCell.x-genMin.x)/(genMax.x-genMin.x);
}

float EntityScatterer::generateGetTime()
{
    return genTimer.getElapsedTime();
}

void EntityScatterer::generateFrame(float timeAllowed)
{
    if(genDone) return;

    BenchTimer timePassed;

    // ERROR_MESSAGE("HELLO 1")

    // physicsMutex.lock();

    // ERROR_MESSAGE("HELLO 2")
    // WARNING_MESSAGE(genCurrentCell.x)

    for(; genCurrentCell.x <= genMax.x; genCurrentCell.x += cellSize, genCurrentCell.y = genMin.y)
    {
        // WARNING_MESSAGE(genCurrentCell.x)

        for(; genCurrentCell.y <= genMax.y; genCurrentCell.y += cellSize)
        {
            // WARNING_MESSAGE(genCurrentCell)
            // WARNING_MESSAGE(genCurrentCell, " ", timePassed.getElapsedTime()*1000.f, " ", timeAllowed)

            timePassed.start();

            BiomeInfos local = getLocalInfos(genCurrentCell);


            // std::default_random_engine generator(genCurrentCell.x + genCurrentCell.y*0.25f + genCurrentCell.x*genCurrentCell.y*0.5f);
            std::default_random_engine generator(512.f*random01Vec2(genCurrentCell*0.01f));

            std::normal_distribution<float> posx(-1.f, +1.f);
            std::normal_distribution<float> posy(-1.f, +1.f);

            std::vector<vec2> otherPos;
            
            for(auto &i : spawnsNames)
            {
                auto &spawn = Loader<EntityScatterer::SpawnInfo>::get(i);
                
                float density = evaluateDensity(spawn, local);

                if(density == 0.f) continue;
                
                std::uniform_int_distribution<int> name(0, spawn.entities.size()-1);
                std::uniform_real_distribution<float> scale(spawn.scaleRangeMin, spawn.scaleRangeMax);
                std::uniform_real_distribution<float> radialx(-spawn.radialRange.x, spawn.radialRange.x);
                std::uniform_real_distribution<float> radialy(-spawn.radialRange.y, spawn.radialRange.y);
                std::uniform_real_distribution<float> radialz(-spawn.radialRange.z, spawn.radialRange.z);

                // std::normal_distribution<float> 
                std::uniform_real_distribution<float>
                    spawnChanceDecimal(0.f, 1.f);

                spawnChanceDecimal(generator); /* Call one time the generator to not have result close to 0 almost all the time*/
                float decimalSpawn = spawnChanceDecimal(generator);
                int numberToSpawn = round(spawn.densityPerCell*density) + (abs(decimalSpawn) < fract(spawn.densityPerCell)*density ? 1 : 0);

                // if(numberToSpawn) 
                    // ERROR_MESSAGE(
                    //     numberToSpawn, "   ",
                    //     decimalSpawn, "   ",
                    //     fract(spawn.densityPerCell*density), "   ",
                    //     round(spawn.densityPerCell*density), "   ",
                    //     (decimalSpawn < fract(spawn.densityPerCell*density) ? 1 : 0)
                    // )

                for(int j = 0; j < numberToSpawn; j++)
                {
                    vec2 pos;
                    vec2 maxDistPos;

                    const int maxTry = 5;
                    float dist = 0.f;
                    for(int l = 0; l < maxTry; l++)
                    {
                        pos = vec2(posx(generator), posy(generator));
                        pos = genCurrentCell + pos*(rangeMin + abs(vec2(cos(pos.x), sin(pos.y)))*rangeMax);

                        float minDist = 1e6;
                        for(auto &p : otherPos)
                            minDist = min(distance(p, pos), minDist);

                        if(minDist > dist)
                        {
                            maxDistPos = pos;
                            dist = minDist;
                        }
                    }

                    pos = maxDistPos;
                    otherPos.push_back(pos);

                    // static BenchTimer test("Time To Spawn Entity");
                    

                    float modelScale = scale(generator);
                    
                    HierarchyState3D state;
                    state.position = vec3(pos.y, getTerrainHeight(pos), pos.x);
                    state.rotation = quat(radians(vec3(radialx(generator),radialy(generator),radialz(generator))));
                    state.scale = vec3(modelScale);
                    spawnEntityToParent(spawn.entities[name(generator)], *genParent, state);

                    // EntityRef e = spawnEntity(
                    //     spawn.entities[name(generator)],
                    //     vec3(pos.y, getTerrainHeight(pos), pos.x),
                    //     quat(radians(vec3(radialx(generator),radialy(generator),radialz(generator))))
                    // );


                    // // WARNING_MESSAGE(modelScale);
                    // if(e->has<EntityModel>())
                    // {
                    //     e->comp<EntityModel>()->state.scaleScalar(modelScale);
                    //     e->comp<EntityModel>()->update();
                    // }

                    // // test.start();

                    // ComponentModularity::addChild(*genParent, e);
                    // genParent->comp<EntityGroupInfo>().children.push_back(e);
                    // WARNING_MESSAGE(genParent->comp<EntityGroupInfo>().children.size())
                    
                    // test.stop();



                    // WARNING_MESSAGE("Time To Spawn Entity '", name, "' : ", test.getDeltaMS(), " ms,  Average : ", test.getElapsedTime()*1000.f/(float)test.getUpdateCounter())
                }
            }
            
            timePassed.stop();


            if(timePassed.getElapsedTime()*1000.f >= timeAllowed)
            {
                physicsMutex.unlock();
                // NOTIF_MESSAGE("TERMINAED BY TIMER")
                return;
            }
        }
    }

    // physicsMutex.unlock();

    // NOTIF_MESSAGE("TERMINAED BY END LOOP")
    // WARNING_MESSAGE(genCurrentCell, " ", genMax, " ", cellSize)
    // WARNING_MESSAGE(genCurrentCell.x >= genMax.x, genCurrentCell.y >= genMax.y)

    if(genCurrentCell.x > genMax.x)
    {
        // NOTIF_MESSAGE("TERMINAED BY END GEN")
        genDone = true;
        genTimer.stop();
    }
}


int EntityScatterer::generate_old(
    vec2 center,
    BiomeInfos infos,
    int minNumber,
    int maxNumber,
    EntityRef Parent
)
{
    int count = 0;

    for(auto &i : spawns_old)
    {
        // int number = minNumber + (maxNumber-minNumber)*(
        //     (1.f-abs(i.spawnWeightsCenter.Grassyness-infos.Grassyness))*i.spawnWeightsRange.Grassyness +
        //     (1.f-abs(i.spawnWeightsCenter.ForestDensity-infos.ForestDensity))*i.spawnWeightsRange.ForestDensity +
        //     (1.f-abs(i.spawnWeightsCenter.Humidity-infos.Humidity))*i.spawnWeightsRange.Humidity +
        //     (1.f-abs(i.spawnWeightsCenter.Mystic-infos.Mystic))*i.spawnWeightsRange.Mystic +
        //     (1.f-abs(i.spawnWeightsCenter.Corruption-infos.Corruption))*i.spawnWeightsRange.Corruption
        // )
        //     /
        // (
        //     step(1e-6f, i.spawnWeightsRange.Grassyness) + 
        //     step(1e-6f, i.spawnWeightsRange.ForestDensity) + 
        //     step(1e-6f, i.spawnWeightsRange.Humidity) + 
        //     step(1e-6f, i.spawnWeightsRange.Mystic) + 
        //     step(1e-6f, i.spawnWeightsRange.Corruption)
        // )
        // ;

        float weightSum = 1.f;
        float weightCount = 0.f;


        #define ADD_BIOME_INFO(info) if(i.spawnWeightsRange.info > 0.f)\
            {weightCount ++; weightSum *= smoothstep(1.f, 0.f, (abs(i.spawnWeightsCenter.info-infos.info)/i.spawnWeightsRange.info));}

        ADD_BIOME_INFO(ForestDensity)
        // ADD_BIOME_INFO(Slope)

        if(weightCount > 0.f)
            weightSum /= weightCount;
        else
            weightSum = 0.f;

        // WARNING_MESSAGE(weightSum);

        int number = minNumber + (maxNumber-minNumber)*weightSum;

        if(!number) continue;

        std::default_random_engine generator(center.x + center.y*0.25f + center.x*center.y*0.5f);



        std::normal_distribution<float>  
        // std::uniform_real_distribution<float>
            posx(-1.f, +1.f);

        std::normal_distribution<float>  
        // std::uniform_real_distribution<float>
            posy(-1.f, +1.f);

        std::uniform_int_distribution<int>     name(0, i.entities.size()-1);

        std::uniform_real_distribution<float>  scale(i.scaleRangeMin, i.scaleRangeMax);

        std::uniform_real_distribution<float>  radialx(-i.radialRange.x, i.radialRange.x);
        std::uniform_real_distribution<float>  radialy(-i.radialRange.y, i.radialRange.y);
        std::uniform_real_distribution<float>  radialz(-i.radialRange.z, i.radialRange.z);

        // std::uniform_real_distribution<float>  slope(-0.1, 0.1);

        // WARNING_MESSAGE(number)

        std::vector<vec2> otherPos(number);

        for(int j = 0; j < number; j++)
        {
            vec2 pos;
            vec2 maxDistPos;

            const int maxTry = 5;
            float dist = 0.f;
            for(int l = 0; l < maxTry; l++)
            {
                pos = vec2(posx(generator), posy(generator));
                pos = center + pos*(rangeMin + abs(vec2(cos(pos.x), sin(pos.y)))*rangeMax);

                float minDist = 1e6;
                for(auto &p : otherPos)
                    minDist = min(distance(p, pos), minDist);

                if(minDist > dist)
                {
                    maxDistPos = pos;
                    dist = minDist;
                }
            }

            pos = maxDistPos;
            otherPos.push_back(pos);

            float bias = 2;
            float h0 = getTerrainHeight(pos);
            float h1 = getTerrainHeight(pos + vec2(+bias, +bias)*0.5f);
            float h2 = getTerrainHeight(pos + vec2(+bias, -bias)*0.5f);
            float h3 = getTerrainHeight(pos + vec2(-bias, +bias)*0.5f);
            float h4 = getTerrainHeight(pos + vec2(-bias, -bias)*0.5f);


            // const float slope = max(abs(h0-h1), max(abs(h0-h2), max(abs(h0-h3), abs(h0 - h3) )));

            // float signedSlope = 0.25f*((h0-h1) + (h0-h2) + (h0-h3) + (h0-h4))/bias;
            float slope = 0.25f*(abs(h0-h1) + abs(h0-h2) + abs(h0-h3) + abs(h0-h4))/bias;

            

            if(i.spawnWeightsRange.Slope > 0.f)
            {
                float dist = smoothstep(1.f, 0.f, abs(i.spawnWeightsCenter.Slope - slope)/i.spawnWeightsRange.Slope);
                if(dist <= 0.f)
                {
                    count --;
                    continue;
                }
            }

            if(i.spawnWeightsRange.Elevation > 0.f)
            {
                float dist = smoothstep(1.f, 0.f, abs(i.spawnWeightsCenter.Elevation - h0/512.f)/i.spawnWeightsRange.Elevation);
                if(dist <= 0.f)
                {
                    count --;
                    continue;
                }
            }


            
            EntityRef e = spawnEntity(
                i.entities[name(generator)],
                vec3(pos.y, getTerrainHeight(pos), pos.x),
                quat(radians(vec3(radialx(generator),radialy(generator),radialz(generator))))
            );

            ComponentModularity::addChild(*Parent, e);

            if(e->has<EntityModel>())
                e->comp<EntityModel>()->state.scaleScalar(scale(generator)).update();

        }

        count += number;
    }

    return count;
}