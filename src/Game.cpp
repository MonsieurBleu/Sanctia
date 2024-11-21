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

void Game::mainloop()
{

    /****** Loading Models and setting up the scene ******/
    ModelRef skybox = newModel(skyboxMaterial);
    skybox->loadFromFolder("ressources/models/skybox/", true, false);

    // skybox->invertFaces = true;
    // skybox->depthWrite = false;
    skybox->state.frustumCulled = false;
    skybox->state.scaleScalar(1E6);
    scene.add(skybox);

    Texture2D EnvironementMap = Texture2D().loadFromFile("ressources/HDRIs/quarry_cloudy_2k.jpg").generate();

    Blueprint::Terrain("ressources/maps/testPlayground.hdr",
                       // "ressources/maps/RuggedTerrain.hdr",
                       // "ressources/maps/generated_512x512.hdr",
                       // "ressources/maps/RT512.hdr",
                       // vec3(512, 64, 512),
                       vec3(256, 64, 256), vec3(0), 128);

    SceneDirectionalLight sun = newDirectionLight(DirectionLight()
                                                      // .setColor(vec3(0xFF, 0xBF, 0x7F) / vec3(255))
                                                      .setColor(vec3(1))
                                                      .setDirection(normalize(vec3(-1.0, -1.0, 0.0)))
                                                      .setIntensity(1.0));

    sun->cameraResolution = vec2(8192);
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
    FastUI_valueMenu menu(ui, {});

    BenchTimer tmpTimer("tmp timer");

    menu->state.setPosition(vec3(-0.9, 0.9, 0)).scaleScalar(0.9);
    globals.appTime.setMenuConst(menu);
    globals.simulationTime.setMenu(menu);
    physicsTimer.setMenu(menu);
    tmpTimer.setMenu(menu);
    globals.cpuTime.setMenu(menu);
    globals.gpuTime.setMenu(menu);
    globals.fpsLimiter.setMenu(menu);
    physicsTicks.setMenu(menu);
    sun->setMenu(menu, U"Sun");

    std::vector<FastUI_value> GameConditionsValues;

    for (auto &i : GameConditionMap)
        GameConditionsValues.push_back(
            FastUI_value((bool *)&GG::currentConditions.get(i.second), UFTconvert.from_bytes(i.first) + U"\t"));

    menu.push_back({FastUI_menuTitle(menu.ui, U"Games Conditions"), FastUI_valueTab(menu.ui, GameConditionsValues)});

    /* TODO : remove */
    menu->state.setPosition(vec3(1e9, 0.9, 0)).scaleScalar(0.9);

    /* Testing Out new ECS based UI*/
    // for(int i = 0; i < 8; i++)
    // {
    //     EntityRef uitest = newEntity("UiTest"
    //         , WidgetUI_Context{&ui}
    //         , WidgetUpdate()
    //         , WidgetBox(
    //             vec2(i/8.f    , -0.5),
    //             vec2((i+1)/8.f,  0.5))
    //         , WidgetBakground()
    //     );

    //     GG::entities.push_back(uitest);

    //     std::cout << uitest->toStr() << "\n";
    // }

    // EntityRef first;
    // {
    //     EntityRef parent;

    //     for(int i = 0; i < 8; i++)
    //     {
    //         EntityRef uitest = newEntity("UiTest"
    //             , WidgetUI_Context{&ui}
    //             , WidgetState()
    //             , WidgetBox(
    //                 vec2(-0.75, -2.25),
    //                 vec2(+0.75, -1.05))
    //             , WidgetBackground()
    //             , WidgetText(U"Bonjour, je suis le reuf. Pour vrai !")
    //             , WidgetButton(WidgetButton::Type::HIDE_SHOW_TRIGGER)
    //         );

    //         if(i == 3)
    //             uitest->comp<WidgetState>().status = ModelStateHideStatus::HIDE;

    //         if(!first)
    //         {
    //             uitest->comp<WidgetBox>() = WidgetBox(vec2(-0.9, 0.25), vec2(0.9,  1.0));
    //             first = uitest;
    //         }
    //         else
    //         {
    //             ComponentModularity::addChild(*parent, uitest);
    //         }

    //         parent = uitest;

    //         GG::entities.push_back(uitest);

    //         std::cout << uitest->toStr() << "\n";
    //     }

    //     ComponentModularity::synchronizeChildren(first);
    // }

    ui.colorTitleBackground.a = 0.9f;

    float widgetTileSPace = 0.01;

    vec3 sunDir = vec3(0.0f);
    vec3 planetRot = vec3(1.0f);
    skybox->uniforms.add(ShaderUniform(&sunDir, 21));
    skybox->uniforms.add(ShaderUniform(&planetRot, 22));

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

    EDITOR::MENUS::GlobalInfos->comp<WidgetStyle>().setautomaticTabbing(1);

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

    ComponentModularity::addChild(*EDITOR::MENUS::GlobalInfos, 
        Blueprint::EDITOR_ENTITY::INO::GlobalBenchmarkScreen()
        );

    EDITOR::MENUS::GlobalControl->comp<WidgetStyle>().setautomaticTabbing(1);

    ComponentModularity::addChild(*EDITOR::MENUS::GlobalControl,
        newEntity("Text input test"
            , WidgetUI_Context{&ui}
            , WidgetState()
            , WidgetBox(
                vec2(-0.9, -0.7),
                vec2(-0.9, +0.9)
            )
            , WidgetBackground()
            , WidgetText(U"[...]")
            , WidgetStyle()
            , WidgetButton(
                WidgetButton::Type::TEXT_INPUT
            )
        )
    );


    EDITOR::MENUS::AppMenu->comp<WidgetStyle>().setautomaticTabbing(5);

    for(int i = 0; i < 5; i++)
    ComponentModularity::addChild(*EDITOR::MENUS::AppMenu,
        Blueprint::EDITOR_ENTITY::INO::ColorSelectionScreen(
            "Sun color", 
            [&sun](){return vec3(sun->getInfos()._color);},
            [&sun](vec3 color){sun->setColor(color);}
        )
    );

    for (auto &i : Loader<Texture2D>::loadingInfos)
        std::cout << i.first << "\n";

    // EDITOR::MENUS::AppMenu->set<WidgetSprite>(WidgetSprite("icon_gizmo"));

#define TEST_ELEMENT_COMPONENT                                                                                         \
    , WidgetUI_Context{&ui}, WidgetState(),                                                                            \
        WidgetBox(vec2(-0.9, +0.9), vec2(+1.1, +2.6), WidgetBox::Type::FOLLOW_SIBLINGS_BOX), WidgetBackground(),       \
        WidgetText(), WidgetButton(WidgetButton::Type::HIDE_SHOW_TRIGGER), WidgetStyle()

    EntityRef first = newEntity(
        "first", WidgetUI_Context{&ui}, WidgetState(), WidgetBox(vec2(-0.25, +0.25), vec2(-0.9, -0.8)),
        WidgetBackground(), WidgetText(U"Titre important"),
        WidgetButton(WidgetButton::Type::HIDE_SHOW_TRIGGER
                     // WidgetButton::Type::CHECKBOX,
                     // WidgetButton::func([](float v)
                     // {
                     //     // std::cout << v << "\n";
                     //     GlobalComponentToggler<PhysicsHelpers>::activated =
                     //     !GlobalComponentToggler<PhysicsHelpers>::activated;
                     // })
                     ),
        WidgetStyle(),
        EntityGroupInfo({
            newEntity("Sous élément 1" TEST_ELEMENT_COMPONENT,
                      EntityGroupInfo({
                          newEntity("Sous élément 1-1" TEST_ELEMENT_COMPONENT),
                          newEntity("Sous élément 1-2" TEST_ELEMENT_COMPONENT),
                          newEntity("Sous élément 1-3" TEST_ELEMENT_COMPONENT),
                          newEntity("Sous élément 1-4" TEST_ELEMENT_COMPONENT,
                                    EntityGroupInfo({
                                        newEntity("Sous élément 1-4-1" TEST_ELEMENT_COMPONENT),
                                        newEntity("Sous élément 1-4-2" TEST_ELEMENT_COMPONENT),
                                        newEntity("Sous élément 1-4-3" TEST_ELEMENT_COMPONENT),
                                        newEntity("Sous élément 1-4-4" TEST_ELEMENT_COMPONENT),
                                    })),
                      })),
            newEntity("Sous élément 2" TEST_ELEMENT_COMPONENT, EntityGroupInfo({
                                                                   newEntity("Sous élément 2-1" TEST_ELEMENT_COMPONENT),
                                                                   newEntity("Sous élément 2-2" TEST_ELEMENT_COMPONENT),
                                                                   newEntity("Sous élément 2-3" TEST_ELEMENT_COMPONENT),
                                                                   newEntity("Sous élément 2-4" TEST_ELEMENT_COMPONENT),
                                                               })),
            newEntity("Sous élément 3" TEST_ELEMENT_COMPONENT), newEntity("Sous élément 4" TEST_ELEMENT_COMPONENT),
            // newEntity("Sous élément 5"
            //     TEST_ELEMENT_COMPONENT
            //     , WidgetBox(
            //         vec2(-0.9, +0.9),
            //         vec2(-2.5, -1.1),
            //         WidgetBox::Type::FOLLOW_SIBLINGS_BOX
            //     )
            // ),
        }));

    ComponentModularity::addChild(*gameScreenWidget, first);

    EDITOR::MENUS::AppControl->comp<WidgetStyle>().setautomaticTabbing(1);
    ComponentModularity::addChild(*EDITOR::MENUS::AppControl,
        Blueprint::EDITOR_ENTITY::INO::ValueInputSlider(
            "Time of day", 
            0, 24, 240, 
            [](float v)
            {
                float t = fmod(v, 24.0);
                GG::timeOfDay = t / 24.f;
            },
            []()
            {
                float t = GG::timeOfDay * 24.f;
                return t;
            },
            [](std::u32string text)
            {
                float t = u32strtof2(text, GG::timeOfDay);
                t = fmod(t, 24.0);
                GG::timeOfDay = t / 24.f;
            }, 
            []()
            {
                std::stringstream s;
                float t = GG::timeOfDay * 24.f;
                s << t;
                return UFTconvert.from_bytes(s.str());
            }
        )
    );

    bool enableTime = true;
    ComponentModularity::addChild(*EDITOR::MENUS::AppControl,
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

            

    /****** Loading Game Specific Elements *******/
    GG::currentConditions.readTxt("saves/gameConditions.txt");

    Faction::setEnemy({Faction::Type::PLAYER}, {Faction::Type::PLAYER_ENEMY});
    Faction::setEnemy({Faction::Type::PLAYER}, {Faction::Type::MONSTERS});
    Faction::setEnemy({Faction::Type::MONSTERS}, {Faction::Type::PLAYER_ENEMY});

    scene.add(Loader<MeshModel3D>::get("barrel01").copy());

    /****** Creating Demo Player *******/
    Player player1;
    GG::playerUniqueInfos = &player1;
    player1.setMenu(menu);

    ObjectGroupRef playerModel(new ObjectGroup);
    auto playerModelBody = Loader<ObjectGroup>::get("PlayerTest").copy();
    // auto playerModelBody = Loader<ObjectGroup>::get("Dummy").copy();
    playerModel->add(playerModelBody);

    GG::playerEntity = newEntity("PLayer", EntityModel{playerModel}, EntityState3D(), EntityDeplacementState(),
                                 SkeletonAnimationState(Loader<SkeletonRef>::get("Xbot")), Items{},
                                 Faction{Faction::Type::PLAYER}, EntityStats(), ActionState{});

    GG::playerEntity->comp<EntityStats>().health.max = 1e4;
    GG::playerEntity->comp<EntityStats>().health.cur = 1e4;
    GG::playerEntity->set<RigidBody>(Blueprint::Assembly::CapsuleBody(1.75, vec3(5, 15, 5), GG::playerEntity));

    Items::equip(GG::playerEntity, Blueprint::Zweihander(), WEAPON_SLOT, BipedSkeletonID::RIGHT_HAND);
    Items::equip(GG::playerEntity, Blueprint::Foot(), LEFT_FOOT_SLOT, BipedSkeletonID::LEFT_FOOT);

    GG::playerEntity->set<AnimationControllerRef>(AnimBlueprint::bipedMoveset("65_2HSword", GG::playerEntity.get()));

    scene.add(Loader<ObjectGroup>::get("PlayerTest").copy());

    // SingleStringBatchRef AttackDirectionHelper(new SingleStringBatch());
    // AttackDirectionHelper->setMaterial(Loader<MeshMaterial>::get("mdFont"));
    // AttackDirectionHelper->setFont(FUIfont);
    // AttackDirectionHelper->align = CENTERED;
    // AttackDirectionHelper->color = ColorHexToV(0xFFFFFF);
    // AttackDirectionHelper->baseUniforms.add(ShaderUniform(&AttackDirectionHelper->color, 32));
    // scene2D.add(AttackDirectionHelper);

    // SingleStringBatchRef HealthBar(new SingleStringBatch());
    // HealthBar->setMaterial(Loader<MeshMaterial>::get("mdFont"));
    // HealthBar->setFont(FUIfont);
    // HealthBar->align = StringAlignement::TO_LEFT;
    // HealthBar->color = ColorHexToV(0x00FF00);
    // HealthBar->baseUniforms.add(ShaderUniform(&HealthBar->color, 32));
    // HealthBar->state.setPosition(screenPosToModel(vec2(-0.8, -0.8)));
    // scene2D.add(HealthBar);

    // {   auto e = Blueprint::TestManequin();
    //     e->set<DeplacementBehaviour>(FOLLOW_WANTED_DIR);
    //     e->set<AgentState>({AgentState::COMBAT_POSITIONING});
    //     e->set<Target>(Target{GG::playerEntity});
    // }

    {
        auto e = Blueprint::TestManequin();
        e->set<DeplacementBehaviour>(STAND_STILL);
        vec4 &c1 = e->comp<EntityModel>()->getLights()[0]->getInfos()._color;
        c1 = vec4(ColorHexToV(0x00FF00), c1.a);
    }

    // for(int i = 0; i < 50; i++)
    // {
    //     auto e1 = Blueprint::TestManequin();
    //     auto e2 = Blueprint::TestManequin();

    //     e1->set<DeplacementBehaviour>(FOLLOW_WANTED_DIR);
    //     e1->set<AgentState>({AgentState::COMBAT_POSITIONING});
    //     e1->set<Target>(Target{e2});
    //     e1->set<Faction>({Faction::ENEMY});

    //     e2->set<DeplacementBehaviour>(FOLLOW_WANTED_DIR);
    //     e2->set<AgentState>({AgentState::COMBAT_POSITIONING});
    //     e2->set<Target>(Target{e1});
    //     e2->set<Faction>({Faction::ENEMY2});

    //     e2->comp<EntityState3D>().position.x += 50;

    //     vec4 &c1 = e1->comp<EntityModel>()->getLights()[0]->getInfos()._color;
    //     c1 = vec4(ColorHexToV(0xFFFF00), c1.a);

    //     vec4 &c2 = e2->comp<EntityModel>()->getLights()[0]->getInfos()._color;
    //     c2 = vec4(ColorHexToV(0xFF50FF), c2.a);
    // }

    // for(int i = 0; i < 0; i++)
    // {
    //     ObjectGroupRef model = newObjectGroup();
    //     model->add(CubeHelperRef(new CubeHelper(vec3(1), vec3(-1))));

    //     rp3d::RigidBody *body = PG::world->createRigidBody(rp3d::Transform(rp3d::Vector3(0 + (i%10)*0.75, 15 + i*2.1,
    //     0), rp3d::Quaternion::identity()));

    //     body->addCollider(PG::common.createBoxShape(rp3d::Vector3(1, 1, 1)), rp3d::Transform::identity());

    //     auto e = newEntity("physictest", EntityModel{model}, body, EntityState3D());
    //     GG::entities.push_back(e);

    //     e->comp<EntityDeplacementState>().wantedDepDirection = vec3(0, 0, 1);
    //     e->comp<EntityDeplacementState>().wantedSpeed = 3.0;
    //     e->comp<EntityState3D>().usequat = true;
    // }
    // {
    //     rp3d::RigidBody *body = PG::world->createRigidBody(rp3d::Transform(rp3d::Vector3(0, -15, 0),
    //     rp3d::Quaternion::identity())); auto collider =
    //     body->addCollider(PG::common.createBoxShape(rp3d::Vector3(100, 15, 100)), rp3d::Transform::identity());
    //     body->setType(rp3d::BodyType::STATIC);
    //     collider->getMaterial().setBounciness(0.f);
    //     collider->getMaterial().setFrictionCoefficient(0.5f);
    //     collider->setCollisionCategoryBits(1<<CollideCategory::ENVIRONEMENT);
    //     collider->setCollideWithMaskBits(1<<CollideCategory::ENVIRONEMENT);
    // }

    /**** Testing Entity Loading *****/
    // {
    NAMED_TIMER(EntityRW)

    EntityRW.start();

    EntityRef writeTest = Blueprint::TestManequin();
    writeTest->set<AgentState>({AgentState::COMBAT_POSITIONING});
    writeTest->set<DeplacementBehaviour>(FOLLOW_WANTED_DIR);
    // writeTest->set<Target>({GG::playerEntity});
    // EntityRef writeTest = Blueprint::Zweihander();
    VulpineTextOutputRef out(new VulpineTextOutput(1 << 16));
    DataLoader<EntityRef>::write(writeTest, out);
    out->saveAs("MannequinTest.vulpineEntity");

    VulpineTextBuffRef in(new VulpineTextBuff("MannequinTest.vulpineEntity"));
    EntityRef readTest = DataLoader<EntityRef>::read(in);
    // readTest->set<Target>({GG::playerEntity});

    VulpineTextOutputRef out2(new VulpineTextOutput(1 << 16));
    DataLoader<EntityRef>::write(readTest, out2);
    out2->saveAs("MannequinTest2.vulpineEntity");
    GG::entities.push_back(readTest);

    EntityRW.end();
    std::cout << EntityRW;

    readTest->set<EntityGroupInfo>(EntityGroupInfo());

    Entity *parent = nullptr;
    // Entity* parent = readTest.get();
    EntityRef firstChild;

    for (int i = 0; i < 16; i++)
    {
        EntityRef child = Blueprint::Zweihander();

        if (!i)
            firstChild = child;

        child->comp<RigidBody>()->setType(rp3d::BodyType::KINEMATIC);
        child->comp<RigidBody>()->setTransform(rp3d::Transform(rp3d::Vector3(0.0f, 0.2f, 0.0f),
                                                               // PG::torp3d(quat(vec3(0, cos(i/PI), 0)))
                                                               DEFQUAT));
        child->comp<RigidBody>()->setType(rp3d::BodyType::DYNAMIC);

        if (parent && parent != child.get())
        {
            ComponentModularity::addChild(*parent, child);
        }

        parent = child.get();
    }

    // ComponentModularity::synchronizeChildren(*readTest);

    // ComponentModularity::mergeChildren(*readTest);

    // System<EntityGroupInfo>([&, this](Entity &entity)
    // {
    //     ComponentModularity::synchronizeChildren(entity);
    // });
    // }

    /***** Setting up material helpers *****/
    {
        vec3 position = vec3(0, 2, 5);
        float jump = 0.25;
        float hueJump = 0.2;

        int c = 0;
        std::vector<vec3> colors(5);

        u32strtocolorHTML(U"#63462D", colors[0]);
        u32strtocolorHTML(U"#ABABAB", colors[1]);
        u32strtocolorHTML(U"#FFD700", colors[2]);
        u32strtocolorHTML(U"#008AD8", colors[3]);
        u32strtocolorHTML(U"#FF003F", colors[4]);

        for (float h = 0.f; h < 1.f; h += hueJump, c++)
            for (float i = 1e-6; i < 1.f; i += jump)
                for (float j = 1e-6; j < 1.f; j += jump)
                {
                    ModelRef helper = Loader<MeshModel3D>::get("materialHelper").copy();

                    helper->uniforms.add(ShaderUniform(colors[c], 20));
                    helper->uniforms.add(ShaderUniform(vec2(i, j), 21));

                    helper->state.setPosition(position + 2.f * vec3(4 * i, 4 * h - 2, 4 * j) / jump + vec3(25, 0, 0));

                    scene.add(helper);
                }

        scene.add(Loader<MeshModel3D>::get("packingPaintHelper").copy());
    }

    // ComponentModularity::SynchFuncs[0].element(GG::playerEntity, Blueprint::Zweihander());

    // /****** Last Pre Loop Routines ******/
    state = AppState::run;
    std::thread physicsThreads(&Game::physicsLoop, this);

    // Sword->meshes[0]->setMenu(menu, U"sword");

    menu.batch();
    scene2D.updateAllObjects();
    fuiBatch->batch();
    fuiBatch->state.frustumCulled = false;
    // SSAO.disable();

    glPointSize(3.f);

    scene.add(SkeletonHelperRef(new SkeletonHelper(GG::playerEntity->comp<SkeletonAnimationState>())));

    ObjectGroupRef dummy = Loader<ObjectGroup>::get("Dummy").copy();
    dummy->state.setPosition(vec3(5, 0, 0));
    scene.add(dummy);

    

    std::string k = InputManager::getInputKeyString("toggle hud");

    std::cout << "Press " << k << " to toggle HUD\n";



    // PlottingHelperRef plottest(new PlottingHelper(vec4(1), 256));
    // scene2D.add(plottest);
    // plottest->state.setPositionZ(0.5);

    /******  Main Loop ******/
    while (state != AppState::quit)
    {
        // plottest->push(globals.appTime.getDeltaMS());
        // plottest->updateData();

        // this allows us to close the window when pressing the close button
        if (glfwWindowShouldClose(globals.getWindow()))
            state = AppState::quit;

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

        if (itcnt == 50)
        {
            physicsMutex.lock();
            // ComponentModularity::mergeChildren(*readTest);
            ComponentModularity::mergeChildren(*firstChild);
            physicsMutex.unlock();
        }

        // temporary
        if (enableTime) {
            GG::timeOfDay += globals.appTime.getDelta() * 0.01;
            GG::timeOfDay = fmod(GG::timeOfDay, 1.f);
        }


        constexpr double distanceToSun = 149.6e9;
        constexpr double planetRadius = 6371e3;
        constexpr double axialTilt = 23.44;
        constexpr double eccentricity = 0.0167;
        constexpr double orbitTilt = 7.25;

        constexpr double latitude = 43.6109;
        constexpr double longitude = 3.8761;

        constexpr double orbitTime = fromDayMonth(20, 6);

        // adjust for sidereal time 
        double adjustedTime = fmod(GG::timeOfDay - orbitTime, 1.0);

        vec3 planetRotation = vec3(
            radians(axialTilt) * cos(adjustedTime * 2.0 * PI),
            adjustedTime * 2.0 * PI,
            radians(axialTilt) * sin(adjustedTime * 2.0 * PI)
        );

        planetRot = planetRotation;

        quat rotQuat = quat(planetRotation);

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

        mat3 tangentSpace = mat3(tangent, bitangent, surfaceNormal);
        vec3 sunDirLocal = tangentSpace * sunDirWorld;

        sunDir = vec3(sunDirLocal.x, sunDirLocal.z, sunDirLocal.y);


        

        sun->setDirection(-sunDir);

        WidgetUI_Context uiContext = WidgetUI_Context(&ui);
        updateEntityCursor(globals.mousePosition(), globals.mouseLeftClickDown(), globals.mouseLeftClick(), uiContext);

        if (globals.mouseRightClickDown())
        {
            auto &b = first->comp<WidgetBox>();

            // vec2 pos = (b.min + b.max)*0.5f;
            vec2 pos = (globals.mousePosition() / vec2(globals.windowSize())) * 2.f - 1.f;
            vec2 scale = (b.max - b.min);

            b.initMin = pos - scale * 0.5f;
            b.initMax = pos + scale * 0.5f;
        }

        updateWidgetsStyle();

        /* TODO : remove */
        // ComponentModularity::synchronizeChildren(first);
        ComponentModularity::synchronizeChildren(gameScreenWidget);

        mainloopStartRoutine();

        for (GLFWKeyInfo input; inputs.pull(input); userInput(input), InputManager::processEventInput(input))
            ;

        InputManager::processContinuousInputs();

        // switch (GG::playerEntity->comp<ActionState>().stance())
        // {
        //     case ActionState::Stance::LEFT :
        //         AttackDirectionHelper->text = U"**<==** V ==>";
        //         break;

        //     case ActionState::Stance::RIGHT :
        //         AttackDirectionHelper->text = U"<== V **==>**";
        //         break;

        //     case ActionState::Stance::SPECIAL :
        //         AttackDirectionHelper->text = U"<== **V** ==>";
        //         break;

        //     default: break;
        // }
        // AttackDirectionHelper->batchText();

        // float h = GG::playerEntity->comp<EntityStats>().health.cur;
        // HealthBar->text = U"<";
        // h /= GG::playerEntity->comp<EntityStats>().health.max;
        // int i = 0;
        // for(; i < (int)(h*50.f); i++) HealthBar->text += U'=';
        // for(; i < 50; i++) HealthBar->text += U" -";
        // HealthBar->text += U">";
        // HealthBar->batchText();

        float maxSlow = player1.getStats().reflexMaxSlowFactor;
        float reflex = player1.getInfos().state.reflex;
        globals.simulationTime.speed = maxSlow + (1.f - maxSlow) * (1.f - reflex * 0.01);

        float scroll = globals.mouseScrollOffset().y;
        float &preflex = GG::playerUniqueInfos->infos.state.reflex;
        if (GG::playerUniqueInfos)
            preflex = clamp(preflex + scroll * 5.f, 0.f, 100.f);
        globals.clearMouseScroll();

        menu.trackCursor();
        menu.updateText();

        effects.update();

        if (GG::playerEntity->comp<EntityStats>().alive)
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

            if (&entity == GG::playerEntity.get() && globals._currentController != &this->spectator)
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

        ManageGarbage<EntityModel>();
        ManageGarbage<PhysicsHelpers>();
        ManageGarbage<WidgetBackground>();
        ManageGarbage<WidgetSprite>();
        ManageGarbage<WidgetText>();
        // GlobalComponentToggler<InfosStatsHelpers>::update(GG::entities);
        // GlobalComponentToggler<PhysicsHelpers>::update(GG::entities);
        // std::vector<EntityRef> p = {GG::playerEntity};
        // GlobalComponentToggler<InfosStatsHelpers>::update(p);
        // GlobalComponentToggler<PhysicsHelpers>::update(p);

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

        tmpTimer.start();
        scene.updateAllObjects();
        tmpTimer.end();

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
        // sun->shadowMap.bindTexture(0, 6);
        screenBuffer2D.bindTexture(0, 7);
        globals.drawFullscreenQuad();

        /* Main loop End */
        mainloopEndRoutine();
    }

    physicsThreads.join();

    GG::entities.clear();
    PG::common.destroyPhysicsWorld(PG::world);
}


