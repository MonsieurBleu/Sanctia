#include <SanctiaEntity.hpp>
#include <Scripting/ScriptInstance.hpp>



Script::Script()
{
}

Script::Script(std::string scriptAssetName, ScriptHook hook) {
    scripts[hook].push_back(scriptAssetName);
}

#define RUN_FOR_HOOK(hook, ...) \
    for (std::string s : scripts[hook]) { \
        if (Loader<ScriptInstance>::loadingInfos.find(s) != Loader<ScriptInstance>::loadingInfos.end()) { \
            Loader<ScriptInstance>::get(s).run(__VA_ARGS__); \
        } else WARNING_MESSAGE("Can't find Script " << s << " on hook " << hook) \
    }

void Script::run_OnInit(Entity& self) {
    RUN_FOR_HOOK(ScriptHook::ON_INIT, self)
    initialized = true;
}

void Script::run_OnUpdate(Entity& self) {
    RUN_FOR_HOOK(ScriptHook::ON_UPDATE, self)
}

void Script::run_OnCollisionEnter(Entity& self, Entity& other, Entity& wearer) {
    RUN_FOR_HOOK(ScriptHook::ON_COLLISION_ENTER, self, other, wearer, false)
}

void Script::run_OnCollisionExit(Entity& self, Entity& other, Entity& wearer) {
    RUN_FOR_HOOK(ScriptHook::ON_COLLISION_EXIT, self, other, wearer, true)
}

void Script::run_OnAgentUpdate(Entity& self) {
    RUN_FOR_HOOK(ScriptHook::ON_AGENT_UPDATE, self)
}

void Script::addScript(std::string scriptAssetName, ScriptHook hook) {
    scripts[hook].push_back(scriptAssetName);
    if (initialized && hook == ScriptHook::ON_INIT) {
        if (Loader<ScriptInstance>::loadingInfos.find(scriptAssetName) != Loader<ScriptInstance>::loadingInfos.end()) {
            Loader<ScriptInstance>::get(scriptAssetName).run();
        }
    }
}