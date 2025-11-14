#pragma once

#include "MappedEnum.hpp"
#include <vector>
#include "AssetManager.hpp"

GENERATE_ENUM_FAST_REVERSE(
    ScriptHook,
    ON_INIT,
    ON_UPDATE,
    ON_COLLISION_ENTER,
    ON_COLLISION_EXIT,
    ON_AGENT_UPDATE,
    HOOK_END
);

class Entity;
typedef std::shared_ptr<Entity> EntityRef;

class Script {
private:
    std::vector<std::string> scripts[ScriptHook::HOOK_END];
    bool initialized = false;
public:
    Script();
    Script(std::string scriptAssetName, ScriptHook hook);
    
    template <typename... Args>
    Script(std::string scriptAssetName, ScriptHook hook, Args... args)
    {
        addScript(scriptAssetName, hook);
        addScript(args...);
    }

    void run_OnInit(Entity& self);
    void run_OnUpdate(Entity& self);
    void run_OnCollisionEnter(Entity& self, Entity& other, Entity& wearer);
    void run_OnCollisionExit(Entity& self, Entity& other, Entity& wearer);
    void run_OnAgentUpdate(Entity& self);

    void addScript(std::string scriptAssetName, ScriptHook hook);

    bool isInitialized() const { return initialized; }
    void setInitialized(bool val) { initialized = val; }

    friend class DataLoader<Script>;
};