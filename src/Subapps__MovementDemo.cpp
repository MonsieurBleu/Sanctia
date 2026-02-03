#include "ComponentTypeLogic.hpp"
#include "Game.hpp"
#include <App.hpp>
#include "GameGlobals.hpp"
#include "Subapps.hpp"
#include <EntityBlueprint.hpp>

Apps::MovementDemo::MovementDemo() : SubApps("Movement Demo")
{
    inputs.push_back(&
        InputManager::addEventInput(
            "input exemple", GLFW_KEY_SPACE, 0, GLFW_PRESS, [&]() {
                
                // NOTIF_MESSAGE("SPACE BAR PRESSED")

            },
            InputManager::Filters::always, false)
    );    

    for(auto &i : inputs)
        i->activated = false;
};

EntityRef Apps::MovementDemo::UImenu()
{
    return newEntity("MovementDemo APP MENU"
        , UI_BASE_COMP
        , WidgetBox()
    );
}

void Apps::MovementDemo::init()
{
    /***** Preparing App Settings *****/
    {
        appRoot = newEntity("AppRoot");
        globals.currentCamera->getState().FOV = radians(100.f);
        globals.simulationTime.resume();

        appRoot->set<state3D>(true);
        GG::playerEntity = spawnEntity("(Combats) Player", vec3(-700, 50, -750)*1.f);
        ComponentModularity::addChild(*appRoot, GG::playerEntity);

        Game::playerControl = PlayerController(globals.currentCamera);
        App::setController(&Game::playerControl);

        Faction::setEnemy({Faction::Type::PLAYER}, {Faction::Type::PLAYER_ENEMY});
        Faction::setEnemy({Faction::Type::PLAYER}, {Faction::Type::MONSTERS});
        Faction::setEnemy({Faction::Type::MONSTERS}, {Faction::Type::PLAYER_ENEMY});
        
        GG::sun->shadowCameraSize = vec2(256, 256);

        
    }

    ComponentModularity::addChild(*appRoot, Blueprint::SpawnMainGameTerrain());

    physicsMutex.lock();
    ComponentModularity::addChild(*appRoot, spawnEntity("movement demo terrain"));
    ComponentModularity::ReparentChildren(*appRoot);
    physicsMutex.unlock();
}

void Apps::MovementDemo::update()
{
    
    
    // GG::draw->drawLine(vec3(-5), vec3(5));
    // GG::draw->drawSphere(vec3(sin(globals.appTime.getElapsedTime()), 0, 0), 3.0f);


    ComponentModularity::synchronizeChildren(appRoot);
}


void Apps::MovementDemo::clean()
{

    globals.simulationTime.pause();

    appRoot = EntityRef();
    GG::playerEntity = EntityRef();
    App::setController(nullptr);

    GG::sun->shadowCameraSize = vec2(0, 0);

    
}

