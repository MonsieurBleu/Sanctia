#include <App.hpp>
#include <GLFW/glfw3.h>
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

void PlayerAttack()
{
    if (globals.currentCamera->getMouseFollow() && GG::playerEntity && GG::playerEntity->has<ActionState>())
    {
        auto &sa = GG::playerEntity->comp<ActionState>();

        if(!sa.isTryingToAttack && !(sa.attacking && sa._stance == ActionState::Stance::RIGHT) && (!sa.blocking || sa.hasBlockedAttack))
        {
            sa.setStance(ActionState::Stance::RIGHT);
            sa.isTryingToAttack = true;
            sa.isTryingToAttackTime = globals.simulationTime.getElapsedTime();
        }
    }
}

void PlayerBlock()
{
    if (globals.currentCamera->getMouseFollow() && GG::playerEntity && GG::playerEntity->has<ActionState>())
    {
        auto &sa = GG::playerEntity->comp<ActionState>();

        if(!sa.isTryingToBlock)
        {
            sa.setStance(ActionState::Stance::RIGHT);
            sa.isTryingToBlock = true;
            sa.isTryingToBlockTime= globals.simulationTime.getElapsedTime();
        }
    }
}

void PlayerStun()
{
    if (globals.currentCamera->getMouseFollow() && GG::playerEntity && GG::playerEntity->has<ActionState>())
    {
        auto &sa = GG::playerEntity->comp<ActionState>();

        if(!sa.isTryingToAttack && !(sa.attacking && sa._stance == ActionState::Stance::SPECIAL))
        {
            sa.isTryingToAttackTime = globals.simulationTime.getElapsedTime();

            sa.setStance(ActionState::Stance::SPECIAL);
            sa.isTryingToAttack = true;
            
        }
    }
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
        "attack", GLFW_MOUSE_BUTTON_LEFT, 0, GLFW_PRESS, PlayerAttack,
        InputManager::Filters::always, false)
    );

    inputs.push_back(&
        InputManager::addEventInput(
        "attack", VULPINE_GAMEPAD_BUTTON_X, 0, GLFW_PRESS, PlayerAttack,
        InputManager::Filters::always, false)
    );

    inputs.push_back(&
        InputManager::addEventInput(
        "kick", GLFW_MOUSE_BUTTON_MIDDLE, 0, GLFW_PRESS, PlayerStun,
        InputManager::Filters::always, false)
    );

    inputs.push_back(&
        InputManager::addEventInput(
        "kick", VULPINE_GAMEPAD_BUTTON_Y, 0, GLFW_PRESS, PlayerStun,
        InputManager::Filters::always, false)
    );

    // duplicating kick input on mouse 4 cause my middle mouse doesn't work :(
    inputs.push_back(&
        InputManager::addEventInput(
        "kick", GLFW_MOUSE_BUTTON_4, 0, GLFW_PRESS, PlayerStun,
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
            PlayerBlock,
            InputManager::Filters::always,
            [&]() {
                if (InputManager::getGamepadAxisValue(GLFW_GAMEPAD_AXIS_LEFT_TRIGGER) > 0.1f)
                {
                    PlayerBlock();
                }
                else {
                    GG::playerEntity->comp<ActionState>().isTryingToBlock = false;
                }
            }
        )
    );
    
    inputs.push_back(&
        InputManager::addEventInput(
            "Double Speed", GLFW_KEY_KP_ADD, 0, GLFW_PRESS, [&]() {
                
                globals.simulationTime.speed *= 2.;

            },
            InputManager::Filters::always, false)
    );    

    inputs.push_back(&
        InputManager::addEventInput(
            "Halfen Speed", GLFW_KEY_KP_SUBTRACT, 0, GLFW_PRESS, [&]() {
                
                globals.simulationTime.speed /= 2.;

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
        
        GG::sun->shadowCameraSize = vec2(64, 64);
        GG::sun->activateShadows();
    }

    appRoot->set<state3D>(true);
    appRoot->set<Script>(Script());
    appRoot->comp<Script>().addScript("World Update", ScriptHook::ON_UPDATE);

    // if(false)
    for(float i = -1.f; i <= 1.f; i += 1.f)
    {
        EntityRef e= spawnEntity("(Combats) Enemy");
        e->comp<state3D>().useinit = true;
        e->comp<state3D>().initPosition = vec3(8.f, 0, i*4.f);
        e->comp<Script>().addScript("Action-Dummy Update", ScriptHook::ON_UPDATE);
        ComponentModularity::addChild(*appRoot, e);
    }

    physicsMutex.lock();
    ComponentModularity::addChild(*appRoot, spawnEntity("Ground_Demo_64xh"));
    ComponentModularity::ReparentChildren(*appRoot);
    physicsMutex.unlock();

    /* Agent Test */
    for(int i = 0; i < 1; i++)
    {
        EntityRef e= spawnEntity("(Combats) Ally");
        e->comp<state3D>().useinit = true;
        e->comp<state3D>().initPosition = vec3(-8.f, 0, 0);
        e->comp<Script>().addScript("Agent Update Test", ScriptHook::ON_AGENT_UPDATE);
        e->set<AgentState>(AgentState());
        e->set<Target>((Target){GG::playerEntity.get()});
        ComponentModularity::addChild(*appRoot, e);
    }

    /* Agent to Agent Combat*/
    for(int i = 0; i < 0; i ++)
    {
        EntityRef e = spawnEntity("(Combats) Ally");
        e->comp<state3D>().useinit = true;
        e->comp<state3D>().initPosition = vec3(rand()%16-8, 0, rand()%24-12);
        e->comp<Script>().addScript("Agent Update Test", ScriptHook::ON_AGENT_UPDATE);
        e->set<AgentState>(AgentState());
        ComponentModularity::addChild(*appRoot, e);

        EntityRef e2 = spawnEntity("(Combats) Enemy");
        e2->comp<state3D>().useinit = true;
        e2->comp<state3D>().initPosition = vec3(rand()%16-8, 0, rand()%24-12);
        e2->comp<Script>().addScript("Agent Update Test", ScriptHook::ON_AGENT_UPDATE);
        e2->set<AgentState>(AgentState());
        ComponentModularity::addChild(*appRoot, e2);


        e->set<Target>((Target){e2.get()});
        e2->set<Target>((Target){e.get()});
    }

    // AnimationControllerRef test = AnimBlueprint::bipedMoveset_PREALPHA_2025("(Human) 2H Sword ", GG::playerEntity.get());
}

void Apps::CombatsApp::update()
{
    ComponentModularity::synchronizeChildren(appRoot);

    /* 
        UPDATING ORBIT CONTROLLER ACTIVATION 
    */

    // auto &tmp = GG::playerEntity->comp<Items>().equipped[WEAPON_SLOT].item->comp<state3D>();
    // WARNING_MESSAGE(tmp.position << "\t" << tmp.quaternion)
}


void Apps::CombatsApp::clean()
{
    globals.simulationTime.pause();

    Faction::clearRelations();

    appRoot = EntityRef();
    GG::playerEntity = EntityRef();
    App::setController(nullptr);

    physicsMutex.lock();
    GG::ManageEntityGarbage__WithPhysics();
    physicsMutex.unlock();

    GG::sun->shadowCameraSize = vec2(0, 0);
}

