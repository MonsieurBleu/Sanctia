#include <App.hpp>
#include <GLFW/glfw3.h>
#include <Subapps.hpp>
#include <EntityBlueprint.hpp>
#include <Game.hpp>

#include <AnimationBlueprint.hpp>

#include <Helpers.hpp>

#define Y_SPAWN_LEVEL 34

std::string PLAYER_SET = "(Combats) Player";

// #define PLAYER_SET "(Combats) Player 2"

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

                std::vector<EntityRef> garbage;
                for(auto i : appRoot->comp<EntityGroupInfo>().children)
                {
                    if(i->has<staticEntityFlag>() and i->comp<staticEntityFlag>().isDYnamic)
                        garbage.push_back(i);
                }

                for(auto i : garbage)
                    ComponentModularity::removeChild(*appRoot, i);

                garbage.clear();

                spawnPlayer(stageNPC);
                spawnScript();
            }, 
            [](Entity *e){return e->comp<WidgetBox>().isUnderCursor ? 1.f : 0.f;}
        )
    );

    newStageButton->comp<WidgetStyle>()
        .setbackgroundColor1(newStageColor)
        .setbackgroundColor2(newStageColor*1.1f)
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
                if (InputManager::isGamePadUsed() and InputManager::getGamepadAxisValue(GLFW_GAMEPAD_AXIS_LEFT_TRIGGER) > -0.95f)
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

        glLineWidth(10.0);
    }



    ComponentModularity::addChild(*appRoot, Blueprint::SpawnMainGameTerrain());

    appRoot->set<state3D>(true);
    appRoot->set<Script>(Script());
    appRoot->comp<Script>().addScript("World Update", ScriptHook::ON_UPDATE);

    // if(false)

    physicsMutex.lock();
    // ComponentModularity::addChild(*appRoot, spawnEntity("Ground_Demo_64xh", vec3(0, Y_SPAWN_LEVEL, 0)));
    ComponentModularity::ReparentChildren(*appRoot);
    physicsMutex.unlock();

    stageSelectionScreen = newEntity("Stage Selection Screen"
        , UI_BASE_COMP
        , WidgetBackground()
        , WidgetStyle()
            .setbackGroundStyle(UiTileType::SQUARE)
            .setbackgroundColor1(vec4(0.05, 0.05, 0.05, 1.0))
    );

    ComponentModularity::addChild(*stageSelectionScreen,newEntity("Background"
        , UI_BASE_COMP
        , WidgetBox(vec2(-2, 2), vec2(-2, 2))
        , WidgetSprite("Concept Decor 1")
    ));

    stageSelectionScreen->comp<WidgetBox>().useClassicInterpolation = true;
    ComponentModularity::addChild(*EDITOR::MENUS::GameScreen, stageSelectionScreen);

    buttonsZone = newEntity("Stage Selection Buttons Zone"
        , UI_BASE_COMP
        , WidgetBox(vec2(-0.1, 0.1), vec2(-0.9, 0.9))
        , WidgetStyle()
            .setautomaticTabbing(12)
            .setuseInternalSpacing(true)
    );
    ComponentModularity::addChild(*stageSelectionScreen, buttonsZone);

    ComponentModularity::addChild(*stageSelectionScreen,
        newEntity("Player Selection Screen"
        , UI_BASE_COMP
        , WidgetBox(vec2(0.4, 0.6), vec2(-0.125, 0.125))
        , WidgetStyle()
            .setautomaticTabbing(2)
            .setuseInternalSpacing(true)
        , EntityGroupInfo({
            VulpineBlueprintUI::Toggable(
                "Player equip Sword and Shield", "", 
                [](Entity *e, float f)
                {PLAYER_SET = "(Combats) Player 2";}, 
                [](Entity *e){return PLAYER_SET == "(Combats) Player 2" ? 0.f : 1.f;},
                vec3(58, 155, 184)/255.f
            ),
            VulpineBlueprintUI::Toggable(
                "Player equip Zweihander", "", 
                [](Entity *e, float f)
                {PLAYER_SET = "(Combats) Player";}, 
                [](Entity *e){return PLAYER_SET == "(Combats) Player" ? 0.f : 1.f;},
                vec3(58, 155, 184)/255.f
            )
        })
        )
    );

    ComponentModularity::addChild(*stageSelectionScreen,

        newEntity("tmp", UI_BASE_COMP, EntityGroupInfo({

        newEntity("Important Notes Group"
            , UI_BASE_COMP
            , WidgetBox(vec2(-0.85, -0.25), vec2(-0.9, 0.1))
            , WidgetStyle().setuseInternalSpacing(true).setautomaticTabbing(2)
            , EntityGroupInfo({

                newEntity("Important Notes"
                , UI_BASE_COMP
                , WidgetBackground()
                , WidgetStyle()
                    .setbackGroundStyle(UiTileType::SQUARE_ROUNDED)
                    .setbackgroundColor1(VulpineColorUI::DarkBackgroundColor1Opaque)
                    .settextColor1(VulpineColorUI::LightBackgroundColor1)
                    .setminFontScale(1.5)
                , WidgetText(
                    U"[ENG]\n"
                    U"\n"
                    U"This is a prototype of a work-in-progress game called \"Sanctia\",\n"
                    U"focused on testing the current combat mechanics.\n"
                    U"\n"
                    U"The main project is meant to be an open-world immersive-sim, focused\n"
                    U"on an intra-diegetic gaming experience, where the player has to\n"
                    U"explore, keep track of quests, make notes or research magic only\n"
                    U"with the tools available inside of the game universe.\n"
                    U"\n"
                    U"For now, you can test a sample of the game's swordfights.\n"
                    U"\n"
                    U"\n"
                    U"***Everything here is work-in-progress, so your feedback is really important !***"
                )),

                newEntity("Important Notes"
                , UI_BASE_COMP
                , WidgetBackground()
                , WidgetStyle()
                    .setbackGroundStyle(UiTileType::SQUARE_ROUNDED)
                    .setbackgroundColor1(VulpineColorUI::DarkBackgroundColor1Opaque)
                    .settextColor1(VulpineColorUI::LightBackgroundColor1)
                    .setminFontScale(1.5)
                , WidgetText(
                    U"[FR]\n"
                    U"\n"
                    U"Vous jouez au prototype de combat d'un jeu en cours de développement\n"
                    U"appelé \"Sanctia\".\n"
                    U"\n"
                    U"Le projet principal est de créer un immersive-sim en monde ouvert\n"
                    U"axé sur une approche intradiégétique, où le joueur devra explorer,\n"
                    U"suivre les quêtes, prendre des notes ou étudier la magie en se servant\n"
                    U"uniquement des outils existants dans l'univers du jeu.\n"
                    U"\n"
                    U"Pour le moment, vous allez tester les combats à l'épée.\n"
                    U"\n"
                    U"\n"
                    U"***Tout dans cette démo est en cours de dev., donc vos retours sont précieux !***"
                ))
            })
        )
        }))
    );

    EntityRef tmp1, tmp2;

    ComponentModularity::addChild(*stageSelectionScreen,

        newEntity("tmp"
            , UI_BASE_COMP
            , EntityGroupInfo({
                newEntity("qr codes"
                    , UI_BASE_COMP
                    , WidgetBox(vec2(0.3, 0.8), vec2(-0.9, -0.6))
                    , WidgetStyle().setuseInternalSpacing(true).setautomaticTabbing(2)
                    , EntityGroupInfo({
                        tmp1 = newEntity(""
                            , UI_BASE_COMP
                            , WidgetBackground()
                            , WidgetStyle()
                                .setbackGroundStyle(UiTileType::SQUARE_ROUNDED)
                                .setbackgroundColor1(VulpineColorUI::DarkBackgroundColor1Opaque)
                                .settextColor1(VulpineColorUI::LightBackgroundColor1)
                                .setminFontScale(1.25)
                            , WidgetText(U"Want to follow the project ?\n\n**Join Our Discord !**")
                        ),
                        newEntity(""
                            , UI_BASE_COMP
                            // , WidgetBackground()
                            , WidgetStyle()
                                .setbackgroundColor1(VulpineColorUI::DarkBackgroundColor1Opaque)
                                .setspriteScale(2.)
                                .setspritePosition(vec2(-0.5, 0))
                            , WidgetSprite("qr discord")
                        ),


                        tmp2 = newEntity(""
                            , UI_BASE_COMP
                            , WidgetBackground()
                            , WidgetStyle()
                                .setbackGroundStyle(UiTileType::SQUARE_ROUNDED)
                                .setbackgroundColor1(VulpineColorUI::DarkBackgroundColor1Opaque)
                                .settextColor1(VulpineColorUI::LightBackgroundColor1)
                                .setminFontScale(1.25)
                            , WidgetText(U"Want to support us ?\n\n**Give your feedback !**", StringAlignment::CENTERED)
                        ),
                        newEntity(""
                            , UI_BASE_COMP
                            // , WidgetBackground()
                            , WidgetStyle()
                                .setbackgroundColor1(VulpineColorUI::DarkBackgroundColor1Opaque)
                                .setspriteScale(2.)
                                .setspritePosition(vec2(-0.5, 0))
                            , WidgetSprite("qr form combats")
                        ),

                        
                    })
                )
        }))
    );

    tmp1->comp<WidgetText>().mesh->align = StringAlignment::CENTERED;
    tmp2->comp<WidgetText>().mesh->align = StringAlignment::CENTERED;

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

    const float alpha = 1.0;

    static const vec2 normalHour(16, 0);
    static const vec2 alternateHour(7, 30);
    static const vec2 sunrise(6, 30); 
    static const vec2 night(0, 0);

    const vec3 TutorialColor = "#d5dadd"_rgb;
    const vec3 EasyDuelColor = "#c7b15a"_rgb;
    const vec3 NormalDuelColor = "#d98632"_rgb;
    const vec3 HardDuelColor = "#ca3b49"_rgb;

    const vec3 NightEmbuscadeColor = "#9879a8"_rgb;

    const vec3 BattleColor = "#33c85b"_rgb;

    addStage("Tutorial",
        vec4(TutorialColor, alpha),
        [&]()
        {
            GG::timeOfDay = GG::hourToTimeOfDay(normalHour.x, normalHour.y);

            for(float i = -1.f; i <= 1.f; i += 1.f)
            {
                EntityRef e = spawnEntity("(Combats) Enemy");
                e->comp<state3D>().useinit = true;
                e->comp<state3D>().initPosition = vec3(8.f, Y_SPAWN_LEVEL, i*4.f);
                e->comp<Script>().addScript("Action-Dummy Update", ScriptHook::ON_UPDATE);
                e->set<Faction>({Faction::PLAYER_ENEMY});
                ComponentModularity::addChild(*stageNPC, e);
            }

            for(float i = -1.f; i <= 1.f; i += 1.f)
            {
                EntityRef e = spawnEntity("(Combats) Enemy 2");
                e->comp<state3D>().useinit = true;
                e->comp<state3D>().initPosition = vec3(0.f, Y_SPAWN_LEVEL, i*4.f);
                e->comp<Script>().addScript("Action-Dummy Update", ScriptHook::ON_UPDATE);
                e->set<Faction>({Faction::PLAYER_ENEMY});
                ComponentModularity::addChild(*stageNPC, e);
            }

            EntityModel tutorialGroup({newObjectGroup()});

            {
                static std::string str = 
                    "[ENG]\n"
                    "Zweihanders deal a lot of damage and have a long reach.\n"
                    "If you get caught by one, you will get **seriously** hurt.\n"
                    "\n"
                    "[FR]\n"
                    "Les Zweihander font de lourds dégâts et ont une grande portée d'attaque.\n"
                    "Si vous vous faites toucher, vous aurez **très** mal.\n"
                ;
                ValueHelperRef<std::string> text( new ValueHelper(str, U"", vec3(1, 0, 0)));
                text->state.scaleScalar(4.f).setPosition(vec3(8, 0, 0));
                tutorialGroup->add(text);
            }

            static std::string strAtck = 
                // "Primary attacks can be blocked.\n"
                // "When blocking an attack, you can counter attack if you are quick enough.\n"
                // "With good timing, you can also do a perfect pary, that can also counter attack."

                "[ENG]\n"
                "Attacks can be blocked and immediately countered, if you are quick enough.\n"
                "\n"
                "[FR]\n"
                "Les attaques peuvent être bloquées, et même contrées, si vous êtes assez rapide."
                ;
            {
                ValueHelperRef<std::string> text( new ValueHelper(strAtck, U"", vec3(0.05)));
                text->state.scaleScalar(4.f).setPosition(vec3(0, 0.75, 0));
                tutorialGroup->add(text);
            }
            {
                ValueHelperRef<std::string> text( new ValueHelper(strAtck, U"", vec3(0.05)));
                text->state.scaleScalar(4.f).setPosition(vec3(8, 0.75, 0));
                tutorialGroup->add(text);
            }
            



            {
                static std::string str = 
                    // "Shields are very effective for blocking.\n"
                    // "The guard of a shield user can only be broken by depleating their stamina.\n"
                    // "Kicks are the most effective way to do that.\n"

                    "[ENG]\n"
                    "Shields are very good at blocking attacks.\n"
                    "\n"
                    "[FR]\n"
                    "Les boucliers sont particulièrement forts pour bloquer les attaques."
                ;
                ValueHelperRef<std::string> text( new ValueHelper(str, U"", vec3(1, 0, 0)));
                text->state.scaleScalar(4.f).setPosition(vec3(0, 0, 4));
                tutorialGroup->add(text);
            }

            static std::string strBlock = 
                // "When an enemy is blocking, they can't be hurt unless their guard is broken.\n"
                // "Kicks are the most effective way to do so.\n"
                // "When a kick break the guard of an enemy, you can directlly follow up with an attack.\n"

                "[ENG]\n"
                "Try and break their guard.\n"
                "\n"
                "[FR]\n"
                "Essayez de briser sa guarde."
                ;
            {
                ValueHelperRef<std::string> text( new ValueHelper(strBlock, U"", vec3(0.05)));
                text->state.scaleScalar(4.f).setPosition(vec3(0, 0.75, 4));
                tutorialGroup->add(text);
            }
            {
                ValueHelperRef<std::string> text( new ValueHelper(strBlock, U"", vec3(0.05)));
                text->state.scaleScalar(4.f).setPosition(vec3(8, 0.75, 4));
                tutorialGroup->add(text);
            }




            static std::string strKick = 
                // "A kick can often-times break your guard.\n"
                // "Regular attacks are faster to perform and have a longer reach than kicks.\n"

                "[ENG]\n"
                "A simple kick is a good way to break an enemy's guard.\n"
                "\n"
                "[FR]\n"
                "Un simple coup de pied est la meilleure manière de briser la garde d'un ennemi."
                ;
            {
                ValueHelperRef<std::string> text( new ValueHelper(strKick, U"", vec3(0.05)));
                text->state.scaleScalar(4.f).setPosition(vec3(0, 0.75, -4));
                tutorialGroup->add(text);
            }
            {
                ValueHelperRef<std::string> text( new ValueHelper(strKick, U"", vec3(0.05)));
                text->state.scaleScalar(4.f).setPosition(vec3(8, 0.75, -4));
                tutorialGroup->add(text);
            }



            tutorialGroup->state.setPosition(vec3(0, Y_SPAWN_LEVEL + 2.0, 0));


            ComponentModularity::addChild(*stageNPC, newEntity("Tutorial", tutorialGroup));
        }
    );

    ComponentModularity::addChild(*buttonsZone, newEntity());

    addStage("Easy Duel 1",
        vec4(EasyDuelColor, alpha),
        [&]()
        {
            GG::timeOfDay = GG::hourToTimeOfDay(normalHour.x, normalHour.y);

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

    addStage("Easy Duel 2",
        vec4(EasyDuelColor, alpha),
        [&]()
        {
            GG::timeOfDay = GG::hourToTimeOfDay(normalHour.x, normalHour.y);

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

    ComponentModularity::addChild(*buttonsZone, newEntity());

    addStage("Normal Duel 1",
        vec4(NormalDuelColor, alpha),
        [&]()
        {
            GG::timeOfDay = GG::hourToTimeOfDay(alternateHour.x, alternateHour.y);

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

    addStage("Normal Duel 2",
        vec4(NormalDuelColor, alpha),
        [&]()
        {
            GG::timeOfDay = GG::hourToTimeOfDay(alternateHour.x, alternateHour.y);

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

    ComponentModularity::addChild(*buttonsZone, newEntity());

    addStage("Hard Duel 1",
        vec4(HardDuelColor, alpha),
        [&]()
        {
            GG::timeOfDay = GG::hourToTimeOfDay(sunrise.x, sunrise.y);

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

    addStage("Hard Duel 2",
        vec4(HardDuelColor, alpha),
        [&]()
        {
            GG::timeOfDay = GG::hourToTimeOfDay(sunrise.x, sunrise.y);

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

    addStage("Rigged Duel",
        vec4(HardDuelColor, alpha),
        [&]()
        {
            GG::timeOfDay = GG::hourToTimeOfDay(normalHour.x, normalHour.y);

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

    std::function<void(int, int, int, int)> spawnBattle = [&](int enemyS, int enemyZ, int allyS, int allyZ)
    {
        const std::string tactitcs = "Trained";
        // const std::string tactitcs = "God Duelist";
        const std::string initiative = "Very Aggressive";

        for(int i = 0; i < enemyS; i++) // Enemy Shields
        {

            vec3 randPos(-8 + rand()%16-8, Y_SPAWN_LEVEL, rand()%24-12);
            EntityRef e= spawnEntity("(Combats) Ally 2", randPos);
            e->comp<Script>().addScript("Agent Update Test", ScriptHook::ON_AGENT_UPDATE);
            e->set<AgentState>(AgentState());
            // e->set<Target>((Target){GG::playerEntity.get()});
            e->set<Target>(Target());
            e->set<Faction>({Faction::PLAYER_ENEMY});
            e->set<AgentProfile>(AgentProfile());
            e->comp<AgentProfile>() 
                + spawnEntity("Combat Profile TEMPLATE")->comp<AgentProfile>()
                + spawnEntity("Combat Profile Tactitcs " + tactitcs)->comp<AgentProfile>()
                + spawnEntity("Combat Profile Initiative " + initiative)->comp<AgentProfile>()
            ;
            ComponentModularity::addChild(*stageNPC, e);
        }
        for(int i = 0; i < enemyZ; i++) // Enemy Zweihander
        {
            vec3 randPos(-16 + rand()%16-8, Y_SPAWN_LEVEL, rand()%24-12);
            EntityRef e= spawnEntity("(Combats) Ally", randPos);
            e->comp<Script>().addScript("Agent Update Test", ScriptHook::ON_AGENT_UPDATE);
            e->set<AgentState>(AgentState());
            // e->set<Target>((Target){GG::playerEntity.get()});
            e->set<Target>(Target());
            e->set<Faction>({Faction::PLAYER_ENEMY});
            e->set<AgentProfile>(AgentProfile());
            e->comp<AgentProfile>() 
                + spawnEntity("Combat Profile TEMPLATE")->comp<AgentProfile>()
                + spawnEntity("Combat Profile Tactitcs " + tactitcs)->comp<AgentProfile>()
                + spawnEntity("Combat Profile Initiative " + initiative)->comp<AgentProfile>()
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
                + spawnEntity("Combat Profile Tactitcs " + tactitcs)->comp<AgentProfile>()
                + spawnEntity("Combat Profile Initiative " + initiative)->comp<AgentProfile>()
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
                + spawnEntity("Combat Profile Tactitcs " + tactitcs)->comp<AgentProfile>()
                + spawnEntity("Combat Profile Initiative " + initiative)->comp<AgentProfile>()
            ;
            ComponentModularity::addChild(*stageNPC, e);
        }
    };

    ComponentModularity::addChild(*buttonsZone, newEntity());

    addStage("Double Duel",
        vec4(NightEmbuscadeColor, alpha),
        [&, spawnBattle]()
        {
            GG::timeOfDay = GG::hourToTimeOfDay(normalHour.x, normalHour.y);
            spawnBattle(1, 1, 1, 1);
        }
    );

    addStage("Night Embuscade",
        vec4(NightEmbuscadeColor, alpha),
        [&, spawnBattle]()
        {
            GG::timeOfDay = GG::hourToTimeOfDay(night.x, night.y);
            spawnBattle(2, 2, 1, 1);
        }
    );

    ComponentModularity::addChild(*buttonsZone, newEntity());

    addStage("Battle",
        vec4(BattleColor, alpha),
        [&, spawnBattle]()
        {
            GG::timeOfDay = GG::hourToTimeOfDay(alternateHour.x, alternateHour.y);
            spawnBattle(6, 6, 5, 5);
        }
    );

    addStage("War",
        vec4(BattleColor, alpha),
        [&, spawnBattle]()
        {
            GG::timeOfDay = GG::hourToTimeOfDay(sunrise.x, sunrise.y);
            const float nbAlly = 100;
            const float nbEnemy = 104;
            spawnBattle(nbEnemy/2, nbEnemy/2, nbAlly/2, nbAlly/2);
        }
    );

    ComponentModularity::addChild(*buttonsZone, newEntity());

    ComponentModularity::addChild(*buttonsZone, VulpineBlueprintUI::Toggable("Return To Desktop", "", 
        [](Entity *e, float f)
        {
            Game::state = AppState::quit;
        },
        [](Entity *e){return 1.f;},
        vec4(vec3(VulpineColorUI::LightBackgroundColor2), 1.0)
    ));

    buttonsZone->comp<WidgetStyle>().setautomaticTabbing(buttonsZone->comp<EntityGroupInfo>().children.size());

    // addStage("War",
    //     vec4(0.5, 1.0, 0.5, 1.0),
    //     [spawnBattle]()
    //     {
    //         const float nb = 150;
    //         spawnBattle(nb/2, nb/2, nb/2, nb/2);
    //     }
    // );


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
}


void Apps::CombatsApp::clean()
{
    globals.simulationTime.pause();

    Faction::clearRelations();

    ComponentModularity::removeChild(*EDITOR::MENUS::GameScreen, stageSelectionScreen);

    GG::playerEntity = EntityRef();
    stageSelectionScreen = EntityRef();
    buttonsZone = EntityRef();
    stageNPC = EntityRef();
    appRoot = EntityRef();
    // EDITOR::MENUS::GameScreen->comp<EntityGroupInfo>().children.clear();
    App::setController(nullptr);

    physicsMutex.lock();
    GG::ManageEntityGarbage__WithPhysics();
    physicsMutex.unlock();

    GG::sun->shadowCameraSize = vec2(0, 0);
}

