#include <Subapps.hpp>
#include "App.hpp"
#include "Blueprint/EngineBlueprintUI.hpp"
#include "ECS/Entity.hpp"
#include "ECS/ModularEntityGroupping.hpp"
#include "GameGlobals.hpp"
#include "SanctiaLuaBindings.hpp"
#include <Scripting/ScriptInstance.hpp>
#include <AssetManager.hpp>

Apps::LuaTesting::LuaTesting() : SubApps("Lua Testing")
{
    inputs.push_back(&
        InputManager::addEventInput(
            "clear root", GLFW_KEY_SPACE, 0, GLFW_PRESS, [&]() {    
                appRoot = newEntity("AppRoot");
            },
            InputManager::Filters::always, false)
    );
    inputs.push_back(&
        InputManager::addEventInput(
            "execute script", GLFW_KEY_R, 0, GLFW_PRESS, [&]() {    
                appRoot->set<Script>(Script("test", ScriptHook::ON_INIT));
            },
            InputManager::Filters::always, false)
    );

    for(auto &i : inputs)
        i->activated = false;
};

EntityRef Apps::LuaTesting::UImenu()
{
    return newEntity("Lua Testing APP MENU"
        , UI_BASE_COMP
        , WidgetBox()
    );
}

void Apps::LuaTesting::init()
{
    /***** Preparing App Settings *****/
    {
        appRoot = newEntity("AppRoot");
        App::setController(&orbitController);
    }

    if (Loader<ScriptInstance>::loadingInfos.find("test_ent") != Loader<ScriptInstance>::loadingInfos.end())
    {
        Loader<ScriptInstance>::get("test_ent").run();
    }


    appRoot->set<Script>(Script("test", ScriptHook::ON_INIT));
}

void Apps::LuaTesting::update()
{
    ComponentModularity::synchronizeChildren(appRoot);

    /* 
        UPDATING ORBIT CONTROLLER ACTIVATION 
    */
    vec2 screenPos = globals.mousePosition();
    screenPos = (screenPos/vec2(globals.windowSize()))*2.f - 1.f;

    auto &box = EDITOR::MENUS::GameScreen->comp<WidgetBox>();
    vec2 cursor = ((screenPos-box.min)/(box.max - box.min));

    if(cursor.x < 0 || cursor.y < 0 || cursor.x > 1 || cursor.y > 1)
        globals.currentCamera->setMouseFollow(false);
    else
        globals.currentCamera->setMouseFollow(true);
}


void Apps::LuaTesting::clean()
{
    globals.simulationTime.pause();

    globals.currentCamera->setMouseFollow(false);
    globals.currentCamera->setPosition(vec3(0));
    globals.currentCamera->setDirection(vec3(-1, 0, 0));

    appRoot = EntityRef();
    App::setController(nullptr);

    GG::sun->shadowCameraSize = vec2(0, 0);
}

