#include <fstream>
#include <thread>

#include <Game.hpp>
#include <Globals.hpp>
// #include <GameObject.hpp>
#include <AssetManager.hpp>
#include <Audio.hpp>
#include <CompilingOptions.hpp>
#include <Constants.hpp>
#include <EntityBlueprint.hpp>
#include <Helpers.hpp>
#include <MathsUtils.hpp>

#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtx/string_cast.hpp>

#include <AnimationBlueprint.hpp>
#include <Graphics/Animation.hpp>
#include <Graphics/Skeleton.hpp>

#include <PhysicsGlobals.hpp>

#include <Subapps.hpp>

void Game::mainloop()
{

    /****** Loading Models and setting up the scene ******/
    globals.simulationTime.pause();

    ModelRef skybox = newModel(skyboxMaterial);
    skybox->loadFromFolder("ressources/models/skybox/", true, false);

    // skybox->invertFaces = true;
    // skybox->depthWrite = true;
    skybox->state.frustumCulled = false;
    skybox->state.scaleScalar(1E6);
    scene.add(skybox);

    Texture2D EnvironementMap = Texture2D().loadFromFile("ressources/HDRIs/quarry_cloudy_2k.jpg").generate();

    SceneDirectionalLight sun = newDirectionLight(DirectionLight()
                                                      // .setColor(vec3(0xFF, 0xBF, 0x7F) / vec3(255))
                                                      .setColor(vec3(1))
                                                      .setDirection(normalize(vec3(-1.0, -1.0, 0.0)))
                                                      .setIntensity(1.0));

    sun->cameraResolution = vec2(8192);
    // sun->cameraResolution = vec2(2048);
    // sun->cameraResolution = vec2(4096);
    sun->shadowCameraSize = vec2(256, 256);
    sun->activateShadows();
    scene.add(sun);

    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glEnable(GL_DEPTH_TEST);
    glLineWidth(1.0);

    /****** Setting Up Debug UI *******/
    FastUI_context ui(fuiBatch, FUIfont, scene2D, defaultFontMaterial);
    ui.spriteMaterial = Loader<MeshMaterial>::get("sprite");
    EDITOR::UIcontext = WidgetUI_Context{&ui};

    ui.colorTitleBackground.a = 0.9f;

    float widgetTileSPace = 0.01;

    vec3 sunDir = vec3(0);
    skybox->uniforms.add(ShaderUniform(&sunDir, 21));

    EDITOR::MENUS::GameScreen = gameScreenWidget = newEntity("Game Screen Widget"
        , WidgetUI_Context{&ui}
        , WidgetState()
        , WidgetBox(vec2(0, +1), vec2(0, +1))
        , WidgetText(U" ")
        , EntityGroupInfo({
            EDITOR::MENUS::AppChoice =
                newEntity("Application Choice Menu"
                    , WidgetUI_Context{&ui}
                    , WidgetState()
                    , WidgetBox(vec2(-1, 1)
                    , vec2(-1.1, -1) + vec2(0.f, -widgetTileSPace))
                    , WidgetBackground()
                    , WidgetStyle().setbackgroundColor1(EDITOR::MENUS::COLOR::DarkBackgroundColor1)
                    ), 
            EDITOR::MENUS::AppControl = 
                newEntity("Current Application Controls"
                    , WidgetUI_Context{&ui}
                    , WidgetState()
                    , WidgetBox(vec2(-1, 1), vec2(1, 1.1) + vec2(widgetTileSPace, -widgetTileSPace))
                    , WidgetBackground()
                    , WidgetStyle().setbackgroundColor1(EDITOR::MENUS::COLOR::DarkBackgroundColor1)
                    ),
            EDITOR::MENUS::GlobalControl = 
                newEntity("Global Controls"
                    , WidgetUI_Context{&ui}
                    , WidgetState()
                    , WidgetBox(vec2(-1, 1), vec2(1.1, 1.2) + vec2(widgetTileSPace, -widgetTileSPace) - vec2(widgetTileSPace))
                    , WidgetBackground()
                    , WidgetStyle().setbackgroundColor1(EDITOR::MENUS::COLOR::DarkBackgroundColor1)
                    ),
            EDITOR::MENUS::GlobalInfos =
                newEntity("Global Informations"
                    , WidgetUI_Context{&ui}
                    , WidgetState()
                    , WidgetBox(vec2(-1, 1), vec2(1.2, 1.9) - vec2(widgetTileSPace * 1.f, 0.f)), WidgetBackground()
                    , WidgetStyle().setbackgroundColor1(EDITOR::MENUS::COLOR::DarkBackgroundColor1)
                    ),
            EDITOR::MENUS::AppMenu =
                newEntity("Current Application Menus"
                    , WidgetUI_Context{&ui}
                    , WidgetState()
                    , WidgetBox(vec2(-2, -1) + vec2(0, -widgetTileSPace), vec2(-1.1, 1.9))
                    , WidgetBackground()
                    , WidgetStyle().setbackgroundColor1(EDITOR::MENUS::COLOR::DarkBackgroundColor1)
                    ),
                }
            )
        );

    gameScreenWidget->set<WidgetBox>(WidgetBox(vec2(-0.33333, 1), vec2(-0.933333, 0.40)));

    finalProcessingStage.addUniform(ShaderUniform((vec4 *)&gameScreenWidget->comp<WidgetBox>().displayMin, 12));

    // EDITOR::MENUS::GlobalInfos->comp<WidgetStyle>().setautomaticTabbing(1);

    // for(int i = 0; i < 5; i++)
    // ComponentModularity::addChild(*EDITOR::MENUS::GlobalInfos,
    //     Blueprint::EDITOR_ENTITY::INO::Toggable("Ingo Stat Helper"
    //         , "icon_idcard"
    //         , WidgetButton::InteractFunc([](float v){
    //             GlobalComponentToggler<InfosStatsHelpers>::activated = !GlobalComponentToggler<InfosStatsHelpers>::activated;
    //         })
    //         , WidgetButton::UpdateFunc([](){
    //             return GlobalComponentToggler<InfosStatsHelpers>::activated ? 0.f : 1.f;
    //         })
    //     )
    // );

    // ComponentModularity::addChild(*EDITOR::MENUS::GlobalInfos,
    //     Blueprint::EDITOR_ENTITY::INO::TimerPlot(globals.appTime, EDITOR::MENUS::COLOR::HightlightColor1)
    // );

    // ComponentModularity::addChild(*EDITOR::MENUS::GlobalInfos,
    //     Blueprint::EDITOR_ENTITY::INO::TimerPlot(physicsTimer, EDITOR::MENUS::COLOR::HightlightColor2)
    // );

    // ComponentModularity::addChild(*EDITOR::MENUS::GlobalInfos,
    //     Blueprint::EDITOR_ENTITY::INO::TimerPlot(globals.cpuTime, EDITOR::MENUS::COLOR::HightlightColor3)
    // );

    // ComponentModularity::addChild(*EDITOR::MENUS::GlobalInfos,
    //     Blueprint::EDITOR_ENTITY::INO::TimerPlot(globals.gpuTime, EDITOR::MENUS::COLOR::HightlightColor4)
    // );

    EntityRef GlobalInfosTitleTab = newEntity("Global Infos Title Tab"
        , WidgetUI_Context{&ui}
        , WidgetState()
        , WidgetBox(vec2(-1, 1), vec2(-1, -0.75))
        , WidgetBackground()
        , WidgetStyle()
            .setautomaticTabbing(1)
            .setbackgroundColor1(EDITOR::MENUS::COLOR::DarkBackgroundColor1)
            .setbackGroundStyle(UiTileType::SQUARE_ROUNDED)
    );

    EntityRef GlobalInfosSubTab = newEntity("Global Infos Sub Tab"
        , WidgetUI_Context{&ui}
        // , WidgetState()
        , WidgetBox(vec2(-1, 1), vec2(-0.75, 1))
        , WidgetBackground()
        , WidgetStyle()
            // .setautomaticTabbing(1)
            .setbackgroundColor1(EDITOR::MENUS::COLOR::DarkBackgroundColor1)
            .setbackGroundStyle(UiTileType::SQUARE_ROUNDED)
    );


    Blueprint::EDITOR_ENTITY::INO::AddToSelectionMenu(
        GlobalInfosTitleTab, GlobalInfosSubTab, 
        Blueprint::EDITOR_ENTITY::INO::GlobalBenchmarkScreen(),
        "Global Benchmark", "icon_chrono"
    );

    Blueprint::EDITOR_ENTITY::INO::AddToSelectionMenu(
        GlobalInfosTitleTab, GlobalInfosSubTab,
        Blueprint::EDITOR_ENTITY::INO::SceneInfos(scene),
        "3D Scene Infos", ""
    );

    Blueprint::EDITOR_ENTITY::INO::AddToSelectionMenu(
        GlobalInfosTitleTab, GlobalInfosSubTab,
        Blueprint::EDITOR_ENTITY::INO::SceneInfos(scene2D),
        "2D Scene Infos", ""
    );


    /* TODO : finish*/
    // Blueprint::EDITOR_ENTITY::INO::AddToSelectionMenu(
    //     GlobalInfosTitleTab, GlobalInfosSubTab,

    //     newEntity("Scenes Infos Menu", 
    //         , WidgetUI_Context{&ui}
    //         , WidgetState()
    //         , WidgetBox()
    //         , Widget

    //     ),

    //     // Blueprint::EDITOR_ENTITY::INO::SceneInfos(scene),

    //     "Scenes Infos", "VulpineIcon"
    // );

    GlobalInfosTitleTab->comp<EntityGroupInfo>().children[0]->comp<WidgetState>().statusToPropagate = ModelStatus::SHOW;


    ComponentModularity::addChild(*EDITOR::MENUS::GlobalInfos, GlobalInfosTitleTab);
    ComponentModularity::addChild(*EDITOR::MENUS::GlobalInfos, GlobalInfosSubTab);

    // EDITOR::MENUS::AppMenu->comp<WidgetStyle>().setautomaticTabbing(1);

    // for(int i = 0; i < 1; i++)
    // ComponentModularity::addChild(*EDITOR::MENUS::AppMenu,
    //     Blueprint::EDITOR_ENTITY::INO::ColorSelectionScreen(
    //         "Sun color", 
    //         [&sun](){return vec3(sun->getInfos()._color);},
    //         [&sun](vec3 color){sun->setColor(color);}
    //     )
    // );

// #define TEST_ELEMENT_COMPONENT                                                                                         \
//     , WidgetUI_Context{&ui}, WidgetState(),                                                                            \
//         WidgetBox(vec2(-0.9, +0.9), vec2(+1.1, +2.6), WidgetBox::Type::FOLLOW_SIBLINGS_BOX), WidgetBackground(),       \
//         WidgetText(), WidgetButton(WidgetButton::Type::HIDE_SHOW_TRIGGER), WidgetStyle()

//     EntityRef first = newEntity(
//         "first", WidgetUI_Context{&ui}, WidgetState(), WidgetBox(vec2(-0.25, +0.25), vec2(-0.9, -0.8)),
//         WidgetBackground(), WidgetText(U"Titre important"),
//         WidgetButton(WidgetButton::Type::HIDE_SHOW_TRIGGER
//                      // WidgetButton::Type::CHECKBOX,
//                      // WidgetButton::func([](float v)
//                      // {
//                      //     // std::cout << v << "\n";
//                      //     GlobalComponentToggler<PhysicsHelpers>::activated =
//                      //     !GlobalComponentToggler<PhysicsHelpers>::activated;
//                      // })
//                      ),
//         WidgetStyle(),
//         EntityGroupInfo({
//             newEntity("Sous élément 1" TEST_ELEMENT_COMPONENT,
//                       EntityGroupInfo({
//                           newEntity("Sous élément 1-1" TEST_ELEMENT_COMPONENT),
//                           newEntity("Sous élément 1-2" TEST_ELEMENT_COMPONENT),
//                           newEntity("Sous élément 1-3" TEST_ELEMENT_COMPONENT),
//                           newEntity("Sous élément 1-4" TEST_ELEMENT_COMPONENT,
//                                     EntityGroupInfo({
//                                         newEntity("Sous élément 1-4-1" TEST_ELEMENT_COMPONENT),
//                                         newEntity("Sous élément 1-4-2" TEST_ELEMENT_COMPONENT),
//                                         newEntity("Sous élément 1-4-3" TEST_ELEMENT_COMPONENT),
//                                         newEntity("Sous élément 1-4-4" TEST_ELEMENT_COMPONENT),
//                                     })),
//                       })),
//             newEntity("Sous élément 2" TEST_ELEMENT_COMPONENT, EntityGroupInfo({
//                                                                    newEntity("Sous élément 2-1" TEST_ELEMENT_COMPONENT),
//                                                                    newEntity("Sous élément 2-2" TEST_ELEMENT_COMPONENT),
//                                                                    newEntity("Sous élément 2-3" TEST_ELEMENT_COMPONENT),
//                                                                    newEntity("Sous élément 2-4" TEST_ELEMENT_COMPONENT),
//                                                                })),
//             newEntity("Sous élément 3" TEST_ELEMENT_COMPONENT), newEntity("Sous élément 4" TEST_ELEMENT_COMPONENT),
//             // newEntity("Sous élément 5"
//             //     TEST_ELEMENT_COMPONENT
//             //     , WidgetBox(
//             //         vec2(-0.9, +0.9),
//             //         vec2(-2.5, -1.1),
//             //         WidgetBox::Type::FOLLOW_SIBLINGS_BOX
//             //     )
//             // ),
//         }));

//     ComponentModularity::addChild(*EDITOR::MENUS::AppMenu, first);

    EDITOR::MENUS::AppControl->comp<WidgetStyle>().setautomaticTabbing(1);
    EDITOR::MENUS::AppChoice->comp<WidgetStyle>().setautomaticTabbing(1);
    EDITOR::MENUS::GlobalControl->comp<WidgetStyle>().setautomaticTabbing(1);

    ComponentModularity::addChild(*EDITOR::MENUS::GlobalControl,
        Blueprint::EDITOR_ENTITY::INO::ValueInputSlider(
            "Time of day", 
            0, 24, 24*4, 
            [](float v)
            {
                float t = v;
                GG::timeOfDay = t;
            },
            []()
            {
                float t = GG::timeOfDay;
                return t;
            },
            [](std::u32string text)
            {
                float t = u32strtof2(text, GG::timeOfDay);
                GG::timeOfDay = t;
            }, 
            []()
            {
                float t = GG::timeOfDay;
                return ftou32str(t);
            }
        )
    );

    bool enableTime = false;
    ComponentModularity::addChild(*EDITOR::MENUS::GlobalControl,
        Blueprint::EDITOR_ENTITY::INO::Toggable(
            "Enable Time", 
            "icon_light",
            [&enableTime](float v)
            {
                enableTime = !enableTime;
            },
            [&enableTime]()
            {
                return enableTime ? 0.f : 1.f;
            }
        )
    );

    ComponentModularity::addChild(*EDITOR::MENUS::GlobalControl,
        Blueprint::EDITOR_ENTITY::INO::Toggable(
            "Entity Infos Stats Helper", 
            "icon_idcard",
            [&](float v)
            {
                GlobalComponentToggler<InfosStatsHelpers>::activated =
                    !GlobalComponentToggler<InfosStatsHelpers>::activated;
            },
            [&]()
            {
                return GlobalComponentToggler<InfosStatsHelpers>::activated  ? 0.f : 1.f;
            }
        )
    );


    ComponentModularity::addChild(*EDITOR::MENUS::GlobalControl,
        Blueprint::EDITOR_ENTITY::INO::Toggable(
            "Entity Physic Helper", 
            "icon_hitbox",
            [&](float v)
            {
                GlobalComponentToggler<PhysicsHelpers>::activated =
                    !GlobalComponentToggler<PhysicsHelpers>::activated;
            },
            [&]()
            {
                return GlobalComponentToggler<PhysicsHelpers>::activated  ? 0.f : 1.f;
            }
        )
    );

    {
        auto &bloom = Bloom;
        ComponentModularity::addChild(*EDITOR::MENUS::GlobalControl,
            Blueprint::EDITOR_ENTITY::INO::Toggable(
                "Bloom", 
                "",
                [&bloom](float v)
                {
                    bloom.toggle();
                },
                [&bloom]()
                {
                    return bloom.isPassEnable() ? 0.f : 1.f;
                }
            )
        );
    }
    {
        auto &ssao = SSAO;
        ComponentModularity::addChild(*EDITOR::MENUS::GlobalControl,
            Blueprint::EDITOR_ENTITY::INO::Toggable(
                "AO", 
                "",
                [&ssao](float v)
                {
                    ssao.toggle();
                },
                [&ssao]()
                {
                    return ssao.isPassEnable() ? 0.f : 1.f;
                }
            )
        );
    }

    ComponentModularity::addChild(*EDITOR::MENUS::GlobalControl,
        Blueprint::EDITOR_ENTITY::INO::Toggable(
            "Physic Interpolation", 
            "",
            [](float v)
            {
                PG::doPhysicInterpolation = !PG::doPhysicInterpolation;
            },
            []()
            {
                return PG::doPhysicInterpolation ? 0.f : 1.f;
            }
        )
    );

    ComponentModularity::addChild(*EDITOR::MENUS::GlobalControl,
        Blueprint::EDITOR_ENTITY::INO::Toggable(
            "Auto Shader Refresh", 
            "",
            [](float v)
            {
                Game::doAutomaticShaderRefresh = !Game::doAutomaticShaderRefresh;
            },
            []()
            {
                return Game::doAutomaticShaderRefresh ? 0.f : 1.f;
            }
        )
    );


    Apps::MainGameApp testsubapps1;
    Apps::MaterialViewerApp materialView;

    // SubApps::switchTo(materialView);

    // Apps::MainGameApp testsubapps2;
    // Apps::MainGameApp testsubapps3;
    // Apps::MainGameApp testsubapps4;




    /****** Creating Demo Player *******/



    
    // Entity* parent = readTest.get();
    



    // ComponentModularity::synchronizeChildren(*readTest);

    // ComponentModularity::mergeChildren(*readTest);

    // System<EntityGroupInfo>([&, this](Entity &entity)
    // {
    //     ComponentModularity::synchronizeChildren(entity);
    // });
    // }


    // ComponentModularity::SynchFuncs[0].element(GG::playerEntity, Blueprint::Zweihander());

    // /****** Last Pre Loop Routines ******/
    state = AppState::run;
    std::thread physicsThreads(&Game::physicsLoop, this);

    // Sword->meshes[0]->setMenu(menu, U"sword");

    // menu.batch();
    scene2D.updateAllObjects();
    fuiBatch->batch();
    fuiBatch->state.frustumCulled = false;
    SSAO.disable();
    Bloom.disable();

    glPointSize(3.f);

    // scene.add(SkeletonHelperRef(new SkeletonHelper(GG::playerEntity->comp<SkeletonAnimationState>())));

    // ObjectGroupRef dummy = Loader<ObjectGroup>::get("Dummy").copy();
    // dummy->state.setPosition(vec3(5, 0, 0));
    // scene.add(dummy);

    

    std::string k = InputManager::getInputKeyString("toggle hud");

    std::cout << "Press " << k << " to toggle HUD\n";



    // PlottingHelperRef plottest(new PlottingHelper(vec4(1), 256));
    // scene2D.add(plottest);
    // plottest->state.setPositionZ(0.5);

    /******  Main Loop ******/
    while (state != AppState::quit)
    {
        mainloopStartRoutine();

        for (GLFWKeyInfo input; inputs.pull(input); userInput(input), InputManager::processEventInput(input))
            ;

        InputManager::processContinuousInputs();

        // plottest->push(globals.appTime.getDeltaMS());
        // plottest->updateData();

        static unsigned int itcnt = 0;
        itcnt++;
        if (doAutomaticShaderRefresh)
        {
            if (itcnt % 144 == 0)
            {
                system("clear");
                std::cout << TERMINAL_INFO << "Refreshing ALL shaders...\n" << TERMINAL_RESET;
                finalProcessingStage.reset();
                Bloom.getShader().reset();
                SSAO.getShader().reset();
                depthOnlyMaterial->reset();
                skyboxMaterial->reset();

                ui.fontMaterial->reset();
                defaultSUIMaterial->reset();

                for (auto &m : Loader<MeshMaterial>::loadedAssets)
                    m.second->reset();
            }
        }


        /****** SUN / TIMEOF DAY LOGIC GESTION 
         * 
         *  TODO : move in another place
         * 
         * ***/
        {
            if (enableTime) {
                GG::timeOfDay += globals.appTime.getDelta() * 5.f;
                GG::timeOfDay = fmod(GG::timeOfDay, 24.f);
            }

            float todayNorm = GG::timeOfDay/24.f;

            // float theta = PI * (todayNorm * 2.f - 1.f);
            // float phi = 0;

            // Parameters for sun movement
            float latitude = radians(45.0f); // Example latitude (e.g., 45 degrees)
            float declination = radians(23.5f); // Max tilt of Earth's axis
            float maxElevation = PI / 2 - fabs(latitude - declination); // Adjust for seasonality if needed

            // Calculate theta (elevation) and phi (azimuth)
            float theta = maxElevation * sin(todayNorm * PI * 2.f - PI / 2.f); // Shift to center day at noon (PI/2 shift)
            float phi = todayNorm * 2.f * PI; // Azimuth angle across the sky

            sunDir = PhiThetaToDir(vec2(phi, theta));
            sun->setDirection(-sunDir);



            sun->setIntensity(smoothstep(-0.1f, 0.5f, theta));

            sun->setColor(
                mix(
                    vec3(255, 198, 0)/255.f,
                    vec3(1, 1, 1),
                    smoothstep(0.f, (float)PI/6.f, theta)
                )
            );

            ambientLight = vec3(0.07);
        }




        WidgetUI_Context uiContext = WidgetUI_Context(&ui);
        updateEntityCursor(globals.mousePosition(), globals.mouseLeftClickDown(), globals.mouseLeftClick(), uiContext);

        SubApps::UpdateApps();
        /* TODO : remove */
        // ComponentModularity::synchronizeChildren(first);
        ComponentModularity::synchronizeChildren(gameScreenWidget);
        updateWidgetsStyle();

        // float maxSlow = player1.getStats().reflexMaxSlowFactor;
        // float reflex = player1.getInfos().state.reflex;
        // globals.simulationTime.speed = maxSlow + (1.f - maxSlow) * (1.f - reflex * 0.01);

        // float scroll = globals.mouseScrollOffset().y;
        // float &preflex = GG::playerUniqueInfos->infos.state.reflex;
        // if (GG::playerUniqueInfos)
        //     preflex = clamp(preflex + scroll * 5.f, 0.f, 100.f);
        // globals.clearMouseScroll();

        effects.update();

        if (GG::playerEntity && GG::playerEntity->comp<EntityStats>().alive)
            GG::playerEntity->comp<EntityState3D>().lookDirection = camera.getDirection();

        /***** Updating animations
        *****/
        if (!globals.simulationTime.isPaused())
            System<SkeletonAnimationState, AnimationControllerRef>([&](Entity &entity) {
                auto &s = entity.comp<SkeletonAnimationState>();
                auto &c = entity.comp<AnimationControllerRef>();

                c->update(globals.simulationTime.getDelta());
                c->applyKeyframes(s);
                s.skeleton->applyGraph(s);
                s.update();
            });

        // /***** SYNCHRONIZING MEG
        // *****/
        //     System<EntityGroupInfo>([&, this](Entity &entity)
        //     {
        //         ComponentModularity::synchronizeChildren(entity);
        //     });

        /***** DEMO DEPLACEMENT SYSTEM
        *****/
        System<EntityState3D, EntityDeplacementState, DeplacementBehaviour>([&, this](Entity &entity) {
            auto &s = entity.comp<EntityState3D>();
            auto &ds = entity.comp<EntityDeplacementState>();

            if (&entity == this->dialogueControl.interlocutor.get())
            {
                vec3 dir = GG::playerEntity->comp<EntityState3D>().position - s.position;
                float dist = length(dir);
                s.lookDirection = ds.wantedDepDirection = dir / dist;
                ds.speed = dist < 1.5f ? 0.f : ds.speed;
                return;
            }

            switch (entity.comp<DeplacementBehaviour>())
            {
            case DeplacementBehaviour::DEMO: {
                float time = globals.simulationTime.getElapsedTime();
                float angle =
                    PI * 2.f *
                        random01Vec2(vec2(time - mod(time, 0.5f + random01Vec2(vec2(entity.ids[ENTITY_LIST]))))) +
                    entity.ids[ENTITY_LIST];
                ds.wantedDepDirection.x = cos(angle);
                ds.wantedDepDirection.z = sin(angle);
                ds.speed = 1;
            }
            break;

            case DeplacementBehaviour::STAND_STILL:
                ds.speed = 0;
                break;

            default:
                break;
            }
        });

        /***** ATTACH THE MODEL TO THE ENTITY STATE *****/

        PG::physicInterpolationMutex.lock();
        float physicInterpolationValue =
            clamp((PG::PG::physicInterpolationTick.timeSinceLastTickMS() * physicsTicks.freq), 0.f, 1.f);
        PG::physicInterpolationMutex.unlock();

        System<EntityModel, EntityState3D>([&, this](Entity &entity) {
            auto &s = entity.comp<EntityState3D>();
            EntityModel &model = entity.comp<EntityModel>();

            if (s.usequat && s.position == model->state.position && s.quaternion == model->state.quaternion)
                return;

            vec3 &dir = s.lookDirection;

            if (s.usequat)
            {
                // model->state.setRotation(glm::eulerAngles(s.quaternion));
                // model->state.setQuaternion(s.quaternion);
                model->state.setQuaternion(
                    glm::eulerAngles(slerp(s._PhysicTmpQuat, s.quaternion, physicInterpolationValue)));
            }
            else if (dir.x != 0.f || dir.z != 0.f)
            {
                quat wantQuat = quatLookAt(normalize(dir * vec3(-1, 0, -1)), vec3(0, 1, 0));
                quat currQuat = quat(model->state.rotation);
                float a = min(1.f, globals.simulationTime.getDelta() * 5.f);

                quat resQuat = slerp(currQuat, wantQuat, a);

                model->state.setRotation(glm::eulerAngles(resQuat));
            }

            // model->state.setPosition(s.position);

            if (s._PhysicTmpPos == vec3(UNINITIALIZED_FLOAT))
                model->state.setPosition(s.position);
            else
                model->state.setPosition(mix(s._PhysicTmpPos, s.position, physicInterpolationValue));

            if (GG::playerEntity && &entity == GG::playerEntity.get() && globals._currentController != &this->spectator)
            {
                // vec3 pos = this->playerControl.cameraShiftPos;

                model->state.update();
                // model->state.setPosition(s.position);
                model->state.setPosition(mix(s._PhysicTmpPos, s.position, physicInterpolationValue));

                auto &s = entity.comp<SkeletonAnimationState>();

                const int headBone = 18;
                vec4 animPos = s[headBone] * inverse(s.skeleton->at(headBone).t) * vec4(0, 0, 0, 1);

                this->camera.setPosition(vec3(model->state.modelMatrix * animPos) + vec3(0, 0.2, 0));
            }
        });

        /***** UPDATING PHYSICS HELPER *****/
#ifdef SANCTIA_DEBUG_PHYSIC_HELPER
        System<PhysicsHelpers, RigidBody, EntityState3D>([&, this](Entity &entity) {
            auto &model = entity.comp<PhysicsHelpers>();
            auto &s = entity.comp<EntityState3D>();

            if (!s.physicActivated)
            {
                // model->state.hide = ModelStateHideStatus::HIDE;
                /* clangy but works for debug models */
                model->state.setPosition(vec3(-1e9));
            }
            else
            {
                model->state.hide = ModelStatus::SHOW;

                switch (entity.comp<RigidBody>()->getType())
                {
                case rp3d::BodyType::DYNAMIC:
                    model->state.setPosition(mix(s._PhysicTmpPos, s.position, physicInterpolationValue));
                    model->state.setQuaternion(
                        glm::eulerAngles(slerp(s._PhysicTmpQuat, s.quaternion, physicInterpolationValue)));

                    break;

                default:
                    model->state.setPosition(s.position);
                    model->state.setQuaternion(s.quaternion);
                    break;
                }
            }
        });
#endif

        /***** KILLS VIOLENTLY UNALIVE ENTITIES *****/
        System<EntityStats>([](Entity &entity) {
            /* TODO : move entity despawn code elswhere*/
            // void *ptr = &entity;
            // if(!entity.comp<EntityStats>().alive)
            // {
            //     GG::entities.erase(
            //         std::remove_if(
            //         GG::entities.begin(),
            //         GG::entities.end(),
            //         [ptr](EntityRef &e){return e.get() == ptr;}
            //     ), GG::entities.end());
            // }
            if (!entity.comp<EntityStats>().alive)
            {
                entity.removeComp<DeplacementBehaviour>();
                entity.comp<ActionState>().lockType = ActionState::LockedDeplacement::SPEED_ONLY;
                entity.comp<ActionState>().lockedMaxSpeed = 0;
                entity.comp<EntityDeplacementState>().speed = 0;
                entity.removeComp<AgentState>();
            }
        });

        ManageGarbage<Items>();
        ManageGarbage<EntityModel>();
        ManageGarbage<PhysicsHelpers>();
        ManageGarbage<WidgetBackground>();
        ManageGarbage<WidgetSprite>();
        ManageGarbage<WidgetText>();

        GlobalComponentToggler<PhysicsHelpers>::updateALL();
        GlobalComponentToggler<InfosStatsHelpers>::updateALL();

        mainloopPreRenderRoutine();

        /* UI & 2D Render */
        glEnable(GL_BLEND);
        glEnable(GL_FRAMEBUFFER_SRGB);

        glDepthFunc(GL_GEQUAL);
        glEnable(GL_DEPTH_TEST);

        scene2D.updateAllObjects();
        fuiBatch->batch();
        screenBuffer2D.activate();
        if (!hideHUD)
        {
            scene2D.cull();
            scene2D.draw();
        }
        screenBuffer2D.deactivate();

        /* 3D Pre-Render */
        glDisable(GL_FRAMEBUFFER_SRGB);
        glDisable(GL_BLEND);
        glDepthFunc(GL_GREATER);
        glEnable(GL_DEPTH_TEST);

        scene.updateAllObjects();

        /***** Items follow the skeleton
        *****/
        System<Items, SkeletonAnimationState, EntityModel>([](Entity &entity) {
            auto &it = entity.comp<Items>();
            auto &sa = entity.comp<SkeletonAnimationState>();
            auto &m = entity.comp<EntityModel>()->state.modelMatrix;

            for (auto &i : it.equipped)
                if (i.item.get())
                {
                    mat4 &t = i.item->comp<ItemTransform>().mat;
                    t = m * sa[i.id] * inverse(sa.skeleton->at(i.id).t);

                    if (i.item->hasComp<EntityModel>())
                    {
                        auto &model = i.item->comp<EntityModel>();

                        model->state.modelMatrix = t;

                        model->update(true);

                        t = model->getChildren()[0]->state.modelMatrix;
                    }

                    auto &is = i.item->comp<EntityState3D>();
                    is.quaternion = quat(t);
                    // is.position = vec3(t[0].w, t[1].w, t[2].w);
                    is.position = vec3(t[3]);
                    is.usequat = true;
                }
        });

        scene.generateShadowMaps();
        globals.currentCamera = &camera;
        renderBuffer.activate();
        scene.cull();

        if (wireframe)
        {
            glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
            skybox->state.hide = ModelStatus::HIDE;
        }
        else
        {
            glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
            skybox->state.hide = ModelStatus::SHOW;
        }

        /* 3D Early Depth Testing */
        // scene.depthOnlyDraw(*globals.currentCamera, true);
        // glDepthFunc(GL_EQUAL);

        /* 3D Render */
        EnvironementMap.bind(4);

        scene.genLightBuffer();
        scene.draw();
        renderBuffer.deactivate();

        glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);

        /* Post Processing */
        renderBuffer.bindTextures();
        SSAO.render(*globals.currentCamera);
        Bloom.render(*globals.currentCamera);

        /* Final Screen Composition */
        glViewport(0, 0, globals.windowWidth(), globals.windowHeight());
        finalProcessingStage.activate();
        sun->shadowMap.bindTexture(0, 6);
        screenBuffer2D.bindTexture(0, 7);
        globals.drawFullscreenQuad();

        /* Main loop End */
        mainloopEndRoutine();
    }

    physicsThreads.join();

    GG::entities.clear();
    PG::common.destroyPhysicsWorld(PG::world);
}


