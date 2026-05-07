#include <EnvironementGenerator.hpp>
#include <AssetManagerUtils.hpp>

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

AUTOGEN_DATA_RW_FUNC_STRUCT(EntityScatterer::SpawnInfo, 
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

AUTOGEN_DATA_RW_FUNC_STRUCT(EntityScatterer,
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
    std::string fileName(filename + ".sBiomeInfos");
    NOTIF_MESSAGE("Creating file : " ,  fileName)
    auto out  = VulpineTextOutputRef(new VulpineTextOutput(1<<16));
    out->write("~", 1);
    out->Tabulate();
    DataLoader<BiomeInfos>::write(*this, out)->saveAs(fileName.c_str());
}

void EntityScatterer::writeToFile(std::string filename)
{
    std::string fileName(filename + ".sEntityScatterer");
    NOTIF_MESSAGE("Creating file : " ,  fileName)
    auto out  = VulpineTextOutputRef(new VulpineTextOutput(1<<16));
    out->write("~", 1);
    DataLoader<EntityScatterer>::write(*this, out)->saveAs(fileName.c_str());
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

float getTerrainHeight(vec2 pos)
{
    static auto &terrain = Loader<Texture2D>::get("Herault_4096");

    float *pixels = (float *)terrain.getPixelSource();

    ivec2 pixelPos = ivec2(round(2048.f + pos));

    const ivec2 res = terrain.getResolution();
    pixelPos = clamp(pixelPos, ivec2(0), res);

    return pixels[pixelPos.x*res.x + pixelPos.y]*512.f;
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


            std::default_random_engine generator(genCurrentCell.x + genCurrentCell.y*0.25f + genCurrentCell.x*genCurrentCell.y*0.5f);

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

                int numberToSpawn = round(spawn.densityPerCell*density);

                // if(numberToSpawn) ERROR_MESSAGE(spawn.densityPerCell*density)

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
                    

                    EntityRef e = spawnEntity(
                        spawn.entities[name(generator)],
                        vec3(pos.y, getTerrainHeight(pos), pos.x),
                        quat(radians(vec3(radialx(generator),radialy(generator),radialz(generator))))
                    );

                    // test.start();

                    ComponentModularity::addChild(*genParent, e);
                    // genParent->comp<EntityGroupInfo>().children.push_back(e);
                    // WARNING_MESSAGE(genParent->comp<EntityGroupInfo>().children.size())
                    
                    // test.stop();

                    if(e->has<EntityModel>())
                        e->comp<EntityModel>()->state.scaleScalar(scale(generator)).update();

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