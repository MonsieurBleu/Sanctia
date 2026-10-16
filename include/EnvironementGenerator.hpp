#pragma  once

#include <SanctiaEntity.hpp>

float getTerrainHeight(vec2 pos);
float getBiomeMap(vec2 pos, std::string name);

namespace GrassGenerator
{
    inline bool active = false;
    inline ObjectGroupRef models[4];

    void init();

    void update();
}

namespace WaterGenerator
{
    inline bool active = false;
    inline ObjectGroupRef model;
    inline EntityRef entity;

    inline float cellSize = 32.f;

    void init();

    EntityRef generate();

    void clear();
}

struct BiomeInfos
{
    float Grassyness = 0.f;
    float ForestDensity = 0.f;
    float Humidity = 0.f;
    float Mystic = 0.f;
    float Corruption = 0.f;
    float Slope = 0.f;
    float Elevation = 0.f;
    float WaterLevel = 0.f;

    float operator*(const BiomeInfos &other)
    {
        return
        (
            Grassyness*other.Grassyness
            +
            ForestDensity*other.ForestDensity
            +
            Humidity*other.Humidity
            +
            Mystic*other.Mystic
            +
            Corruption*other.Corruption
        )/(Grassyness + ForestDensity + Humidity + Mystic + Corruption);
    };

    void writeToFile(std::string filename);
};

class EntityScatterer
{
    private : 

        // std::random_device                  rand_dev;
        // std::mt19937                        generator;
        // std::uniform_int_distribution<int>  distx;
        // std::uniform_int_distribution<int>  disty;

        // std::vector<std::vector<vec3>> generatedPosition;

        bool genDone = true;
        vec2 genCurrentCell;
        vec2 genMin;
        vec2 genMax;
        BenchTimer genTimer;
        Entity *genParent = nullptr;
    
    public : 

        struct SpawnInfo
        {
            std::vector<std::string> entities;
            BiomeInfos spawnWeightsCenter;
            BiomeInfos spawnWeightsRange;
            BiomeInfos spawnWeightsTransition;
            vec3 radialRange = vec3(0);

            float densityPerCell = 1.f;

            float scaleRangeMin = 1.f;
            float scaleRangeMax = 1.f;
        
            void writeToFile(std::string filename);
        };

        vec2 rangeMin = vec2(2);
        vec2 rangeMax = vec2(1);

        float cellSize = 16.f;

        std::vector<SpawnInfo> spawns_old;

        std::vector<std::string> spawnsNames;

        EntityScatterer();

        EntityScatterer(
            vec2 rangeMin,
            vec2 rangeMax,
            const std::vector<SpawnInfo> &spawns
        );

        void generateInit(
            vec2 min, vec2 max, EntityRef parent
        );
        void generateCancel();
        void generateFrame(
            float timeAllowed
        );
        float generateGetProgress();
        float generateGetTime();
        
        void generateChunk(const vec2 cell, const BiomeInfos & local);

        int generate_old(
            vec2 center,
            BiomeInfos infos,
            int minNumber,
            int maxNumber,
            EntityRef Parent
        );

        static BiomeInfos getLocalInfos(vec2 center);
        static float evaluateDensity(const SpawnInfo &biome, const BiomeInfos &local);

        void writeToFile(std::string filename);
};

namespace WorldEnvironementGenerator
{
    // inline std::vector<EntityScatterer::SpawnInfo *> spawns;
    // inline std::unordered_map<std::string, EntityScatterer::SpawnInfo *> spawns;
    inline std::unordered_map<std::string, bool> spawns;

    inline EntityRef root;
    // inline std::vector<EntityRef> chunks; // can be replace by root->comp<EntityGroupInfos>().children

    inline vec2 worldMin = vec2(-2048.0);
    inline vec2 worldMax = vec2(+2048.0);

    inline ivec2 resolution = vec2(0);

    inline float chunkSize = 64.0;

    inline bool active = false;

    namespace multiFrameGen
    {
        inline ivec2 current;
        inline float timeLimitMS = 10.0;

        void refresh();
        void update();
        float completion();
    };

    void init();
    void clear();

    void generateChunk(ivec2 uv);

    void reset();

    void reGenerate(vec2 minp, vec2 maxp);
};  