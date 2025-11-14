#include <App.hpp>
#include <Subapps.hpp>
#include <EntityBlueprint.hpp>
#include <Game.hpp>

#include <AnimationBlueprint.hpp>

void spawnPlayer(EntityRef appRoot)
{
    physicsMutex.lock();

    ComponentModularity::removeChild(*appRoot, GG::playerEntity);
    GG::playerEntity = EntityRef();
    GameGlobals::ManageEntityGarbage__WithPhysics();

    auto tmp = spawnEntity("(Combats) Player");
    GG::playerEntity = tmp; 

    ComponentModularity::addChild(*appRoot, tmp);
    ComponentModularity::ReparentChildren(*appRoot);

    physicsMutex.unlock();
}

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
        "Respawn", GLFW_KEY_ENTER, 0, GLFW_PRESS, [&]() {
            spawnPlayer(appRoot);

        },
        InputManager::Filters::always, false)
    );

    inputs.push_back(&
        InputManager::addEventInput(
        "attack", GLFW_MOUSE_BUTTON_LEFT, 0, GLFW_PRESS, [&]() {
            if (globals.currentCamera->getMouseFollow() && GG::playerEntity)
            {
                GG::playerEntity->comp<ActionState>().setStance(ActionState::Stance::RIGHT);
                GG::playerEntity->comp<ActionState>().isTryingToAttack = true;
            }
        },
        InputManager::Filters::always, false)
    );

    inputs.push_back(&
        InputManager::addEventInput(
        "kick", GLFW_MOUSE_BUTTON_MIDDLE, 0, GLFW_PRESS, [&]() {
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
        "sprint kick", GLFW_MOUSE_BUTTON_MIDDLE, GLFW_MOD_SHIFT, GLFW_PRESS, [&]() {
            if (globals.currentCamera->getMouseFollow() && GG::playerEntity)
            {
                GG::playerEntity->comp<ActionState>().setStance(ActionState::Stance::SPECIAL);
                GG::playerEntity->comp<ActionState>().isTryingToAttack = true;
            }
        },
        InputManager::Filters::always, false)
    );

    inputs.push_back(&
        InputManager::addContinuousInput(
            "block", 
            GLFW_MOUSE_BUTTON_RIGHT, 
            [&]() {
                GG::playerEntity->comp<ActionState>().isTryingToBlock = true;
            },
            InputManager::Filters::always,
            [&]() {
                GG::playerEntity->comp<ActionState>().isTryingToBlock = false;
            }
        )
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
        globals.currentCamera->getState().FOV = radians(85.f);
        globals.simulationTime.resume();

        // spawnPlayer(appRoot);
        GG::playerEntity = spawnEntity("(Combats) Player");
        // GG::playerEntity = spawnEntity("'Player");

        Game::playerControl = PlayerController(globals.currentCamera);
        App::setController(&Game::playerControl);

        Faction::setEnemy({Faction::Type::PLAYER}, {Faction::Type::PLAYER_ENEMY});
        Faction::setEnemy({Faction::Type::PLAYER}, {Faction::Type::MONSTERS});
        Faction::setEnemy({Faction::Type::MONSTERS}, {Faction::Type::PLAYER_ENEMY});
        
        GG::sun->shadowCameraSize = vec2(256, 256);
    }

    appRoot->set<EntityState3D>(true);
    appRoot->set<Script>(Script());
    appRoot->comp<Script>().addScript("World Update", ScriptHook::ON_UPDATE);

    for(float i = -1.f; i <= 1.f; i += 1.f)
    {
        EntityRef e= spawnEntity("(Combats) Enemy");
        e->comp<EntityState3D>().useinit = true;
        e->comp<EntityState3D>().initPosition = vec3(-4.f, 0, i*4.f);
        e->comp<Script>().addScript("Action-Dummy Update", ScriptHook::ON_UPDATE);
        ComponentModularity::addChild(*appRoot, e);
    }

    physicsMutex.lock();
    ComponentModularity::addChild(*appRoot, spawnEntity("Ground_Demo_64xh"));
    ComponentModularity::ReparentChildren(*appRoot);
    physicsMutex.unlock();

    /* Agent Test */
    // for(int i = 0; i < 50; i++)
    {
        EntityRef e= spawnEntity("(Combats) Ally");
        e->comp<EntityState3D>().useinit = true;
        e->comp<EntityState3D>().initPosition = vec3(4.f, 0, 0);
        e->comp<Script>().addScript("Agent Update Test", ScriptHook::ON_AGENT_UPDATE);
        e->set<AgentState>(AgentState());
        e->set<Target>((Target){GG::playerEntity});
        ComponentModularity::addChild(*appRoot, e);
    }


    // AnimationControllerRef test = AnimBlueprint::bipedMoveset_PREALPHA_2025("(Human) 2H Sword ", GG::playerEntity.get());
}

void Apps::CombatsApp::update()
{
    ComponentModularity::synchronizeChildren(appRoot);

    /* 
        UPDATING ORBIT CONTROLLER ACTIVATION 
    */

    // auto &tmp = GG::playerEntity->comp<Items>().equipped[WEAPON_SLOT].item->comp<EntityState3D>();
    // WARNING_MESSAGE(tmp.position << "\t" << tmp.quaternion)
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

