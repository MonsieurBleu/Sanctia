#include "ECS/ComponentTypeScripting.hpp"
#include <fstream>
#include <thread>

#include <Utils.hpp>
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
#include <glm/gtc/quaternion.hpp>

#include <AnimationBlueprint.hpp>
#include <Graphics/Animation.hpp>
#include <Graphics/Skeleton.hpp>

#include <PhysicsGlobals.hpp>
#include <Subapps.hpp>
#include <Settings.hpp>

#include <Scripting/ScriptInstance.hpp>
#include <SanctiaLuaBindings.hpp>

#include <Scripting/ScriptInstance.hpp>


#include <FenceGPU.hpp>

void Game::mainloop()
{
    /****** Loading Script Related States ******/
    threadStateName = "Main Thread";
    threadState.open_libraries(
        sol::lib::base, 
        sol::lib::coroutine, 
        sol::lib::string, 
        sol::lib::io,
        sol::lib::math,
        sol::lib::jit
    );
    // threadState.set_exception_handler(&my_exception_handler);

    SanctiaLuaBindings::bindAll(threadState);
    std::thread physicsThreads(&Game::physicsLoop, this);

    /****** Loading Models and setting up the scene ******/
    globals.simulationTime.pause();
    
    GG::skybox = newModel(skyboxMaterial);
    
    GG::skybox->loadFromFolder("data/commons/models/skybox/", false, false);

    // GG::skybox->invertFaces = true;
    GG::skybox->depthWrite = true;
    GG::skybox->state.frustumCulled = false;
    GG::skybox->state.scaleScalar(1E6);
    GG::skybox->uniforms.add(ShaderUniform(&GG::skyboxType, 32));
    scene.add(GG::skybox);

    // Texture2D EnvironementMap = Texture2D().loadFromFile("data/commons/HDRIs/quarry_cloudy_2k.jpg").generate();
    // Texture2D EnvironementMap = Loader<Texture2D>::get("IndoorEnvironmentHDRI004_4K-TONEMAPPED");
    Texture2D EnvironementMap = Loader<Texture2D>::get("IndoorEnvironmentHDRI008_4K-TONEMAPPED");
    

    SceneDirectionalLight sunLight = newDirectionLight(DirectionLight()
                                                      // .setColor(vec3(0xFF, 0xBF, 0x7F) / vec3(255))
                                                      .setColor(vec3(1))
                                                      .setDirection(normalize(vec3(-1.0, -1.0, 0.0)))
                                                      .setIntensity(1.0));

    GG::sun = sunLight;

    GG::sun->cameraResolution = vec2(8192);
    GG::sun->shadowCameraSize = vec2(0, 0);
    GG::sun->activateShadows();
    scene.add(sunLight);

    GG::moon = newDirectionLight(DirectionLight()
        .setColor(0.25f*vec3(0.6, 0.9, 1.0)));
    // scene.add(GG::moon);

    GG::moon->cameraResolution = vec2(8192);
    GG::moon->shadowCameraSize = vec2(256);
    // GG::moon->activateShadows();

    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glEnable(GL_DEPTH_TEST);
    glLineWidth(1.0);


    GlobalComponentToggler<LevelOfDetailsInfos>::activated = true;

    // renderBuffer.clearColor = VulpineColorUI::DarkBackgroundColor2;


    /****** Setting Up Debug UI *******/
    FastUI_context ui(fuiBatch, FUIfont, scene2D, defaultFontMaterial);
    ui.spriteMaterial = Loader<MeshMaterial>::get("sprite");
    VulpineBlueprintUI::UIcontext = WidgetUI_Context{&ui};

    ui.colorTitleBackground.a = 0.9f;

    float widgetTileSPace = 0.01;
    float controLWidgetSize = 0.08f;

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
                    , WidgetBox(vec2(-1, 1 - widgetTileSPace), vec2(-1.1 + widgetTileSPace, -1 - widgetTileSPace))
                    , WidgetBackground()
                    , WidgetStyle().setbackgroundColor1(VulpineColorUI::DarkBackgroundColor1).setbackGroundStyle(UiTileType::SQUARE_ROUNDED)
                    ), 
            EDITOR::MENUS::AppControl = 
                newEntity("Current Application Controls"
                    , WidgetUI_Context{&ui}
                    , WidgetState()
                    , WidgetBox(vec2(-1, 1 - widgetTileSPace), vec2(1.f + widgetTileSPace, 1.f + widgetTileSPace + controLWidgetSize))
                    , WidgetBackground()
                    , WidgetStyle().setbackgroundColor1(VulpineColorUI::DarkBackgroundColor1).setbackGroundStyle(UiTileType::SQUARE_ROUNDED)
                    ),
            EDITOR::MENUS::GlobalControl = 
                newEntity("Global Controls"
                    , WidgetUI_Context{&ui}
                    , WidgetState()
                    , WidgetBox(vec2(-1, 1 - widgetTileSPace), vec2(1.f + 2.f*widgetTileSPace + controLWidgetSize, 1.f + 2.f*widgetTileSPace + 2.f*controLWidgetSize))
                    , WidgetBackground()
                    , WidgetStyle().setbackgroundColor1(VulpineColorUI::DarkBackgroundColor1).setbackGroundStyle(UiTileType::SQUARE_ROUNDED)
                    ),
            EDITOR::MENUS::GlobalInfos =
                newEntity("Global Informations"
                    , WidgetUI_Context{&ui}
                    , WidgetState()
                    , WidgetBox(vec2(-1, 1 - widgetTileSPace), vec2(1.f + 3.f*widgetTileSPace + 2.f*controLWidgetSize, 1.9-widgetTileSPace))
                    , WidgetBackground()
                    , WidgetStyle().setbackgroundColor1(VulpineColorUI::DarkBackgroundColor1)
                    ),
            EDITOR::MENUS::AppMenu =
                newEntity("Current Application Menus"
                    , WidgetUI_Context{&ui}
                    , WidgetState()
                    , WidgetBox(vec2(-2 + widgetTileSPace, -1 - widgetTileSPace), vec2(-1.1 + widgetTileSPace, 1.9-widgetTileSPace))
                    , WidgetBackground()
                    , WidgetStyle().setbackgroundColor1(VulpineColorUI::DarkBackgroundColor1)
                    ),
                }
            )
        );

    gameScreenWidget->set<WidgetBox>(WidgetBox(vec2(-0.33333, 1), vec2(-0.933333, 0.40)));

    finalProcessingStage.addUniform(ShaderUniform((vec4 *)&gameScreenWidget->comp<WidgetBox>().displayMin, 12));

    // EDITOR::MENUS::GlobalInfos->comp<WidgetStyle>().setautomaticTabbing(1);

    // for(int i = 0; i < 5; i++)
    // ComponentModularity::addChild(*EDITOR::MENUS::GlobalInfos,
    //     VulpineBlueprintUI::Toggable("Ingo Stat Helper"
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
    //     VulpineBlueprintUI::TimerPlot(globals.appTime, VulpineColorUI::HightlightColor1)
    // );

    // ComponentModularity::addChild(*EDITOR::MENUS::GlobalInfos,
    //     VulpineBlueprintUI::TimerPlot(physicsTimer, VulpineColorUI::HightlightColor2)
    // );

    // ComponentModularity::addChild(*EDITOR::MENUS::GlobalInfos,
    //     VulpineBlueprintUI::TimerPlot(globals.cpuTime, VulpineColorUI::HightlightColor3)
    // );

    // ComponentModularity::addChild(*EDITOR::MENUS::GlobalInfos,
    //     VulpineBlueprintUI::TimerPlot(globals.gpuTime, VulpineColorUI::HightlightColor4)
    // );

    float TitleTabSize = 0.775f;

    EntityRef GlobalInfosTitleTab = newEntity("Global Infos Title Tab"
        , WidgetUI_Context{&ui}
        , WidgetState()
        , WidgetBox(vec2(-1, 1), vec2(-1, -TitleTabSize))
        , WidgetBackground()
        , WidgetStyle()
            .setautomaticTabbing(1)
            .setbackgroundColor1(VulpineColorUI::DarkBackgroundColor1)
            .setbackGroundStyle(UiTileType::SQUARE_ROUNDED)
    );
    
    EntityRef GlobalInfosSubTab = newEntity("Global Infos Sub Tab"
        , WidgetUI_Context{&ui}
        // , WidgetState()
        , WidgetBox(vec2(-1, 1), vec2(-TitleTabSize, 1))
        // , WidgetBackground()
        , WidgetStyle()
            // .setautomaticTabbing(1)
            // .setbackgroundColor1(VulpineColorUI::DarkBackgroundColor1)
            // .setbackGroundStyle(UiTileType::SQUARE_ROUNDED)
    );



    VulpineBlueprintUI::AddToSelectionMenu(
        GlobalInfosTitleTab, GlobalInfosSubTab, 
        Blueprint::EDITOR_ENTITY::INO::GlobalBenchmarkScreen(),
        "Global Benchmark", "icon_chrono"
    );

    // VulpineBlueprintUI::AddToSelectionMenu(
    //     GlobalInfosTitleTab, GlobalInfosSubTab, 
    //     Blueprint::EDITOR_ENTITY::INO::DebugConsole(),
    //     "Debug Console", ""
    // );

    VulpineBlueprintUI::AddToSelectionMenu(
        GlobalInfosTitleTab, GlobalInfosSubTab, 
        Blueprint::EDITOR_ENTITY::INO::AmbientControls(),
        "Ambient Controls", ""
    );

    VulpineBlueprintUI::AddToSelectionMenu(
        GlobalInfosTitleTab, GlobalInfosSubTab,
        VulpineBlueprintUI::SceneInfos(scene),
        "3D Scene Infos", ""
    );

    VulpineBlueprintUI::AddToSelectionMenu(
        GlobalInfosTitleTab, GlobalInfosSubTab,
        VulpineBlueprintUI::SceneInfos(scene2D),
        "2D Scene Infos", ""
    );

    std::unordered_map<std::string, EntityRef> activeInputsList;

    VulpineBlueprintUI::AddToSelectionMenu(
        GlobalInfosTitleTab, GlobalInfosSubTab,
        VulpineBlueprintUI::StringListSelectionMenu(
            "Active Inputs List",
            activeInputsList,
            [](Entity *e, float f){},
            [](Entity *e){
                
                if(e->has<EntityGroupInfo>())
                {
                    auto parent = e->comp<EntityGroupInfo>().parent;

                    if(parent && parent->comp<EntityGroupInfo>().children.size() == 1)
                    {
                        std::string &fullstr = e->comp<EntityInfos>().name;
                        std::string category, name, inputstr;

                        int endlinecnt = 0;
                        for(auto c : fullstr)
                        {
                            if(c == '\n')
                                endlinecnt ++;
                            else
                            if(endlinecnt == 0)
                                category += c;
                            else
                            if(endlinecnt == 1)
                                name += c;
                            else
                                inputstr += c;
                        }
                        
                        parent->comp<WidgetStyle>().setautomaticTabbing(0);

                        ComponentModularity::addChild(*parent, newEntity("Category Str"
                            , UI_BASE_COMP
                            , WidgetBox(vec2(-1, -0.5), vec2(-1, 1))
                            , WidgetText(UFTconvert.from_bytes(category))
                            , WidgetStyle()
                                .settextColor1(VulpineColorUI::HightlightColor1)
                        ));
                        
                        auto &box = e->comp<WidgetBox>();
                        box.set(vec2(-0.5, 0.5), vec2(box.initMin.y, box.initMax.y));
                        e->comp<WidgetText>().text = UFTconvert.from_bytes(name);

                        ComponentModularity::addChild(*parent, newEntity("Control Str"
                            , UI_BASE_COMP
                            , WidgetBox(vec2(0.5, 1), vec2(-1, 1))
                            , WidgetText(UFTconvert.from_bytes(inputstr))
                            , WidgetStyle()
                                .settextColor1(VulpineColorUI::HightlightColor2)
                        ));
                    }
                }
                
                if(e->comp<EntityGroupInfo>().children.empty())
                {

                    
                }

                // e->comp<WidgetText>().mesh->align = StringAlignment::CENTERED;

                return 0.f;
            },
            -2.f
        ),
        "Controls Helper", ""
    );


    /* TODO : finish*/
    // VulpineBlueprintUI::AddToSelectionMenu(
    //     GlobalInfosTitleTab, GlobalInfosSubTab,

    //     newEntity("Scenes Infos Menu", 
    //         , WidgetUI_Context{&ui}
    //         , WidgetState()
    //         , WidgetBox()
    //         , Widget

    //     ),

    //     // VulpineBlueprintUI::SceneInfos(scene),

    //     "Scenes Infos", "VulpineIcon"
    // );

    GlobalInfosTitleTab->comp<EntityGroupInfo>().children[0]->comp<WidgetState>().statusToPropagate = ModelStatus::SHOW;


    ComponentModularity::addChild(*EDITOR::MENUS::GlobalInfos, GlobalInfosTitleTab);
    ComponentModularity::addChild(*EDITOR::MENUS::GlobalInfos, GlobalInfosSubTab);

    // EDITOR::MENUS::AppMenu->comp<WidgetStyle>().setautomaticTabbing(1);

    // for(int i = 0; i < 1; i++)
    // ComponentModularity::addChild(*EDITOR::MENUS::AppMenu,
    //     VulpineBlueprintUI::ColorSelectionScreen(
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

    // EDITOR::MENUS::AppControl->comp<WidgetStyle>().setautomaticTabbing(1);
    EDITOR::MENUS::AppChoice->comp<WidgetStyle>().setautomaticTabbing(1);
    EDITOR::MENUS::GlobalControl->comp<WidgetStyle>().setautomaticTabbing(1);

    // ComponentModularity::addChild(*EDITOR::MENUS::GlobalControl,
    //     VulpineBlueprintUI::ValueInputSlider(
    //         "moon orbit time", 
    //         0, 1, 100, 
    //         [](Entity* e, float v)
    //         {
    //             float t = v;
    //             GG::moonOrbitTime = t;
    //         },
    //         [](Entity *e)
    //         {
    //             float t = GG::moonOrbitTime;
    //             return t;
    //         },
    //         [](std::u32string text)
    //         {
    //             float t = u32strtof2(text, GG::moonOrbitTime);
    //             GG::moonOrbitTime = t;
    //         }, 
    //         []()
    //         {
    //             float t = GG::moonOrbitTime;
    //             return ftou32str(t);
    //         }
    //     )
    // );

    ComponentModularity::addChild(*EDITOR::MENUS::GlobalControl,
        VulpineBlueprintUI::Toggable(
            "Entity Infos Stats Helper", 
            "icon_idcard",
            [&](Entity *e, float v)
            {
                GlobalComponentToggler<InfosStatsHelpers>::activated =
                    !GlobalComponentToggler<InfosStatsHelpers>::activated;
            },
            [&](Entity *e)
            {
                return GlobalComponentToggler<InfosStatsHelpers>::activated  ? 0.f : 1.f;
            }
        )
    );


    ComponentModularity::addChild(*EDITOR::MENUS::GlobalControl,
        VulpineBlueprintUI::Toggable(
            "Entity Physic Helper", 
            "icon_hitbox",
            [&](Entity *e, float v)
            {
                GlobalComponentToggler<PhysicsHelpers>::activated =
                    !GlobalComponentToggler<PhysicsHelpers>::activated;
            },
            [&](Entity *e)
            {
                return GlobalComponentToggler<PhysicsHelpers>::activated  ? 0.f : 1.f;
            }
        )
    );

    {
        ComponentModularity::addChild(*EDITOR::MENUS::GlobalControl,
            VulpineBlueprintUI::Toggable(
                "Bloom", 
                "",
                [&](Entity *e, float v)
                {
                    paintShaderPass.enableBloom = !paintShaderPass.enableBloom;
                },
                [&](Entity *e)
                {
                    return paintShaderPass.enableBloom ? 0.f : 1.f;
                }
            )
        );
    }
    {
        ComponentModularity::addChild(*EDITOR::MENUS::GlobalControl,
            VulpineBlueprintUI::Toggable(
                "AO", 
                "",
                [&](Entity *e, float v)
                {
                    paintShaderPass.enableAO = !paintShaderPass.enableAO;
                },
                [&](Entity *e)
                {
                    return paintShaderPass.enableAO? 0.f : 1.f;
                }
            )
        );
    }

    // ComponentModularity::addChild(*EDITOR::MENUS::GlobalControl,
    //     VulpineBlueprintUI::Toggable(
    //         "Physic Interpolation", 
    //         "",
    //         [](Entity *e, float v)
    //         {
    //             PG::doPhysicInterpolation = !PG::doPhysicInterpolation;
    //         },
    //         [](Entity *e)
    //         {
    //             return PG::doPhysicInterpolation ? 0.f : 1.f;
    //         }
    //     )
    // );

    // ComponentModularity::addChild(*EDITOR::MENUS::GlobalControl,
    //     VulpineBlueprintUI::Toggable(
    //         "Shader Hot Reload", 
    //         "",
    //         [](Entity *e, float v)
    //         {
    //             Game::doAutomaticShaderRefresh = !Game::doAutomaticShaderRefresh;
    //         },
    //         [](Entity *e)
    //         {
    //             return Game::doAutomaticShaderRefresh ? 0.f : 1.f;
    //         }
    //     )
    // );

    // ComponentModularity::addChild(*EDITOR::MENUS::GlobalControl,
    //     VulpineBlueprintUI::Toggable(
    //         "Script Hot Reload", 
    //         "",
    //         [](Entity *e, float v)
    //         {
    //             Game::doScriptHotReload = !Game::doScriptHotReload;
    //         },
    //         [](Entity *e)
    //         {
    //             return Game::doScriptHotReload ? 0.f : 1.f;
    //         }
    //     )
    // );

    ComponentModularity::addChild(*EDITOR::MENUS::GlobalControl,
        VulpineBlueprintUI::Toggable(
            "Pré-Alpha World Regions", 
            "",
            [&](Entity *e, float v)
            {
                worldRegionHelperEnlable = worldRegionHelperEnlable ? 0 : 1;
            },
            [&](Entity *e)
            {
                return worldRegionHelperEnlable ? 0.f : 1.f;
            }
        )
    );

    
    // Apps::MainGameApp testsubapps1;
    Apps::EntityCreator entityCreator;
    Apps::AssetListViewer assetView;
    Apps::MaterialViewerApp materialView;
    
    Apps::CombatsApp combatsApps;
    Apps::AnimationApp animationViewer;
    
    // Apps::EventGraphApp eventGraph;
    Apps::SceneMergeApp sceneMerge;
    Apps::LunaTesting lunaTest;
    Apps::MovementDemo movementDemo;


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


    // Sword->meshes[0]->setMenu(menu, U"sword");

    // menu.batch();
    scene2D.updateAllObjects();
    fuiBatch->batch();
    fuiBatch->state.frustumCulled = false;
    // SSAO.disable();
    // Bloom.disable();

    glPointSize(3.f);

    // scene.add(SkeletonHelperRef(new SkeletonHelper(GG::playerEntity->comp<SkeletonAnimationState>())));

    // ObjectGroupRef dummy = Loader<ObjectGroup>::get("Dummy").copy();
    // dummy->state.setPosition(vec3(5, 0, 0));
    // scene.add(dummy);

    

    if (!Settings::lastOpenedApp.empty())
    {
        SubApps::switchTo(Settings::lastOpenedApp);      
    }


    // Loader<ScriptInstance>::get("test").run();

    // for (auto &m : Loader<MeshMaterial>::loadingInfos)
    //     Loader<MeshMaterial>::get(m.first);


    /******  Main Loop ******/
    while (state != AppState::quit)
    {
        // PG::doPhysicInterpolation = false;

        mainloopStartRoutine();
        mainloopPreRenderRoutine();


        /* UI & 2D Render */
        glEnable(GL_BLEND);
        glEnable(GL_FRAMEBUFFER_SRGB);

        glDepthFunc(GL_GEQUAL);
        glEnable(GL_DEPTH_TEST);
        // glDisable(GL_DEPTH_TEST);

        // FenceGPU::list["Scene 2D Preparation"] = FenceGPU();

        fuiBatch->batch();
        screenBuffer2D.activate();
        if (!hideHUD)
        {
            scene2D.cull();

            // FenceGPU::list["Scene 2D Draw"] = FenceGPU();

            scene2D.draw();
            
            glFlush();
            // FenceGPU::list["Scene 2D Draw End"] = FenceGPU();
        }
        screenBuffer2D.deactivate();

        /* 3D Pre-Render */
        glDisable(GL_FRAMEBUFFER_SRGB);
        glDisable(GL_BLEND);
        glDepthFunc(GL_GREATER);
        glEnable(GL_DEPTH_TEST);
        
        // FenceGPU::list["Scene 3D Preparation"] = FenceGPU();

        scene.generateShadowMaps();
        globals.currentCamera = &camera;
        defferedBuffer->activate();
        scene.cull();

        if (wireframe)
        {
            glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
            // GG::skybox->state.hide = ModelStatus::HIDE;
        }
        else
        {
            glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
            // GG::skybox->state.hide = ModelStatus::SHOW;
        }

        /* 3D Early Depth Testing */
        scene.depthOnlyDraw(*globals.currentCamera, true);
        glDepthFunc(GL_EQUAL);

        /* 3D Render */
        EnvironementMap.bind(4);

        scene.genLightBuffer();
        // FenceGPU::list["Scene 3D Draw"] = FenceGPU();
        scene.draw();
        // FenceGPU::list["Scene 3D Draw End"] = FenceGPU();
        defferedBuffer->deactivate();

        glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);

        // FenceGPU::list["Deffered Pass Draw"] = FenceGPU();
        paintShaderPass.render(*globals.currentCamera);

        /* Post Processing */
        defferedBuffer->bindTextures();
        // SSAO.render(*globals.currentCamera);
        // Bloom.render(*globals.currentCamera);

        // FenceGPU::list["Deffered Pass End"] = FenceGPU();

        /* Final Screen Composition */
        glViewport(0, 0, globals.windowWidth(), globals.windowHeight());
        finalProcessingStage.activate();
        GG::sun->shadowMap.bindTexture(0, 6);
        // GG::moon->shadowMap.bindTexture(0, 6);
        screenBuffer2D.bindTexture(0, 7);
        paintShaderPass.getFBO().bindTexture(2, 8);
        paintShaderPass.FBO_AO.bindTexture(0, 3);
        ShaderUniform(&GG::skyboxType, 25).activate();
        // WARNING_MESSAGE(paintShaderPass.getFBO().getNBtextures())
        globals.drawFullscreenQuad();
        glFlush();
        
        

        
        
        for (GLFWKeyInfo input; inputs.pull(input); userInput(input), InputManager::processEventInput(input));
        std::vector<GLFWKeyInfo> gamepadInputs = InputManager::pollGamepad();
        for (auto &input : gamepadInputs) userInput(input), InputManager::processEventInput(input);
        InputManager::processContinuousInputs();

        // plottest->push(globals.appTime.getDeltaMS());
        // plottest->updateData();

        // this allows us to close the window when pressing the close button
        if (glfwWindowShouldClose(globals.getWindow()))
            state = AppState::quit;


        // static unsigned int itcnt = 0;
        // itcnt++;

        float currentTime = globals.appTime.getElapsedTime();

        static float lastShaderRefreshTime = 0; 
        static float shaderRefreshLatence = 1;

        static float lastScriptRefreshTime = 0.125; // timer is offseted for scripts
        static float scriptRefreshLatence = 0.25;

        if (doAutomaticShaderRefresh)
        {
            if(currentTime-lastShaderRefreshTime > shaderRefreshLatence)
            {
                lastShaderRefreshTime = currentTime;

                NAMED_TIMER(shaderRefresh)

                for(auto &i : Shader::fileWatchers)
                {
                    i.second.second = i.second.first.hasChanged();
                }

                finalProcessingStage.reset();
                Bloom.getShader().reset();
                SSAO.getShader().reset();
                depthOnlyMaterial->reset();
                skyboxMaterial->reset();
                skyboxMaterial.depthOnly->reset();

                paintShaderPass.getShader().reset();
                paintShaderPass.copyShader.reset();
                paintShaderPass.cloudShader.reset();
                paintShaderPass.bloomShader.reset();
                paintShaderPass.aoShader.reset();

                ui.fontMaterial->reset();
                defaultSUIMaterial->reset();
                
                
                for (auto &m : Loader<MeshMaterial>::loadedAssets)
                {
                    m.second->reset();

                    if(m.second.depthOnly)
                        m.second.depthOnly->reset();
                }
            }
        }   

        if(doScriptHotReload)
        {
            for(auto &i : Loader<ScriptInstance>::loadedAssets)
            {
                if(i.second.filewatcher.hasChanged())
                {
                    i.second.triggerRecompileOnNextRun();
                }
            }
        }

        /****** Refreshing Controls List helper
        ******/        
        std::vector<std::string> currentInputsListsTmp;
        for(auto &i : InputManager::eventInputs)
        {
            if(!i.activated) continue;

            std::string input = i.inputName + "\n**" + InputManager::getInputKeyString(i) + "**";

            bool isGlobal = true;
            for(auto &j : SubApps::getActiveAppInputs())
            {
                if(j == &i)
                {
                    input = SubApps::getActiveAppName() + "\n" + input;
                    isGlobal = false;
                    break;
                }
            }
            if(isGlobal)
                input = "~Global\n " + input;

            currentInputsListsTmp.push_back(input);

            auto elem = activeInputsList.find(input);
            
            if(elem == activeInputsList.end())
                activeInputsList[input] = EntityRef();
        }

        for(auto &i : InputManager::continuousInputs)
        {
            if(!i.activated) continue;

            std::string input = i.inputName + "\n**HOLD " + InputManager::getInputKeyString(i) + "**";

            bool isGlobal = true;
            for(auto &j : SubApps::getActiveAppInputs())
            {
                if(j == &i)
                {
                    input = SubApps::getActiveAppName() + "\n" + input;
                    isGlobal = false;
                    break;
                }
            }
            if(isGlobal)
                input = "~Global\n " + input;

            currentInputsListsTmp.push_back(input);

            auto elem = activeInputsList.find(input);
            
            if(elem == activeInputsList.end())
                activeInputsList[input] = EntityRef();
        }

        std::vector<std::string> inputsToBeRemovedTmp;
        for(auto &i : activeInputsList)
        {
            bool remove = true;
            for(auto &j : currentInputsListsTmp)
            if(i.first == j)
            {
                remove = false;
                break;
            }

            if(remove)
            {
                inputsToBeRemovedTmp.push_back(i.first);
            }
        }

        for(auto &i : inputsToBeRemovedTmp)
            activeInputsList.erase(i);

        // {

            

        //     // float theta = PI * (todayNorm * 2.f - 1.f);
        //     // float phi = 0;

        //     // Parameters for sun movement
        //     float latitude = radians(45.0f); // Example latitude (e.g., 45 degrees)
        //     float declination = radians(23.5f); // Max tilt of Earth's axis
        //     float maxElevation = PI / 2 - fabs(latitude - declination); // Adjust for seasonality if needed

        //     // Calculate theta (elevation) and phi (azimuth)
        //     float theta = maxElevation * sin(todayNorm * PI * 2.f - PI / 2.f); // Shift to center day at noon (PI/2 shift)
        //     float phi = todayNorm * 2.f * PI; // Azimuth angle across the sky

        //     sunDir = PhiThetaToDir(vec2(phi, theta));
        //     sun->setDirection(-sunDir);



        
        // }


        /****** SUN / TIME OF DAY LOGIC
         * 
         *  TODO : move in another place
         * 
         * ***/
        {
            if (GG::timeOfDayCycleEnable) {
                float timeOfDayIncrement = globals.appTime.getDelta() * GG::timeOfDaySpeed;
                GG::timeOfDay += timeOfDayIncrement;
                GG::timeOfDay = fmod(GG::timeOfDay, 24.f);

                constexpr float moonOrbitTime = 27.321661f * 24.f;
                constexpr float moonOrbitIncrementPerTimeOfDay = 1.f / moonOrbitTime;

                float moonOrbitIncrement = timeOfDayIncrement * moonOrbitIncrementPerTimeOfDay;

                GG::moonOrbitTime += moonOrbitIncrement;
            }

            float timeNorm = GG::timeOfDay/24.f;


            constexpr double distanceToSun = 149.6e9;
            constexpr double planetRadius = 6371e3;
            constexpr double axialTilt = 23.44;
            constexpr double eccentricity = 0.0167;
            constexpr double orbitTilt = 7.25;

            constexpr double latitude = 43.6109;
            constexpr double longitude = 3.8761;

            // constexpr double orbitTime = fromDayMonth(10, 4);
            constexpr double orbitTime = 0.8;

            // adjust for sidereal time 
            double adjustedTime = fmod(timeNorm - 0.8 - (1.5f/24.f), 1.0);

            vec3 planetRotation = vec3(
                radians(axialTilt) * cos(adjustedTime * 2.0 * PI),
                adjustedTime * 2.0 * PI,
                radians(axialTilt) * sin(adjustedTime * 2.0 * PI)
            );

            planetRot = planetRotation;

            quat rotQuat = glm::quat(planetRotation);

            // quat axialTiltQuat = angleAxis((float)radians(axialTilt), vec3(0, 0, 1)); // Axial tilt
            // quat dailyRotationQuat = angleAxis((float)adjustedTime * 2.0f * (float)PI, vec3(0, 1, 0)); // Daily rotation

            // quat rotQuat = dailyRotationQuat * axialTiltQuat;

            // planetRot = glm::eulerAngles(rotQuat);

            vec3 surfPos = vec3(
                cos(radians(latitude)) * cos(radians(longitude)),
                sin(radians(latitude)),
                cos(radians(latitude)) * sin(radians(longitude))
            );

            vec3 surfPosTransformed = (rotQuat * surfPos) * (float)planetRadius;

            vec3 orbitPos = vec3(
                cos(orbitTime * 2.0 * PI) * distanceToSun * (1.0 - eccentricity * eccentricity),
                sin(orbitTime * 2.0 * PI) * distanceToSun * sin(radians(orbitTilt)),
                sin(orbitTime * 2.0 * PI) * distanceToSun * cos(radians(orbitTilt))
            );
            
            vec3 surfaceWorldPos = surfPosTransformed + orbitPos;
            vec3 sunPos = vec3(0);
            vec3 sunDirWorld = normalize(sunPos - surfaceWorldPos);

            
            vec3 surfaceNormal = normalize(surfPosTransformed);
            vec3 tangent = normalize(cross(surfaceNormal, vec3(0, 1, 0)));
            vec3 bitangent = normalize(cross(surfaceNormal, tangent));
            tangentSpace = mat3(tangent, surfaceNormal, bitangent); // ?? 👽
            // std::cout << "surf norm: vec3(" << surfaceNormal.x << ", " << surfaceNormal.y << ", " << surfaceNormal.z << ")\n"
            //           << "tangent: vec3(" << tangent.x << ", " << tangent.y << ", " << tangent.z << ")\n"
            //           << "bitangent: vec3(" << bitangent.x << ", " << bitangent.y << ", " << bitangent.z << ")\n"
            //           << "tangent space up: vec3(" << (tangentSpace * vec3(0, 1, 0)).x << ", " << (tangentSpace * vec3(0, 1, 0)).y << ", " << (tangentSpace * vec3(0, 1, 0)).z << ")\n\n\n\n"
            //           << std::flush;


            vec3 sunDirLocal = transpose(tangentSpace) * sunDirWorld;

            
            // sunDir = vec3(sunDirLocal.x, sunDirLocal.z, sunDirLocal.y);
            sunDir = sunDirLocal;
            planetPos = orbitPos;
            
            if(GG::useCustomSunPos)
            {
                // sunDir = GG::customSunPos;
                // sunDir = normalize(vec3(1, 1, 0));
                
                sunDir = PhiThetaToDir(PI*vec2(2.*GG::customSunPhi, GG::customSunTheta));
            }
            
            // not sure about this theta
            float theta = acos(sunDir.y);
            float sunIntensity = smoothstep(-0.2f, 0.25f, sunDir.y);
            // float sunIntensity = smoothstep(-0.2f, 0.25f, theta);            
            sunLight->setIntensity(sunIntensity);

            // std::cout << "Sun intensity " << sunIntensity << "\n";

            sunLight->setColor(
                mix(
                    vec3(255, 198, 0)/255.f,
                    vec3(1, 1, 1),
                    smoothstep(0.f, (float)PI/6.f, theta)
                )
            );

            sunLight->setDirection(-sunDir);

            // compute moon position and rotation

            constexpr double distanceToMoon = 384400e3;
            constexpr double moonOrbitTilt = 5.145 + orbitTilt;
            constexpr double moonOrbitEccentricity = 0.0549;
            

            moonPos = vec3(
                cos(GG::moonOrbitTime * 2.0 * PI) * distanceToMoon * (1.0 - moonOrbitEccentricity * moonOrbitEccentricity),
                sin(GG::moonOrbitTime * 2.0 * PI) * distanceToMoon * sin(radians(moonOrbitTilt)),
                sin(GG::moonOrbitTime * 2.0 * PI) * distanceToMoon * cos(radians(moonOrbitTilt))
            ); 

            vec3 moonDirWorld = normalize(moonPos);
            quat moonRotQuat = quatLookAt(moonDirWorld, vec3(0, 1, 0));

            moonRot = glm::eulerAngles(moonRotQuat);

            GG::moon->setDirection(-normalize(moonDirWorld * tangentSpace));
            GG::moon->setIntensity(0.25 * (1.0 - sunIntensity));
        }

        // for(int i = 0; i < 512; i++) WARNING_MESSAGE("Yooo");

        SubApps::UpdateApps();
        
        WidgetUI_Context uiContext = WidgetUI_Context(&ui);
        updateEntityCursor(globals.mousePosition(), globals.mouseLeftClickDown(), globals.mouseLeftClick(), uiContext);

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
            GG::playerEntity->comp<state3D>().lookDirection = camera.getDirection();

        /*****
            UPDATING VISUAL FATIGUE LEVEL
        */
        System<EntityStats, StainStatus>([&](Entity &entity) {
            auto &stats = entity.comp<EntityStats>();
            entity.comp<StainStatus>().fatigue = 1.0 - stats.stamina.cur/stats.stamina.max;
        });

        /*****
            Executing scripts on update
        */
        System<Script>([&](Entity &entity) {
            if (!entity.comp<Script>().isInitialized())
                entity.comp<Script>().run_OnInit(entity);

            // std::cout << entity.toStr() << "\n";

            entity.comp<Script>().run_OnUpdate(entity); 
        });

        /***** Updating animations
        *****/
        // if (!globals.simulationTime.isPaused())
        float deltaAnim = globals.simulationTime.isPaused() ? 0.f : globals.simulationTime.getDelta();
        System<SkeletonAnimationState, AnimationControllerRef>([&](Entity &entity) {
            auto &s = entity.comp<SkeletonAnimationState>();
            auto &c = entity.comp<AnimationControllerRef>();

            c->update(deltaAnim);
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
        System<state3D, MovementState, DeplacementBehaviour>([&, this](Entity &entity) {
            auto &s = entity.comp<state3D>();
            auto &ds = entity.comp<MovementState>();

            if (&entity == this->dialogueControl.interlocutor.get())
            {
                vec3 dir = GG::playerEntity->comp<state3D>().position - s.position;
                float dist = length(dir);
                s.lookDirection = ds.wantedMoveDirection = dir / dist;
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
                ds.wantedMoveDirection.x = cos(angle);
                ds.wantedMoveDirection.z = sin(angle);
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
            clamp((PG::PG::physicInterpolationTick.timeSinceLastTick() * physicsTicks.freq), 0.f, 1.f);
        PG::physicInterpolationMutex.unlock();

        System<EntityModel, state3D>([&, this](Entity &entity) {
            auto &s = entity.comp<state3D>();
            EntityModel &model = entity.comp<EntityModel>();

            if (s.usequat && s.position == model->state.position && s.quaternion == model->state.quaternion)
                return;

            vec3 dir = s.lookDirection;

            float physicInterpolationValue2 = entity.has<EntityStats>() and !entity.comp<EntityStats>().alive ? 1.f : physicInterpolationValue;
            // physicInterpolationValue2 = 1.f;

            auto n = entity.comp<AnimationControllerInfos>().c_str();
            const bool isSwordAndShield = !strcmp(n, "(Human) Sword And Shield ");
            if(isSwordAndShield)
            {
                dir = normalize(dir - 0.25f*cross(dir, vec3(0, 1, 0)));
            }

            if (s.usequat)
            {
                // model->state.setRotation(glm::eulerAngles(s.quaternion));
                // model->state.setQuaternion(s.quaternion);
                model->state.setQuaternion(
                    glm::eulerAngles(slerp(s._PhysicTmpQuat, s.quaternion, physicInterpolationValue2)));
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
                model->state.setPosition(mix(s._PhysicTmpPos, s.position, physicInterpolationValue2));

            if (GG::playerEntity && &entity == GG::playerEntity.get() && globals._currentController != &this->spectator)
            {
                // vec3 pos = this->playerControl.cameraShiftPos;

                model->state.update();
                // model->state.setPosition(s.position);
                model->state.setPosition(mix(s._PhysicTmpPos, s.position, physicInterpolationValue2));

                auto &s = entity.comp<SkeletonAnimationState>();

                const int headBone = 18;
                vec4 animPos = s[headBone] * inverse(s.skeleton->at(headBone).t) * vec4(0, 0, 0, 1);

                // animPos += vec4(0, 0, -1, 0);

                this->camera.setPosition(vec3(model->state.modelMatrix * animPos) + vec3(0, 0.2, 0));
            }
        });

        /***** UPDATING PHYSICS HELPER *****/
#ifdef SANCTIA_DEBUG_PHYSIC_HELPER
        System<PhysicsHelpers, RigidBody, state3D>([&, this](Entity &entity) {
            auto &model = entity.comp<PhysicsHelpers>();
            auto &s = entity.comp<state3D>();

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

        /***** UPDATING ENTITY STATS *****/
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

            auto &stats = entity.comp<EntityStats>();
            stats.adrenaline.cur = clamp(
                stats.adrenaline.cur - sign(stats.adrenaline.cur)*75*globals.simulationTime.getDelta(),
                stats.adrenaline.min,
                stats.adrenaline.max
            );
            if(abs(stats.adrenaline.cur) < 1)
                stats.adrenaline.cur = 0;

            float runningMod = 0;
            
            if(entity.has<MovementState>())
            {
                runningMod = 20*smoothstep(
                    entity.comp<MovementState>().walkSpeed,
                    entity.comp<MovementState>().sprintSpeed,
                    entity.comp<MovementState>().speed
                );
            }

            float adrenalineMod = 0.5*250.f*stats.adrenaline.cur/stats.adrenaline.max;
            adrenalineMod = max(adrenalineMod, 0.f);

            stats.stamina.cur = clamp(
                stats.stamina.cur + globals.simulationTime.getDelta()*(10 - runningMod + adrenalineMod),
                stats.stamina.min,
                stats.stamina.max
            );

            
            // Kill unalive entity
            if (!entity.comp<EntityStats>().alive)
            {
                // if(entity.has<Items>())
                // {
                //     entity.comp<Items>().unequip(
                //         entity,
                //         WEAPON_SLOT
                //     );
                // }

                // physicsMutex.lock();

                if(
                    entity.has<RigidBody>() 
                    && (!entity.has<MovementState>() || entity.comp<MovementState>().grounded)
                    // && entity.comp<RigidBody>()->getLinearVelocity().y == 0.f 
                )
                {
                    entity.remove<RigidBody>();
                }


                // physicsMutex.unlock();

                entity.remove<DeplacementBehaviour>();
                entity.comp<ActionState>().lockType = ActionState::LockedMovement::SPEED_ONLY;
                entity.comp<ActionState>().lockedMaxSpeed = 0;
                entity.comp<MovementState>().speed = 0;
                entity.remove<AgentState>();
            }


        });

        GG::ManageEntityGarbage();


        if(GlobalComponentToggler<PhysicsHelpers>::needUpdate() || GlobalComponentToggler<InfosStatsHelpers>::needUpdate())
        {
            physicsMutex.lock();
            GlobalComponentToggler<PhysicsHelpers>::updateALL();
            GlobalComponentToggler<InfosStatsHelpers>::updateALL();
            physicsMutex.unlock();
        }

        if(GlobalComponentToggler<LevelOfDetailsInfos>::needUpdate())
        {
            GlobalComponentToggler<LevelOfDetailsInfos>::updateALL();
        }

        scene2D.updateAllObjects();
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

                    if (i.item->has<EntityModel>())
                    {
                        auto &model = i.item->comp<EntityModel>();

                        model->state.modelMatrix = t;

                        model->update(true);

                        t = model->getChildren()[0]->state.modelMatrix;
                    }

                    auto &is = i.item->comp<state3D>();
                    is.quaternion = quat(t);
                    // is.position = vec3(t[0].w, t[1].w, t[2].w);
                    is.position = vec3(t[3]);
                    is.usequat = true;
                }
            
            if(entity.has<AnimationControllerInfos>())
            {
                auto n = entity.comp<AnimationControllerInfos>().c_str();
                auto &i = it.equipped[WEAPON_SLOT];
                auto &i2 = it.equipped[SECOND_HAND_SLOT];
                
                const bool is2HSword = !strcmp(n, "(Human) 2H Sword ");
                const bool isSwordAndShield = !strcmp(n, "(Human) Sword And Shield ");
                // const bool isSwordAndShield = false;

                // NOTIF_MESSAGE(isSwordAndShield << "\t" << is2HSword)

                /*
                    Special rule for 2H weapon that need to be attached depending on both hands
                */
                if(i.item && is2HSword)
                {
                    
                    std::vector<std::string> affectedBones = {
                            "Hand",
                            "HandThumb2",
                            "HandIndex2",
                            "HandMiddle2",
                            "HandRing2",
                            "HandPinky2"
                        };

                    vec3 pl(0);
                    vec3 pr(0);
                    float cnt = 0;
                    for(auto &i : affectedBones)
                    {
                        int idl = 0, idr = 0;
                        idl = sa.skeleton->boneNamesMap["Left" + i];
                        idr = sa.skeleton->boneNamesMap["Right" + i];

                        mat4 tl = m * sa[idl] * inverse(sa.skeleton->at(idl).t);
                        mat4 tr = m * sa[idr] * inverse(sa.skeleton->at(idr).t);

                        pl += vec3(tl * vec4(0, 0, 0, 1));
                        pr += vec3(tr * vec4(0, 0, 0, 1));
                        cnt ++;
                    }
                    pl /= cnt;
                    pr /= cnt;

                    auto &is = i.item->comp<state3D>();

                    // int iHand = sa.skeleton->boneNamesMap["RightHand"];
                    // mat4 tHand = m * sa[iHand] * inverse(sa.skeleton->at(iHand).t);

                    
                    quat frontFix = angleAxis(radians(180.f), normalize(vec3(1, 0, 0))) * angleAxis(radians(45.f), normalize(vec3(0, 0, 1)));
                    vec3 u = normalize(vec3(0, 1, 0));
                    vec3 dir = normalize(pr-pl);
                    float a = abs(dot(u, dir));
                    u = normalize(mix(u, normalize(vec3(1, 0, 0)), max(0.f, (a-0.9f)*10.f)));
                    vec3 s = normalize(cross(u, dir));
                    vec3 r = cross(dir, s);
                    is.quaternion = quat(mat3(s, r, dir)) * frontFix;
                    is.position = pr + dir*0.2f;
                    is.usequat = true;


                    if (i.item->has<EntityModel>())
                    {
                        auto &model = i.item->comp<EntityModel>();
                        model->state.setQuaternion(is.quaternion);
                        model->state.setPosition(is.position);
                        model->update(true);
                    }
                }
                if(i.item and isSwordAndShield)
                {
                    std::vector<std::string> affectedBones = {
                            "RightHand",
                            "RightHandThumb2",
                            "RightHandIndex2",
                            "RightHandMiddle2",
                            "RightHandRing2",
                            "RightHandPinky2"
                        };

                    // vec3 p(0);
                    // vec3 d(0);
                    // float cnt = 0;
                    // for(auto &i : affectedBones)
                    // {
                    //     int id = sa.skeleton->boneNamesMap[i];
                    //     mat4 t = m * sa[id] * inverse(sa.skeleton->at(id).t);
                    //     p += vec3(t * vec4(0, 0, 0, 1));
                    //     d += vec3(t * vec4(1, 0, 0, 0));
                    //     cnt ++;
                    // }
                    // p /= cnt;
                    // d = normalize(d/cnt);
                    
                    // auto &is = i.item->comp<state3D>();

                    // quat frontFix = angleAxis(radians(180.f), normalize(vec3(1, 0, 0))) * angleAxis(radians(45.f), normalize(vec3(0, 0, 1)));
                    // vec3 u = normalize(vec3(0, 0, 1));
                    // vec3 dir = d;
                    // float a = abs(dot(u, dir));
                    // u = normalize(mix(u, normalize(vec3(1, 0, 0)), max(0.f, (a-0.9f)*10.f)));
                    // vec3 s = normalize(cross(u, dir));
                    // vec3 r = cross(dir, s);
                    // is.quaternion = quat(mat3(s, r, dir)) * frontFix;
                    // // is.quaternion = angleAxis(radians(90.f), axis(is.quaternion));
                    // // NOTIF_MESSAGE(angle(is.quaternion))
                    // is.position = p + dir*0.45f;
                    // is.usequat = true;



                    vec3 p(0);
                    vec3 d(0);
                    vec3 lastp(0);
                    float cnt = 0;
                    for(auto &i : affectedBones)
                    {
                        int id = sa.skeleton->boneNamesMap[i];
                        mat4 t = m * sa[id] * inverse(sa.skeleton->at(id).t);
                        p += vec3(t * vec4(0, 0, 0, 1));
                        d += vec3(t * vec4(1, 0, 0, 0));
                        cnt ++;
                    }
                    
                    p /= cnt;
                    d = normalize(d/cnt);

                    
                    auto &is = i.item->comp<state3D>();
                    is.quaternion = quatLookAt(-d, vec3(0, 1, 0));
                    is.position = p;
                    is.usequat = true;




                    if (i.item->has<EntityModel>())
                    {
                        auto &model = i.item->comp<EntityModel>();
                        model->state.setQuaternion(is.quaternion);
                        model->state.setPosition(is.position);
                        model->update(true);
                    }
                }
                if(i2.item and isSwordAndShield)
                {
                    std::vector<std::string> affectedBones = {
                            "LeftHand",
                            "LeftHandThumb2",
                            "LeftHandIndex2",
                            "LeftHandMiddle2",
                            "LeftHandRing2",
                            "LeftHandPinky2"
                        };

                    vec3 p(0);
                    vec3 d(0);
                    vec3 lastp(0);
                    float cnt = 0;
                    for(auto &i : affectedBones)
                    {
                        int id = sa.skeleton->boneNamesMap[i];
                        mat4 t = m * sa[id] * inverse(sa.skeleton->at(id).t);
                        p += vec3(t * vec4(0, 0, 0, 1));
                        d += vec3(t * vec4(1, 0, 0, 0));
                        cnt ++;
                    }
                    
                    p /= cnt;
                    d = normalize(d/cnt);
                    
                    mat4 t = m * sa[0] * inverse(sa.skeleton->at(0).t);
                    vec3 ptmp = vec3(t * vec4(0, 0, 0, 1));
                    vec3 wf = normalize(ptmp-p);
                    wf = normalize(wf * vec3(1, 0, 1));
                    
                    auto &is = i2.item->comp<state3D>();
                    is.quaternion = quatLookAt(wf, -d);
                    is.position = p + d*0.f;
                    is.usequat = true;

                    if (i2.item->has<EntityModel>())
                    {
                        auto &model = i2.item->comp<EntityModel>();
                        model->state.setQuaternion(is.quaternion);
                        model->state.setPosition(is.position);
                        model->update(true);
                    }
                }
            }
        });

        /* Main loop End */
        mainloopEndRoutine();
    }

    Settings::bloomEnabled = Bloom.isPassEnable();
    Settings::ssaoEnabled = SSAO.isPassEnable();
    Settings::renderScale = globals.renderScale();
    Settings::lastOpenedApp = SubApps::getActiveAppName();

    // save settings to file
    Settings::save();

    physicsThreads.join();
    
    cursorHelp = 
    gameScreenWidget = 
    GlobalInfosTitleTab = 
    GlobalInfosSubTab =
    EDITOR::MENUS::GameScreen =
    EDITOR::MENUS::AppChoice =
    EDITOR::MENUS::AppControl=
    EDITOR::MENUS::AppMenu =
    EDITOR::MENUS::GlobalControl =
    EDITOR::MENUS::GlobalInfos = EntityRef();
    GG::entities.clear();
    PG::common.destroyPhysicsWorld(PG::world);


    // Prevent crashed from LuaState being destroyed before scripts
    Loader<ScriptInstance>::loadedAssets.clear(); 
}


