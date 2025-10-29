#include <App.hpp>
#include <Subapps.hpp>
#include <EntityBlueprint.hpp>
#include <Game.hpp>

#include <AnimationBlueprint.hpp>

Apps::CombatsApp::CombatsApp() : SubApps("Combats")
{
    inputs.push_back(&
        InputManager::addEventInput(
        "die", GLFW_KEY_BACKSPACE, 0, GLFW_PRESS, [&]() {
            if(!GG::playerEntity) return;
            GG::playerEntity->comp<EntityStats>().alive = false;
        },
        []() {return globals.currentCamera->getMouseFollow();}, false)
    );


    inputs.push_back(&
        InputManager::addEventInput(
        "attack", GLFW_MOUSE_BUTTON_LEFT, 0, GLFW_PRESS, [&]() {
            if (globals.currentCamera->getMouseFollow() && GG::playerEntity)
                GG::playerEntity->comp<ActionState>().isTryingToAttack = true;
        },
        InputManager::Filters::always, false)
    );

    inputs.push_back(&
        InputManager::addEventInput(
        "attack", GLFW_MOUSE_BUTTON_MIDDLE, 0, GLFW_PRESS, [&]() {
            if (globals.currentCamera->getMouseFollow() && GG::playerEntity)
            {
                GG::playerEntity->comp<ActionState>().setStance(ActionState::Stance::SPECIAL);
                GG::playerEntity->comp<ActionState>().isTryingToAttack = true;
            }
        },
        InputManager::Filters::always, false)
    );

    inputs.push_back(&
        InputManager::addEventInput(
        "attack", GLFW_MOUSE_BUTTON_MIDDLE, GLFW_MOD_SHIFT, GLFW_PRESS, [&]() {
            if (globals.currentCamera->getMouseFollow() && GG::playerEntity)
            {
                GG::playerEntity->comp<ActionState>().setStance(ActionState::Stance::SPECIAL);
                GG::playerEntity->comp<ActionState>().isTryingToAttack = true;
            }
        },
        InputManager::Filters::always, false)
    );

    inputs.push_back(&
        InputManager::addEventInput(
        "attack", GLFW_MOUSE_BUTTON_RIGHT, 0, GLFW_PRESS, [&]() {
            if (globals.currentCamera->getMouseFollow() && GG::playerEntity)
            {
                GG::playerEntity->comp<ActionState>().isTryingToBlock ^= true;
                NOTIF_MESSAGE((int)GG::playerEntity->comp<ActionState>().isTryingToBlock)
            }
        },
        InputManager::Filters::always, false)
    );

    for(auto &i : inputs)
        i->activated = false;
};

EntityRef Apps::CombatsApp::UImenu()
{
    return newEntity("Combats APP MENU"
        , UI_BASE_COMP
        , WidgetBox()
    );
}

void Apps::CombatsApp::init()
{
    /***** Preparing App Settings *****/
    {
        appRoot = newEntity("AppRoot");
        globals.currentCamera->getState().FOV = radians(100.f);
        globals.simulationTime.resume();

        GG::playerEntity = spawnEntity("(Combats) Player");
        // GG::playerEntity = spawnEntity("'Player");

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

    AnimationControllerRef test = AnimBlueprint::bipedMoveset_PREALPHA_2025("(Human) 2H Sword ", GG::playerEntity.get());
}

void Apps::CombatsApp::update()
{
    ComponentModularity::synchronizeChildren(appRoot);

    /* 
        UPDATING ORBIT CONTROLLER ACTIVATION 
    */
}


void Apps::CombatsApp::clean()
{
    globals.simulationTime.pause();

    Faction::clearRelations();

    appRoot = EntityRef();
    GG::playerEntity = EntityRef();
    App::setController(nullptr);

    GG::sun->shadowCameraSize = vec2(0, 0);
}

