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
                
                NOTIF_MESSAGE("SPACE BAR PRESSED")

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

        GG::playerEntity = spawnEntity("(Combats) Player");

        Game::playerControl = PlayerController(globals.currentCamera);
        App::setController(&Game::playerControl);

        Faction::setEnemy({Faction::Type::PLAYER}, {Faction::Type::PLAYER_ENEMY});
        Faction::setEnemy({Faction::Type::PLAYER}, {Faction::Type::MONSTERS});
        Faction::setEnemy({Faction::Type::MONSTERS}, {Faction::Type::PLAYER_ENEMY});
        
        GG::sun->shadowCameraSize = vec2(256, 256);
    }

    physicsMutex.lock();
    ComponentModularity::addChild(*appRoot, spawnEntity("Ground_Demo_64xh"));
    ComponentModularity::ReparentChildren(*appRoot);
    physicsMutex.unlock();
}

void Apps::MovementDemo::update()
{
    ComponentModularity::synchronizeChildren(appRoot);

    // std::cout << "Player velocity: " << GG::playerEntity->comp<DeplacementState>().velocity << std::endl;
}


void Apps::MovementDemo::clean()
{
    globals.simulationTime.pause();

    appRoot = EntityRef();
    GG::playerEntity = EntityRef();
    App::setController(nullptr);

    GG::sun->shadowCameraSize = vec2(0, 0);
}

