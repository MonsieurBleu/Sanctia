#include <App.hpp>
#include <GLFW/glfw3.h>
#include <Subapps.hpp>
#include <EntityBlueprint.hpp>
#include <Game.hpp>

#include <AnimationBlueprint.hpp>

#define Y_SPAWN_LEVEL 38

#define PLAYER_SET "(Combats) Player 2"

void spawnPlayer(EntityRef appRoot)
{
    physicsMutex.lock();

    // ComponentModularity::removeChild(*appRoot, GG::playerEntity);
    appRoot->comp<EntityGroupInfo>().children.clear();
    GG::playerEntity = EntityRef();  
    GameGlobals::ManageEntityGarbage__WithPhysics();

    auto tmp = spawnEntity(PLAYER_SET, vec3(16, Y_SPAWN_LEVEL, 0));
    GG::playerEntity = tmp; 

    ComponentModularity::addChild(*appRoot, tmp);
    ComponentModularity::ReparentChildren(*appRoot);

    physicsMutex.unlock();
}

void Apps::CombatsApp::addStage(const std::string &name, vec4 newStageColor, std::function<void()> spawnScript)
{
    EntityRef newStageButton;

    ComponentModularity::addChild(*buttonsZone,
        newStageButton = VulpineBlueprintUI::Toggable(
            name, 
            "", 
            [&, spawnScript](Entity *e, float f)
            {
                globals.currentCamera->setMouseFollow(true);
                spawnPlayer(stageNPC);
                spawnScript();
            }, 
            [](Entity *e){return e->comp<WidgetBox>().isUnderCursor ? 1.f : 0.f;}
        )
    );

    newStageButton->comp<WidgetStyle>()
        .setbackgroundColor1(newStageColor)
        .setbackgroundColor2(newStageColor*0.9f)
        .settextColor1(vec4(0.05, 0.05, 0.05, 0.95))
        .settextColor2(vec4(0.05, 0.05, 0.05, 0.95))
    ;
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
            spawnPlayer(stageNPC);

        },
        []() {return globals.currentCamera->getMouseFollow();}, false)
    );

    inputs.push_back(&
        InputManager::addEventInput(
        "attack", GLFW_MOUSE_BUTTON_LEFT, 0, GLFW_PRESS, PlayerAttack,
        []() {return globals.currentCamera->getMouseFollow();}, false)
    );

    inputs.push_back(&
        InputManager::addEventInput(
        "attack", VULPINE_GAMEPAD_BUTTON_X, 0, GLFW_PRESS, PlayerAttack,
        []() {return globals.currentCamera->getMouseFollow();}, false)
    );

    inputs.push_back(&
        InputManager::addEventInput(
        "kick", GLFW_MOUSE_BUTTON_MIDDLE, 0, GLFW_PRESS, PlayerStun,
        []() {return globals.currentCamera->getMouseFollow();}, false)
    );

    inputs.push_back(&
        InputManager::addEventInput(
        "kick", VULPINE_GAMEPAD_BUTTON_Y, 0, GLFW_PRESS, PlayerStun,
        []() {return globals.currentCamera->getMouseFollow();}, false)
    );

    // duplicating kick input on mouse 4 cause my middle mouse doesn't work :(
    inputs.push_back(&
        InputManager::addEventInput(
        "kick", GLFW_MOUSE_BUTTON_4, 0, GLFW_PRESS, PlayerStun,
        []() {return globals.currentCamera->getMouseFollow();}, false)
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
        []() {return globals.currentCamera->getMouseFollow();}, false)
    );

    inputs.push_back(&
        InputManager::addContinuousInput(
            "block", 
            GLFW_MOUSE_BUTTON_RIGHT, 
            PlayerBlock,
            []() {return globals.currentCamera->getMouseFollow();},
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

    inputs.push_back(&
        InputManager::addEventInput(
        "toggle free cam", GLFW_KEY_F12, 0, GLFW_PRESS, [&]() { 
            if (globals.getController() == &Game::playerControl && GG::playerEntity)
            {
                GG::playerEntity->comp<EntityModel>()->state.hide = ModelStatus::HIDE;

                for (auto &i : GG::playerEntity->comp<Items>().equipped)
                    if (i.item.get() && i.item->has<EntityModel>())
                        i.item->comp<EntityModel>()->state.hide = ModelStatus::HIDE;
                App::setController(&Game::spectator);
            }
            else if (globals.getController() == &Game::spectator && GG::playerEntity)
            {
                App::setController(&Game::playerControl);
                // playerControl.body->position = globals.currentCamera->getPosition();

                if (GG::playerEntity->has<RigidBody>())
                {
                    auto body = GG::playerEntity->comp<RigidBody>();
                    if (body)
                    {
                        body->setIsActive(true);
                        if(GG::playerEntity->has<staticEntityFlag>()) GG::playerEntity->comp<staticEntityFlag>().shoudBeActive = body->isActive();
                        body->setTransform(rp3d::Transform(PG::torp3d(globals.currentCamera->getPosition() + vec3(0, 5, 0)),
                                                           rp3d::Quaternion::identity()));
                    }
                }

                GG::playerEntity->comp<EntityModel>()->state.hide = ModelStatus::SHOW;
                for (auto &i : GG::playerEntity->comp<Items>().equipped)
                    if (i.item.get() && i.item->has<EntityModel>())
                        i.item->comp<EntityModel>()->state.hide = ModelStatus::SHOW;
            }
        })
    );

    Inputs::toggleFreeMouse = InputManager::addEventInput(
        "Gamepad Cursor Mode Toggle", VULPINE_GAMEPAD_BUTTON_START, 0, GLFW_PRESS, [&]() { globals.currentCamera->toggleMouseFollow(); },
        InputManager::Filters::always, false);

    Inputs::toggleFreeMouse = InputManager::addEventInput(
        "Gamepad Cursor Select", VULPINE_GAMEPAD_BUTTON_A, 0, GLFW_PRESS, [&]() 
        {
            giveCallbackToApp(GLFWKeyInfo{
                globals.getWindow(), 
                GLFW_MOUSE_BUTTON_LEFT, 
                GLFW_MOUSE_BUTTON_LEFT, 
                GLFW_PRESS, 
                0
            });
        },
        [](){return !globals.currentCamera->getMouseFollow();}, false);

    Inputs::toggleFreeMouse = InputManager::addEventInput(
        "Gamepad Cursor Select", VULPINE_GAMEPAD_BUTTON_A, 0, GLFW_RELEASE, [&]() 
        {
            giveCallbackToApp(GLFWKeyInfo{
                globals.getWindow(), 
                GLFW_MOUSE_BUTTON_LEFT, 
                GLFW_MOUSE_BUTTON_LEFT, 
                GLFW_RELEASE, 
                0
            });
        },
        [](){return !globals.currentCamera->getMouseFollow();}, false);


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

        stageNPC = newEntity("Stage NPC", state3D(true));
        ComponentModularity::addChild(*appRoot, stageNPC);

        // spawnPlayer(appRoot);
        GG::playerEntity = spawnEntity(PLAYER_SET);
        GG::playerEntity->comp<state3D>().useinit = true;
        GG::playerEntity->comp<state3D>().initPosition = vec3(16, Y_SPAWN_LEVEL, 0);
        ComponentModularity::addChild(*stageNPC, GG::playerEntity);

        // GG::playerEntity->set<AgentProfile>(spawnEntity("Combat Profile Weak")->comp<AgentProfile>());

        // GG::playerEntity->set<AgentProfile>(AgentProfile());
        // GG::playerEntity->comp<AgentProfile>() + spawnEntity("Combat Profile Weak")->comp<AgentProfile>();

        // GG::playerEntity->comp<state3D>().initPosition = vec3(0, 0, -16);
        // GG::playerEntity = spawnEntity("'Player");

        Game::playerControl = PlayerController(globals.currentCamera);
        App::setController(&Game::playerControl);

        Faction::setEnemy({Faction::Type::PLAYER}, {Faction::Type::PLAYER_ENEMY});
        Faction::setEnemy({Faction::Type::PLAYER}, {Faction::Type::PLAYER_ENEMY2});

        Faction::setEnemy({Faction::Type::PLAYER}, {Faction::Type::MONSTERS});
        Faction::setEnemy({Faction::Type::MONSTERS}, {Faction::Type::PLAYER_ENEMY});

        Faction::setEnemy({Faction::Type::PLAYER_ENEMY2}, {Faction::Type::PLAYER_ENEMY});
        
        Faction::setEnemy({Faction::Type::TEST2}, {Faction::Type::TEST1});

        GG::sun->shadowCameraSize = vec2(64, 64);
        GG::sun->activateShadows();

        globals.currentCamera->setMouseFollow(false);

        GG::draw = std::make_shared<Draw>(appRoot);
    }



    ComponentModularity::addChild(*appRoot, Blueprint::SpawnMainGameTerrain());

    appRoot->set<state3D>(true);
    appRoot->set<Script>(Script());
    appRoot->comp<Script>().addScript("World Update", ScriptHook::ON_UPDATE);

    // if(false)

    physicsMutex.lock();
    ComponentModularity::addChild(*appRoot, spawnEntity("Ground_Demo_64xh", vec3(0, Y_SPAWN_LEVEL, 0)));
    ComponentModularity::ReparentChildren(*appRoot);
    physicsMutex.unlock();

    /* Agent Test */
    for(int i = 0; i < 0; i++)
    {
        EntityRef e= spawnEntity("(Combats) Ally", vec3(-8.f, Y_SPAWN_LEVEL, 0));
        e->comp<Script>().addScript("Agent Update Test", ScriptHook::ON_AGENT_UPDATE);
        e->set<AgentState>(AgentState());
        e->set<Target>((Target){GG::playerEntity.get()});
        e->set<Faction>({Faction::PLAYER_ENEMY});
        ComponentModularity::addChild(*appRoot, e);
    }

    /* Agent to Agent Combat*/
    for(int i = 0; i < 0; i ++)
    {
        // EntityRef e = spawnEntity("(Combats) Ally");
        EntityRef e = spawnEntity(PLAYER_SET, vec3(16 + rand()%16-8, Y_SPAWN_LEVEL, rand()%24-12));
        e->comp<Script>().addScript("Agent Update Test", ScriptHook::ON_AGENT_UPDATE);
        e->set<AgentState>(AgentState());
        // e->set<Faction>({Faction::TEST1});
        e->set<Faction>({Faction::PLAYER});
        e->set<Target>((Target){});
        ComponentModularity::addChild(*appRoot, e);
    }

    for(int i = 0; i < 0; i ++)
    {
        EntityRef e2 = spawnEntity("(Combats) Enemy", vec3(- 16 + rand()%16-8, Y_SPAWN_LEVEL, rand()%24-12));

        e2->comp<Script>().addScript("Agent Update Test", ScriptHook::ON_AGENT_UPDATE);
        e2->set<AgentState>(AgentState());
        // e2->set<Faction>({Faction::TEST2});
        e2->set<Faction>({Faction::PLAYER_ENEMY});
        e2->set<Target>((Target){});
        ComponentModularity::addChild(*appRoot, e2);
    }



    stageSelectionScreen = newEntity("Stage Selection Screen"
        , UI_BASE_COMP
        , WidgetBackground()
        , WidgetStyle()
            .setbackGroundStyle(UiTileType::SQUARE)
            .setbackgroundColor1(vec4(0.05, 0.05, 0.05, 0.5))
    );
    stageSelectionScreen->comp<WidgetBox>().useClassicInterpolation = true;
    ComponentModularity::addChild(*EDITOR::MENUS::GameScreen, stageSelectionScreen);

    buttonsZone = newEntity("Stage Selection Buttons Zone"
        , UI_BASE_COMP
        , WidgetBox(vec2(-0.5, 0.5), vec2(-0.9, 0.9))
        , WidgetStyle()
            .setautomaticTabbing(12)
    );
    ComponentModularity::addChild(*stageSelectionScreen, buttonsZone);


    // EntityRef newStageButton;
    // vec4 newStageColor = vec4(1., 0.5, 0.5, 1.0);
    // std::function<void()> spawnScript = [&]()
    // {
    //     for(float i = -1.f; i <= 1.f; i += 1.f)
    //     {
    //         EntityRef e = spawnEntity("(Combats) Enemy");
    //         e->comp<state3D>().useinit = true;
    //         e->comp<state3D>().initPosition = vec3(8.f, Y_SPAWN_LEVEL, i*4.f);
    //         e->comp<Script>().addScript("Action-Dummy Update", ScriptHook::ON_UPDATE);
    //         e->set<Faction>({Faction::PLAYER_ENEMY});
    //         ComponentModularity::addChild(*stageNPC, e);
    //     }
    // };

    // ComponentModularity::addChild(*buttonsZone,
    //     newStageButton = VulpineBlueprintUI::Toggable(
    //         "Training Dummies", 
    //         "", 
    //         [&, spawnScript](Entity *e, float f)
    //         {
    //             globals.currentCamera->setMouseFollow(true);
    //             spawnPlayer(stageNPC);
    //             spawnScript();
    //         }, 
    //         [](Entity *e){return e->comp<WidgetBox>().isUnderCursor ? 1.f : 0.f;}
    //     )
    // );

    // newStageButton->comp<WidgetStyle>()
    //     .setbackgroundColor1(newStageColor)
    //     .setbackgroundColor2(newStageColor*0.9f)
    //     .settextColor1(vec4(0.05, 0.05, 0.05, 0.95))
    //     .settextColor2(vec4(0.05, 0.05, 0.05, 0.95))
    // ;

    addStage("Martial Training Dummies",
        vec4(1., 0.5, 0.5, 1.0),
        [&]()
        {
            for(float i = -1.f; i <= 1.f; i += 1.f)
            {
                EntityRef e = spawnEntity("(Combats) Enemy");
                e->comp<state3D>().useinit = true;
                e->comp<state3D>().initPosition = vec3(0.f, Y_SPAWN_LEVEL, i*4.f);
                e->comp<Script>().addScript("Action-Dummy Update", ScriptHook::ON_UPDATE);
                e->set<Faction>({Faction::PLAYER_ENEMY});
                ComponentModularity::addChild(*stageNPC, e);
            }

            for(float i = -1.f; i <= 1.f; i += 1.f)
            {
                EntityRef e = spawnEntity("(Combats) Enemy 2");
                e->comp<state3D>().useinit = true;
                e->comp<state3D>().initPosition = vec3(8.f, Y_SPAWN_LEVEL, i*4.f);
                e->comp<Script>().addScript("Action-Dummy Update", ScriptHook::ON_UPDATE);
                e->set<Faction>({Faction::PLAYER_ENEMY});
                ComponentModularity::addChild(*stageNPC, e);
            }
        }
    );

    addStage("1v1 Weak Enemy - Zweihander",
        vec4(1., 0.5, 0.5, 1.0),
        [&]()
        {
            EntityRef e= spawnEntity("(Combats) Ally", vec3(-8.f, Y_SPAWN_LEVEL, 0));
            e->comp<Script>().addScript("Agent Update Test", ScriptHook::ON_AGENT_UPDATE);
            e->set<AgentState>(AgentState());
            e->set<Target>((Target){GG::playerEntity.get()});
            e->set<Faction>({Faction::PLAYER_ENEMY});
            e->set<AgentProfile>(AgentProfile());
            e->comp<AgentProfile>() 
                + spawnEntity("Combat Profile TEMPLATE")->comp<AgentProfile>()
                + spawnEntity("Combat Profile Tactitcs Beginner")->comp<AgentProfile>()
                + spawnEntity("Combat Profile Initiative Passive")->comp<AgentProfile>()
            ;
            ComponentModularity::addChild(*stageNPC, e);
        }
    );

    addStage("1v1 Weak Enemy - Sword And Shield",
        vec4(1., 0.5, 0.5, 1.0),
        [&]()
        {
            EntityRef e= spawnEntity("(Combats) Ally 2", vec3(-8.f, Y_SPAWN_LEVEL, 0));
            e->comp<Script>().addScript("Agent Update Test", ScriptHook::ON_AGENT_UPDATE);
            e->set<AgentState>(AgentState());
            e->set<Target>((Target){GG::playerEntity.get()});
            e->set<Faction>({Faction::PLAYER_ENEMY});
            e->set<AgentProfile>(AgentProfile());
            e->comp<AgentProfile>() 
                + spawnEntity("Combat Profile TEMPLATE")->comp<AgentProfile>()
                + spawnEntity("Combat Profile Tactitcs Beginner")->comp<AgentProfile>()
                + spawnEntity("Combat Profile Initiative Passive")->comp<AgentProfile>()
            ;
            ComponentModularity::addChild(*stageNPC, e);
        }
    );

    // addStage("1v1 Weak Enemy - Shield",
    //     vec4(1., 0.5, 0.5, 1.0) * 0.8f,
    //     [&]()
    //     {

    //     }
    // );


    addStage("1v1 Strong Enemy - Zweihander",
        vec4(1., 0.5, 0.5, 1.0),
        [&]()
        {
            EntityRef e= spawnEntity("(Combats) Ally", vec3(-8.f, Y_SPAWN_LEVEL, 0));
            e->comp<Script>().addScript("Agent Update Test", ScriptHook::ON_AGENT_UPDATE);
            e->set<AgentState>(AgentState());
            e->set<Target>((Target){GG::playerEntity.get()});
            e->set<Faction>({Faction::PLAYER_ENEMY});
            e->set<AgentProfile>(AgentProfile());
            e->comp<AgentProfile>() 
                + spawnEntity("Combat Profile TEMPLATE")->comp<AgentProfile>()
                + spawnEntity("Combat Profile Tactitcs Trained")->comp<AgentProfile>()
                + spawnEntity("Combat Profile Initiative Aggressive")->comp<AgentProfile>()
            ;
            ComponentModularity::addChild(*stageNPC, e);
        }
    );

    addStage("1v1 Strong Enemy - Sword And Shield",
        vec4(1., 0.5, 0.5, 1.0),
        [&]()
        {
            EntityRef e= spawnEntity("(Combats) Ally 2", vec3(-8.f, Y_SPAWN_LEVEL, 0));
            e->comp<Script>().addScript("Agent Update Test", ScriptHook::ON_AGENT_UPDATE);
            e->set<AgentState>(AgentState());
            e->set<Target>((Target){GG::playerEntity.get()});
            e->set<Faction>({Faction::PLAYER_ENEMY});
            e->set<AgentProfile>(AgentProfile());
            e->comp<AgentProfile>() 
                + spawnEntity("Combat Profile TEMPLATE")->comp<AgentProfile>()
                + spawnEntity("Combat Profile Tactitcs Trained")->comp<AgentProfile>()
                + spawnEntity("Combat Profile Initiative Aggressive")->comp<AgentProfile>()
            ;
            ComponentModularity::addChild(*stageNPC, e);
        }
    );

    addStage("1v1 Very Strong Enemy - Zweihander",
        vec4(1., 0.5, 0.5, 1.0),
        [&]()
        {
            EntityRef e= spawnEntity("(Combats) Ally", vec3(-8.f, Y_SPAWN_LEVEL, 0));
            e->comp<Script>().addScript("Agent Update Test", ScriptHook::ON_AGENT_UPDATE);
            e->set<AgentState>(AgentState());
            e->set<Target>((Target){GG::playerEntity.get()});
            e->set<Faction>({Faction::PLAYER_ENEMY});
            e->set<AgentProfile>(AgentProfile());
            e->comp<AgentProfile>() 
                + spawnEntity("Combat Profile TEMPLATE")->comp<AgentProfile>()
                + spawnEntity("Combat Profile Tactitcs God Duelist")->comp<AgentProfile>()
                + spawnEntity("Combat Profile Initiative Passive")->comp<AgentProfile>()
            ;
            ComponentModularity::addChild(*stageNPC, e);
        }
    );

    addStage("1v1 Very Strong Enemy - Sword And Shield",
        vec4(1., 0.5, 0.5, 1.0),
        [&]()
        {
            EntityRef e= spawnEntity("(Combats) Ally 2", vec3(-8.f, Y_SPAWN_LEVEL, 0));
            e->comp<Script>().addScript("Agent Update Test", ScriptHook::ON_AGENT_UPDATE);
            e->set<AgentState>(AgentState());
            e->set<Target>((Target){GG::playerEntity.get()});
            e->set<Faction>({Faction::PLAYER_ENEMY});
            e->set<AgentProfile>(AgentProfile());
            e->comp<AgentProfile>() 
                + spawnEntity("Combat Profile TEMPLATE")->comp<AgentProfile>()
                + spawnEntity("Combat Profile Tactitcs God Duelist")->comp<AgentProfile>()
                + spawnEntity("Combat Profile Initiative Passive")->comp<AgentProfile>()
            ;
            ComponentModularity::addChild(*stageNPC, e);
        }
    );

    addStage("1v2 Weak Enemies",
        vec4(1., 0.5, 0.5, 1.0),
        [&]()
        {
            {
                EntityRef e= spawnEntity("(Combats) Ally", vec3(-8.f, Y_SPAWN_LEVEL, 0));
                e->comp<Script>().addScript("Agent Update Test", ScriptHook::ON_AGENT_UPDATE);
                e->set<AgentState>(AgentState());
                e->set<Target>((Target){GG::playerEntity.get()});
                e->set<Faction>({Faction::PLAYER_ENEMY});
                e->set<AgentProfile>(AgentProfile());
                e->comp<AgentProfile>() 
                    + spawnEntity("Combat Profile TEMPLATE")->comp<AgentProfile>()
                    + spawnEntity("Combat Profile Tactitcs Beginner")->comp<AgentProfile>()
                    + spawnEntity("Combat Profile Initiative Passive")->comp<AgentProfile>()
                ;
                ComponentModularity::addChild(*stageNPC, e);
            }
            {
                EntityRef e = spawnEntity("(Combats) Ally 2", vec3(-8.f, Y_SPAWN_LEVEL, 0));
                e->comp<Script>().addScript("Agent Update Test", ScriptHook::ON_AGENT_UPDATE);
                e->set<AgentState>(AgentState());
                e->set<Target>((Target){GG::playerEntity.get()});
                e->set<Faction>({Faction::PLAYER_ENEMY});
                e->set<AgentProfile>(AgentProfile());
                e->comp<AgentProfile>() 
                    + spawnEntity("Combat Profile TEMPLATE")->comp<AgentProfile>()
                    + spawnEntity("Combat Profile Tactitcs Beginner")->comp<AgentProfile>()
                    + spawnEntity("Combat Profile Initiative Passive")->comp<AgentProfile>()
                ;
                ComponentModularity::addChild(*stageNPC, e);
            }
        }
    );

    // addStage("1v1 Strong Enemy - Shield",
    //     vec4(1., 0.5, 0.5, 1.0) * 0.8f,
    //     [&]()
    //     {

    //     }
    // );

    // addStage("Catalys Training Dummies",
    //     vec4(0.5, 0.5, 1.0, 1.0) * 0.8f,
    //     [&]()
    //     {

    //     }
    // );

    // addStage("1v1 Weak Mage",
    //     vec4(0.5, 0.5, 1.0, 1.0) * 0.8f,
    //     [&]()
    //     {

    //     }
    // );

    // addStage("1v1 String Mage",
    //     vec4(0.5, 0.5, 1.0, 1.0) * 0.8f,
    //     [&]()
    //     {

    //     }
    // );


    std::function<void(int, int, int, int)> spawnBattle = [&](int enemyS, int enemyZ, int allyS, int allyZ)
    {
        for(int i = 0; i < enemyS; i++) // Enemy Shields
        {
            vec3 randPos(-8 + rand()%16-8, Y_SPAWN_LEVEL, rand()%24-12);
            EntityRef e= spawnEntity("(Combats) Enemy 2", randPos);
            e->comp<Script>().addScript("Agent Update Test", ScriptHook::ON_AGENT_UPDATE);
            e->set<AgentState>(AgentState());
            // e->set<Target>((Target){GG::playerEntity.get()});
            e->set<Target>(Target());
            e->set<Faction>({Faction::PLAYER_ENEMY});
            e->set<AgentProfile>(AgentProfile());
            e->comp<AgentProfile>() 
                + spawnEntity("Combat Profile TEMPLATE")->comp<AgentProfile>()
                + spawnEntity("Combat Profile Tactitcs Trained")->comp<AgentProfile>()
                + spawnEntity("Combat Profile Initiative Passive")->comp<AgentProfile>()
            ;
            ComponentModularity::addChild(*stageNPC, e);
        }
        for(int i = 0; i < enemyZ; i++) // Enemy Zweihander
        {
            vec3 randPos(-16 + rand()%16-8, Y_SPAWN_LEVEL, rand()%24-12);
            EntityRef e= spawnEntity("(Combats) Enemy", randPos);
            e->comp<Script>().addScript("Agent Update Test", ScriptHook::ON_AGENT_UPDATE);
            e->set<AgentState>(AgentState());
            // e->set<Target>((Target){GG::playerEntity.get()});
            e->set<Target>(Target());
            e->set<Faction>({Faction::PLAYER_ENEMY});
            e->set<AgentProfile>(AgentProfile());
            e->comp<AgentProfile>() 
                + spawnEntity("Combat Profile TEMPLATE")->comp<AgentProfile>()
                + spawnEntity("Combat Profile Tactitcs Trained")->comp<AgentProfile>()
                + spawnEntity("Combat Profile Initiative Aggressive")->comp<AgentProfile>()
            ;
            ComponentModularity::addChild(*stageNPC, e);
        }


        for(int i = 0; i < allyS; i++) // Ally Shields
        {
            vec3 randPos(+8 + rand()%16-8, Y_SPAWN_LEVEL, rand()%24-12);
            EntityRef e= spawnEntity("(Combats) Player 2", randPos);
            e->comp<Script>().addScript("Agent Update Test", ScriptHook::ON_AGENT_UPDATE);
            e->set<AgentState>(AgentState());
            // e->set<Target>((Target){GG::playerEntity.get()});
            e->set<Target>(Target());
            e->set<Faction>({Faction::PLAYER});
            e->set<AgentProfile>(AgentProfile());
            e->comp<AgentProfile>() 
                + spawnEntity("Combat Profile TEMPLATE")->comp<AgentProfile>()
                + spawnEntity("Combat Profile Tactitcs Trained")->comp<AgentProfile>()
                + spawnEntity("Combat Profile Initiative Aggressive")->comp<AgentProfile>()
            ;
            ComponentModularity::addChild(*stageNPC, e);
        }
        for(int i = 0; i < allyZ; i++) // Ally Zweihander
        {
            vec3 randPos(+16 + rand()%16-8, Y_SPAWN_LEVEL, rand()%24-12);
            EntityRef e= spawnEntity("(Combats) Player", randPos);
            e->comp<Script>().addScript("Agent Update Test", ScriptHook::ON_AGENT_UPDATE);
            e->set<AgentState>(AgentState());
            // e->set<Target>((Target){GG::playerEntity.get()});
            e->set<Target>(Target());
            e->set<Faction>({Faction::PLAYER});
            e->set<AgentProfile>(AgentProfile());
            e->comp<AgentProfile>() 
                + spawnEntity("Combat Profile TEMPLATE")->comp<AgentProfile>()
                + spawnEntity("Combat Profile Tactitcs Trained")->comp<AgentProfile>()
                + spawnEntity("Combat Profile Initiative Aggressive")->comp<AgentProfile>()
            ;
            ComponentModularity::addChild(*stageNPC, e);
        }
    };


    addStage("3v4 Easy Battle",
        vec4(0.5, 1.0, 0.5, 1.0),
        [spawnBattle]()
        {
            spawnBattle(2, 2, 1, 1);
        }
    );

    addStage("10v4 Hard Battle",
        vec4(0.5, 1.0, 0.5, 1.0) * 0.8f,
        [spawnBattle]()
        {

        }
    );

    addStage("War",
        vec4(0.5, 1.0, 0.5, 1.0),
        [spawnBattle]()
        {
            const float nb = 250;
            spawnBattle(nb/2, nb/2, nb/2, nb/2);
        }
    );


    // AnimationControllerRef test = AnimBlueprint::bipedMoveset_PREALPHA_2025("(Human) 2H Sword ", GG::playerEntity.get());
}

void Apps::CombatsApp::update()
{
    if(!GG::playerEntity->has<AgentProfile>())
        GG::playerEntity->set<AgentProfile>(AgentProfile());

    ComponentModularity::synchronizeChildren(appRoot);

    /* 
        UPDATING ORBIT CONTROLLER ACTIVATION 
    */

    // auto &tmp = GG::playerEntity->comp<Items>().equipped[WEAPON_SLOT].item->comp<state3D>();
    // WARNING_MESSAGE(tmp.position << "\t" << tmp.quaternion)

    bool cameraFollow = globals.currentCamera->getMouseFollow();
    ModelStatus status = cameraFollow ? ModelStatus::HIDE : ModelStatus::SHOW;

    stageSelectionScreen->comp<WidgetState>().status = status;
    stageSelectionScreen->comp<WidgetState>().statusToPropagate = status;

    if(cameraFollow)
        globals.simulationTime.resume();
    else 
        globals.simulationTime.pause();

    GG::draw->update();
}


void Apps::CombatsApp::clean()
{
    globals.simulationTime.pause();

    Faction::clearRelations();

    GG::playerEntity = EntityRef();
    stageSelectionScreen = EntityRef();
    buttonsZone = EntityRef();
    stageNPC = EntityRef();
    appRoot = EntityRef();
    EDITOR::MENUS::GameScreen->comp<EntityGroupInfo>().children.clear();
    App::setController(nullptr);

    physicsMutex.lock();
    GG::ManageEntityGarbage__WithPhysics();
    physicsMutex.unlock();

    GG::sun->shadowCameraSize = vec2(0, 0);
    GG::draw = nullptr;
}

