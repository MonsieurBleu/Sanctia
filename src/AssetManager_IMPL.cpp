#include <SanctiaEntity.hpp>
#include <EnvironementGenerator.hpp>

#include <ModManager.hpp>

const std::vector<std::pair<
        std::pair<const std::string, const std::string>,
        std::pair<bool, const std::string>
    >> gameFiletypes = 
    {
        {{"sEntityScatterer", "EntityScatterer"}, {false, ""}},
        {{"sEntitySpawn", "EntityScatterer::SpawnInfo"}, {false, ""}},
    };

#define GAME_SPECIFIC_TYPES_LOAD \
    LOAD_ALL(EntityScatterer) \
    LOAD_ALL(EntityScatterer::SpawnInfo)

#include <AssetManager_IMPL.hpp>
