#include <App.hpp>

#include <Scripting/ScriptInstance.hpp>

#include <Subapps.hpp>
#include <EntityBlueprint.hpp>
#include <Game.hpp>
#include <random>

#include <MathsUtils.hpp>
#include <EnvironementGenerator.hpp>

Camera PlayerCameraStateSave;
ClusteredFrustumHelperRef playerCamhelper;
FrustumHelperRef ShadowCamhelper1;
FrustumHelperRef ShadowCamhelper2;
FrustumHelperRef ShadowCamhelper3;

EntityScatterer *currentScatterer = nullptr;
EntityScatterer::SpawnInfo tmpSpawnInfo;

Apps::ForestApp::ForestApp() : SubApps("Forest")
{
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
                        if(GG::playerEntity->has<PhysicsInfos>()) GG::playerEntity->comp<PhysicsInfos>().shoudBeActive = body->isActive();
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

    inputs.push_back(&
        InputManager::addEventInput(
            "input exemple", GLFW_KEY_F, 0, GLFW_PRESS, [&]() {
                
                // NOTIF_MESSAGE("SPACE BAR PRESSED")

                // globals.getScene()->tree.root.cull(2);

                GG::draw->drawSphere(
                    globals.currentCamera->getPosition(),
                    2.0,
                    120.f,
                    ModelState3D(),
                    vec3(1, 0, 0)
                );


                globals.getScene()->tree.root.recusriveCall([&](StaticSceneOctree::Node &n){
                    // GG::draw->drawBox(
                    //     n.middle-n.size*0.5f,
                    //     n.middle+n.size*0.5f,
                    //     0.f,
                    //     ModelState3D(),
                    //     hsv2rgb(vec3((float)(n.nodeDepth)/QUADTREE_ITERATION, 1.0, 1.0))
                    // );

                    // GG::draw->drawBoxFromHalfExtents(
                    //     n.middle,
                    //     n.size*0.5f,
                    //     0.f,
                    //     ModelState3D(),
                    //     hsv2rgb(vec3((float)(n.nodeDepth)/QUADTREE_ITERATION, 1.0, 1.0))
                    // );

                    // if(n.nodeDepth <= 1) return;
                
                    vec3 color = vec3(0, 1, 0);

                    if(n.status[1] == StaticSceneOctree::Node::CullingStatus::NotVisible)
                        color = vec3(1, 0, 0);

                    if(n.status[1] == StaticSceneOctree::Node::CullingStatus::Undefined)
                        color = vec3(1);

                    if(n.status[1] == StaticSceneOctree::Node::CullingStatus::PartiallyVisible)
                    {
                        // partiallyVisibleElements += n.elements.size();
                        color = vec3(0, 1, 1);
                    }

                    if(n.status[1] == StaticSceneOctree::Node::CullingStatus::FullyVisible)
                    {
                        color = vec3(0, 1, 0);
                        // fullyVisibleElements += n.elements.size();
                    }

                    if(color.r == 1.0) return;
                    if(n.elements.empty()) return;

                    // if(n.query)
                    // {
                    //     // n.query->getQueryResults();

                    //     n.query->retreiveQueryResults();

                    //     if(!n.query->getQueryResult())
                    //         color = vec3(1, 0.5, 0);

                    //     // ERROR_MESSAGE(n.query->getQueryResult());
                    // }

                    GG::draw->drawBoxFromHalfExtents(
                        n.middle,
                        n.size*0.5f,
                        120.f,
                        ModelState3D(),
                        color
                    );

                });


            },
            InputManager::Filters::always, false)
    );    

    inputs.push_back(&
        InputManager::addEventInput(
        "save camera state", GLFW_KEY_E, 0, GLFW_PRESS, [&]() { 
            PlayerCameraStateSave = *globals.currentCamera;
        })
    );

    // inputs.push_back(&
    //     InputManager::addEventInput(
    //     "frustum helper", GLFW_KEY_E, 0, GLFW_PRESS, [&]() { 

    //         Loader<ScriptInstance>::get("Shadowmap Fit").run(PlayerCameraStateSave);

    //         if(helper && helper2)
    //         {
    //             globals.getScene()->remove(helper);
    //             globals.getScene()->remove(helper2);
    //         }

    //         Camera cam1 = threadState["RETURN_Camera_1"];
    //         cam1.setType(CameraType::ORTHOGRAPHIC);

    //         helper = std::make_shared<ClusteredFrustumHelper>(PlayerCameraStateSave, ivec3(5), vec3(0, 1, 0));
    //         helper2 = std::make_shared<FrustumHelper>(cam1, ivec3(10), vec3(1, 0, 0));

    //         globals.getScene()->add(helper);
    //         globals.getScene()->add(helper2);
    //     })
    // );

    for(auto &i : inputs)
        i->activated = false;
};

vec3 getHeatmapColor(const float alpha)
{
    const vec3 lowest  = vec3(0.125, 0.0, 0.5);
    const vec3 low     = vec3(0.25, 0.75, 0.5);
    const vec3 normal  = vec3(1.0, 1.0, 0.5);
    const vec3 high    = vec3(1.0, 0.5, 0.0);
    const vec3 highest = vec3(1.0, 0.0, 0.0);

    vec3 color(lowest);

    color = mix(color, low,     smoothstep(0.00f, 0.25f, alpha));
    color = mix(color, normal,  smoothstep(0.25f, 0.50f, alpha));
    color = mix(color, high,    smoothstep(0.50f, 0.75f, alpha));
    color = mix(color, highest, smoothstep(0.75f, 1.00f, alpha));

    return color;
}

EntityRef BiomeInfosValueModifier(float &center, float &range, std::string name)
{
    // auto sliders = newEntity("Biome Infos Value Modifier " + name + " - Sliders"
    //     , UI_BASE_COMP
    //     , WidgetStyle()
    //         .setautomaticTabbing(1)
    //     , EntityGroupInfo({
    //         VulpineBlueprintUI::NamedEntry(U"Center", 
    //             VulpineBlueprintUI::ValueInputSlider(name
    //                 , 0, 1, 100, 
    //                 [&center](float f)
    //                 {
    //                     center = f;
    //                 },
    //                 [&center]()
    //                 {
    //                     return center;
    //                 }), 
    //                 0.25
    //             ),
    //         VulpineBlueprintUI::NamedEntry(U"Range", 
    //             VulpineBlueprintUI::ValueInputSlider(name
    //                 , 0, 1, 100, 
    //                 [&range](float f)
    //                 {
    //                     range = f;
    //                 },
    //                 [&range]()
    //                 {
    //                     return range;
    //                 }),
    //                 0.25
    //             )
    //     })
    // );

    auto visualazer = newEntity("Biome Infos Value Modifier " + name + " - Visualazer"
        , UI_BASE_COMP
        , WidgetStyle()
            .setautomaticTabbing(1)
        , EntityGroupInfo()
        , WidgetButton(
            WidgetButton::Type::SLIDER,
            [&center](Entity *e, float f){
                center = f;
            },
            [&center, &range](Entity *e)
            {
                auto &box = e->comp<WidgetBox>();
                if(box.isUnderCursor)
                {
                    vec2 off = globals.mouseScrollOffset();
                    range = clamp(range+sign(off.y)*0.1f, 0.f, 1.f);
                    globals.clearMouseScroll();
                }
                return center;
            }
        )
    );

    const int it = 100;
    for(int i = 0; i <= it; i++)
    {
        float a = (float)i/(float)it;
        ComponentModularity::addChild(*visualazer, 
            newEntity("Biome Infos Value Modifier " + name + " - Visualazer part"
                , UI_BASE_COMP
                , WidgetBox([a, &center, &range](Entity* parent, Entity* child){
                    float alpha = range != 0.f ? 1.f-clamp(abs(a-center)/range, 0.f, 1.f) : 0.f;

                    vec3 color = getHeatmapColor(alpha);

                    child->comp<WidgetStyle>().setbackgroundColor1(vec4(color, 1.f));
                })
                , WidgetBackground()
                , WidgetStyle()
            )
        );
    }

    return VulpineBlueprintUI::NamedEntry(UFTconvert.from_bytes(name),
        newEntity("Biome Infos Value Modifier" + name
            , UI_BASE_COMP
            , WidgetStyle()
                .setautomaticTabbing(1)
            , EntityGroupInfo({
                // sliders,
                visualazer
                })
            ),
        0.33,
        false
    );
};


EntityRef BiomeInfosModifier(BiomeInfos &center, BiomeInfos &range)
{
    #define BIOME_VALUE_MODIFER(v) BiomeInfosValueModifier(center.v, range.v, #v)

    return newEntity("Biome Infos Modifier"
        , UI_BASE_COMP
        , WidgetStyle()
            .setautomaticTabbing(sizeof(BiomeInfos)/sizeof(float))
            // .setuseInternalSpacing(true)
        , EntityGroupInfo({
            BIOME_VALUE_MODIFER(Elevation),
            BIOME_VALUE_MODIFER(Slope),
            BIOME_VALUE_MODIFER(Humidity),
            BIOME_VALUE_MODIFER(Grassyness),
            BIOME_VALUE_MODIFER(ForestDensity),
            BIOME_VALUE_MODIFER(Mystic),
            BIOME_VALUE_MODIFER(Corruption)
        })
    );
};

EntityRef Apps::ForestApp::BiomeDataModifier()
{
    auto selectedEntitySpawn = VulpineBlueprintUI::StringListSelectionMenu(
        "Spawn List", 
        localEntityList, 
        [&](Entity *e, float f)
        {
            std::vector<std::string> tmp;

            bool firstErased = false;

            std::string tmps = e->comp<EntityInfos>().name;

            while(tmps.size() and tmps.back() != '[')
                tmps.pop_back();
            
            tmps.pop_back();
            tmps.pop_back();

            for(auto &i : tmpSpawnInfo.entities)
            {
                if(i != tmps or firstErased)
                    tmp.push_back(i);
                else
                    firstErased = true;
            }

            tmpSpawnInfo.entities = tmp;
        }, 
        [](Entity *e){return 0.f;}, -1.0, VulpineColorUI::HightlightColor1, 0.075
    )
    ;

    auto entityListMenu = VulpineBlueprintUI::StringListSelectionMenu(
        "Add Entity To Spawn List", 
        entityList,
        [&](Entity *e, float f)
        {
            tmpSpawnInfo.entities.push_back(e->comp<EntityInfos>().name);
        }, 
        [](Entity *e){return 0.f;}, -1.0, VulpineColorUI::HightlightColor5, 0.075
    )
    ;


    return newEntity("Biome Infos Modifier"
        , UI_BASE_COMP
        , WidgetStyle().setautomaticTabbing(1)
        , EntityGroupInfo({
            selectedEntitySpawn,
            entityListMenu
        })
    );
};

EntityRef Apps::ForestApp::UImenu()
{
    // auto &testBiome = Loader<EntityScatterer>::get("Biomes").spawns_old[0];

    EntityRef biomeInfoEditor = newEntity("Biome Infos Editor"
        , UI_BASE_COMP
        , WidgetStyle().setautomaticTabbing(2)
        , EntityGroupInfo({
            BiomeInfosModifier(tmpSpawnInfo.spawnWeightsCenter, tmpSpawnInfo.spawnWeightsRange),
            newEntity("Biome Infos Modifer - Values"
                , UI_BASE_COMP
                , WidgetStyle().setautomaticTabbing(6)
                , EntityGroupInfo({
                    VulpineBlueprintUI::NamedEntry(U"Rotation Range X", 
                        VulpineBlueprintUI::ValueInputSlider(
                            "Rotation Range X", 0.f, 180.f, 180/5, 
                            [](float f){tmpSpawnInfo.radialRange.x = f;}, 
                            [](){return tmpSpawnInfo.radialRange.x;}
                        ), 0.33
                    ),
                    VulpineBlueprintUI::NamedEntry(U"Rotation Range Y", 
                        VulpineBlueprintUI::ValueInputSlider(
                            "Rotation Range Y", 0.f, 180.f, 180/5, 
                            [](float f){tmpSpawnInfo.radialRange.y = f;}, 
                            [](){return tmpSpawnInfo.radialRange.y;}
                        ), 0.33
                    ),
                    VulpineBlueprintUI::NamedEntry(U"Rotation Range Z", 
                        VulpineBlueprintUI::ValueInputSlider(
                            "Rotation Range Z", 0.f, 180.f, 180/5, 
                            [](float f){tmpSpawnInfo.radialRange.z = f;}, 
                            [](){return tmpSpawnInfo.radialRange.z;}
                        ), 0.33
                    ),
                    VulpineBlueprintUI::NamedEntry(U"Scale Min", 
                        VulpineBlueprintUI::ValueInputSlider(
                            "Scale Min", 0.f, 10.f, 100, 
                            [](float f){tmpSpawnInfo.scaleRangeMin = f;}, 
                            [](){return tmpSpawnInfo.scaleRangeMin;}
                        ), 0.33
                    ),
                    VulpineBlueprintUI::NamedEntry(U"Scale Max", 
                        VulpineBlueprintUI::ValueInputSlider(
                            "Scale Max", 0.f, 10.f, 100,
                            [](float f){tmpSpawnInfo.scaleRangeMax = f;}, 
                            [](){return tmpSpawnInfo.scaleRangeMax;}
                        ), 0.33
                    ),
                    VulpineBlueprintUI::NamedEntry(U"Density", 
                        VulpineBlueprintUI::ValueInputSlider(
                            "Density", 0.f, 20.f, 200, 
                            [](float f){tmpSpawnInfo.densityPerCell = f;}, 
                            [](){return tmpSpawnInfo.densityPerCell;}
                        ), 0.33
                    )
                })
            
            )
        })
    );

    EntityRef biomeLoadMenu = newEntity("Biome Load Menu"
        , UI_BASE_COMP
        , WidgetStyle().setautomaticTabbing(1)
        , EntityGroupInfo({

            VulpineBlueprintUI::StringListSelectionMenu("Entity Scatterers", entityScattererList, 
                [&](Entity *e, float f)
                {
                    currentScatterer = &Loader<EntityScatterer>::get(e->comp<EntityInfos>().name);
                    spawnInfosListInsideCurrentScatterer.clear();
                },
                [](Entity *e)
                {
                    return &Loader<EntityScatterer>::get(e->comp<EntityInfos>().name) == currentScatterer ? 0.f : 1.f;
                },
                -1.0,
                VulpineColorUI::HightlightColor2,
                0.075
            ),

            VulpineBlueprintUI::StringListSelectionMenu("Current Biomes", spawnInfosListInsideCurrentScatterer, 
                [&](Entity *e, float f)
                {
                    if(!currentBiome.empty())
                        Loader<EntityScatterer::SpawnInfo>::get(currentBiome) = tmpSpawnInfo;

                    currentBiome = e->comp<EntityInfos>().name;
                    tmpSpawnInfo = Loader<EntityScatterer::SpawnInfo>::get(currentBiome);
                },
                [&](Entity *e)
                {
                    return currentBiome == e->comp<EntityInfos>().name ? 0.f : 1.f;
                },
                -1.0,
                VulpineColorUI::HightlightColor3,
                0.075
            ),

            VulpineBlueprintUI::StringListSelectionMenu("Biomes To Add", spawnInfosList, 
                [](Entity *e, float f){},
                [](Entity *e){return 0.f;},
                -1.0,
                VulpineColorUI::HightlightColor4,
                0.075
            ),

        })
    );

    return newEntity("Forest APP MENU"
        , UI_BASE_COMP
        , WidgetStyle().setautomaticTabbing(3)
        , WidgetBox()
        , EntityGroupInfo({
            biomeLoadMenu,
            biomeInfoEditor,
            BiomeDataModifier()
        })
    );
}

EntityRef Apps::ForestApp::UIcontrols()
{
    auto toggleBiomeHelper = VulpineBlueprintUI::Toggable("Biome Helper", "", 
        [&](Entity *e, float f)
        {
            if(biomeHelper)
                destroyBiomeHelper();
            else
                createBiomeHelper();
        },
        [&](Entity *e)
        {
            return biomeHelper ? 0.f : 1.f;
        }
    );

    auto toggleBiomeGen = VulpineBlueprintUI::Toggable("Generate", "", 
        [&](Entity *e, float f)
        {
            if(biome)
            {
                biome = EntityRef();
                GG::ManageEntityGarbage__WithPhysics();
            }

            if(currentScatterer)
            {
                currentScatterer->generateInit(vec2(-2000.f), vec2(2000.f), biome = newEntity("Biome", state3D(true), EntityGroupInfo()));
                // biome->comp<EntityGroupInfo>().children.reserve(1<<17);
            }
        },
        [&](Entity *e)
        {
            return currentScatterer and currentScatterer->generateGetProgress() < 1.f ? 0.f : 1.f;
        }
    );

    auto biomeGenProgress = VulpineBlueprintUI::ColoredConstEntry("Gen Progress", 
        []()
        {
            return currentScatterer ? ftou32str(currentScatterer->generateGetProgress()*100.f) + U"%" : U"-";
        }
    );

    return newEntity("Forest APP MENU"
        , UI_BASE_COMP
        , WidgetStyle().setautomaticTabbing(1)
        , WidgetBox()
        , EntityGroupInfo({
            toggleBiomeHelper,
            toggleBiomeGen,
            biomeGenProgress,
            newEntity()
        })
    );
}


void Apps::ForestApp::init()
{
    physicsMutex.lock();
    
    /***** Preparing App Settings *****/
    {
        appRoot = newEntity("AppRoot", state3D(true));

        // App::setController(&orbitController);
        // orbitController.position = vec3(0, 32, 0);
        // App::setController(&spectator);

        // globals.currentCamera->setPosition(vec3(0, 64, 32));

        GG::sun->shadowCameraSize = vec2(2048);
        // GG::sun->activateShadows();




        GG::playerEntity = spawnEntity("(Combats) Player");
        GG::playerEntity->comp<state3D>().useinit = true;
        GG::playerEntity->comp<state3D>().initPosition = vec3(16, 34, 0);
        ComponentModularity::addChild(*appRoot, GG::playerEntity);

        // Game::playerControl = PlayerController(globals.currentCamera);
        // App::setController(&Game::playerControl);


        GG::playerEntity->comp<EntityModel>()->state.hide = ModelStatus::HIDE;

        for (auto &i : GG::playerEntity->comp<Items>().equipped)
            if (i.item.get() && i.item->has<EntityModel>())
                i.item->comp<EntityModel>()->state.hide = ModelStatus::HIDE;
        App::setController(&Game::spectator);
    }

    ComponentModularity::addChild(*appRoot, Blueprint::SpawnMainGameTerrain());

    appRoot->set<state3D>(true);

    srand(0);

    if(false)
    {
        /*
            Precisly the number of tree in skyrim : 46225
        */
        // const float numberOfTree = 128000;
        const float numberOfTree = 65000;
        // const float numberOfTree = 46430;
        // const float numberOfTree = 4096;
        // const float numberOfTree = 512;
    
        // const float worldChunkSegment = 16;
        // const float worldChunkSize = 4096/worldChunkSegment;
    
        const float chunkSize = 4096;
        const float subChunkSize = chunkSize/sqrt(numberOfTree);
    
        int cnt = 0;
    
        std::vector<std::string> spawnables = {
            "Tree Test 7", 
            "Chair 00", 
            "Table 00"
        };
    
        for(float i = -chunkSize/2.f; i < chunkSize/2.f; i+=subChunkSize)
        for(float j = -chunkSize/2.f; j < chunkSize/2.f; j+=subChunkSize)
        {
            auto r = rand();
            vec2 pos = vec2(i, j) + subChunkSize*(0.5f - vec2(r%64, (r>>6)%64)/64.f);
    
            float h0 = getTerrainHeight(pos);
            float h1 = getTerrainHeight(pos + vec2(+subChunkSize, +subChunkSize)*0.5f);
            float h2 = getTerrainHeight(pos + vec2(+subChunkSize, -subChunkSize)*0.5f);
            float h3 = getTerrainHeight(pos + vec2(-subChunkSize, +subChunkSize)*0.5f);
            float h4 = getTerrainHeight(pos + vec2(-subChunkSize, -subChunkSize)*0.5f);
            
            const float maxSlope = 0.5f*subChunkSize;
            const float slope = max(abs(h0-h1), max(abs(h0-h2), max(abs(h0-h3), abs(h0 - h3) )));
            if(slope >= maxSlope) continue;
    
            cnt ++;
    
    
    
            
    
            for(auto &e : spawnables)
            {
                r = rand();
                pos = vec2(i, j) + subChunkSize*(0.5f - vec2(r%64, (r>>6)%64)/64.f);
    
                quat q = quat(vec3(
                    radians(180 + float(rand()%10) - 5.f),
                    radians(float(rand()%360)),
                    radians(180 + float(rand()%10) - 5.f)
                ));
    
                EntityRef tmp;
    
                ComponentModularity::addChild(*appRoot, tmp = spawnEntity(
                    e,
                    vec3(pos.y, getTerrainHeight(pos), pos.x),
                    q
                ));
    
                tmp->comp<EntityModel>()->state.setScale(vec3(
                    0.75 + 0.5f*float(rand()%64)/64.f,
                    0.75 + 0.5f*float(rand()%64)/64.f,
                    0.75 + 0.5f*float(rand()%64)/64.f
                )
                );
            }
        }
        
        NOTIF_MESSAGE("Number of cells ", cnt);
    }
    
    if(false)
    {
        NAMED_TIMER(Biome_Generation);
        Biome_Generation.start();

        const float chunkSize = 16;

        EntityScatterer Biomes(
            vec2(0), vec2(chunkSize), 
            {
                EntityScatterer::SpawnInfo /* Basic Forest */
                {
                    .entities = {
                        "Tree Oak 01",
                        "Tree Oak 01",
                        "Tree Olive 01",
                        "Tree Olive 02",
                        "Tree Pine 02",
                    },
                    .spawnWeightsCenter = {.Grassyness = 0.0, .ForestDensity = 1.0, .Slope = 0.1, .Elevation = 0.0},
                    .spawnWeightsRange  = {.Grassyness = 0.0, .ForestDensity = 1.0, .Slope = 0.5, .Elevation = 0.2},
                    .radialRange = vec3(10, 180, 10),
                    .scaleRangeMin = 1.25,
                    .scaleRangeMax = 0.75
                },

                EntityScatterer::SpawnInfo /* Mountain Forest */
                {
                    .entities = {
                        "Tree Cypres 01",
                        "Tree Cypres 01",
                        "Tree Cypres 01",
                        "Tree Pine 02",
                        "Tree Pine 02"
                    },
                    .spawnWeightsCenter = {.Grassyness = 0.0, .ForestDensity = 1.0, .Slope = 0.1, .Elevation = 0.45},
                    .spawnWeightsRange  = {.Grassyness = 0.0, .ForestDensity = 1.0, .Slope = 0.5, .Elevation = 0.25},
                    .radialRange = vec3(10, 180, 10),
                    .scaleRangeMin = 1.25,
                    .scaleRangeMax = 0.75
                }
            }
        );

        // Biomes.writeToFile("data/[0] Export/Biomes/Biomes");
        // Biomes.spawns[0].spawnWeightsCenter.writeToFile("data/[0] Export/Biomes/Basic Forest");
        // VulpineTextBuffRef file(new VulpineTextBuff("data/[0] Export/Biomes/Biomes.sEntityScatterer"));
        // WARNING_MESSAGE(file->read())
        // EntityScatterer testRead = DataLoader<EntityScatterer>::read(file);
        // testRead.writeToFile("data/[0] Export/Biomes/Biomes2");

        const float border = 256.f;

        int globalCount = 0;

        Loader<Texture2D>::get("Forest Density").generate();
        int rowCnt = 0;
        for(float x = border-2048.f; x < 2048.f-border; x+=chunkSize, rowCnt ++)
        for(float y = border-2048.f; y < 2048.f-border; y+=chunkSize)
        {
            vec2 pos = vec2(x, y + (rowCnt%2 ? 0 : chunkSize*0.5));
            BiomeInfos localBiome = 
            {
                    .Grassyness = getBiomeMap(pos, "Grassyness"),
                    .ForestDensity = getBiomeMap(pos, "Forest Density")
            };

            // WARNING_MESSAGE(localBiome.ForestDensity);
            
            int count = Biomes.generate_old(
                pos,
                // {.ForestDensity = 1.f - smoothstep(256.f, 1024.f, length(vec2(x, y)))},
                // {.ForestDensity = 1.f - abs(x)/(2048.f-border)}, 
                // {.ForestDensity = 1.0},
                // {
                //     .Grassyness = getBiomeMap(vec2(x, y), "Grassyness"),
                //     .ForestDensity = getBiomeMap(vec2(x, y), "Forest Density")
                // },
                localBiome,
                0, 
                3,
                appRoot
            );

            globalCount += count;
        }

        Biome_Generation.stop();
        NOTIF_MESSAGE(globalCount)
        NOTIF_MESSAGE(Biome_Generation.getElapsedTime());
    }

    
    if(false)
    {

        // const float grassPatchCNT = 32000;

        const float areaSize = (4096-512);

        // const float grassPatchSize = areaSize/sqrt(grassPatchCNT);
        const float grassPatchSize = 22;

        int cnt = 0;

        for(float i = -areaSize*0.5; i <= areaSize*0.5; i+=grassPatchSize)
        for(float j = -areaSize*0.5; j <= areaSize*0.5; j+=grassPatchSize)
        {
            vec2 pos = vec2(i, j);
            float h0 = getTerrainHeight(pos);

            ComponentModularity::addChild(*appRoot, spawnEntity(
                // "Grass Patch 2",
                "Grass Patch 4",
                // "Grass Patch 5",
                vec3(pos.y, h0, pos.x)
            ));

            cnt ++;
        }

        NOTIF_MESSAGE("Number of cells ", cnt);
    }

    
    // currentScatterer = &Loader<EntityScatterer>::get("Biomes");

    // for(float x = -2048; x <= 2048; x += 16)
    // for(float y = -2048; y <= 2048; y += 16)
    // {
    //     float h = getTerrainHeight(vec2(x, y));
    //     EntityRef cube = spawnEntity("Cube Helper", vec3(y, h+32.0, x));

    //     cube->comp<EntityModel>()->state.setScale(vec3(16.f, 32.f, 16.f));
    //     cube->comp<EntityModel>()->update();

    //     ComponentModularity::addChild(*appRoot, cube);
    // }


    ComponentModularity::ReparentChildren(*appRoot);
    physicsMutex.unlock();

    glLineWidth(4.0);

    // globals.simulationTime.resume();
}

void Apps::ForestApp::update()
{
    // ComponentModularity::synchronizeChildren(appRoot);

    /* 
        UPDATING ORBIT CONTROLLER ACTIVATION 
    */
    // vec2 screenPos = globals.mousePosition();
    // screenPos = (screenPos/vec2(globals.windowSize()))*2.f - 1.f;

    // auto &box = EDITOR::MENUS::GameScreen->comp<WidgetBox>();
    // vec2 cursor = ((screenPos-box.min)/(box.max - box.min));

    // if(cursor.x < 0 || cursor.y < 0 || cursor.x > 1 || cursor.y > 1)
    //     globals.currentCamera->setMouseFollow(false);
    // else
    //     globals.currentCamera->setMouseFollow(true);

    if(currentScatterer and biomeHelper)
        System<EntityModel>([&](Entity &e, EntityModel &m)
            {
                if(e.comp<EntityInfos>().name != "Cube Helper") return;

                vec3 pos = m->state.position;
                auto local = currentScatterer->getLocalInfos(vec2(pos.z, pos.x));
                float alpha = currentScatterer->evaluateDensity(tmpSpawnInfo, local);

                vec3 color = getHeatmapColor(alpha);

                m->state.setScale(vec3(currentScatterer->rangeMax.x, 32.f, currentScatterer->rangeMax.y) + color/10.f);
                m->update();
            }, 
            32, globals.appTime.getUpdateCounter()%32
        );

    if(currentScatterer)
    {
        currentScatterer->generateFrame(20.f);

        // if(currentScatterer->generateGetProgress() < 1.f)
        //      globals.simulationTime.pause();
        // else
        //      globals.simulationTime.resume();

        for(auto &i : currentScatterer->spawnsNames)
            spawnInfosListInsideCurrentScatterer[i];

        std::unordered_map<std::string, int> tmp;

        for(auto &i : tmpSpawnInfo.entities)
        {
            auto elem = tmp.find(i);
            if(elem == tmp.end()) tmp[i] = 0;

            tmp[i]++;

            // if(!localEntityList[i])
            //     localEntityList[i] = EntityRef();
        }

        std::unordered_map<std::string, EntityRef> tmp2;

        for(auto &i : tmp)
        {
            std::string id = i.first + " [" + std::to_string(i.second) + "]";
            tmp2[id] = localEntityList[id];
        }

        localEntityList = tmp2;

        if(currentBiome.size())
            Loader<EntityScatterer::SpawnInfo>::get(currentBiome) = tmpSpawnInfo;
    }
    else if(!localEntityList.empty())
    {
        localEntityList.clear();
    }

    for(auto &i : Loader<EntityScatterer::SpawnInfo>::loadingInfos)
    {
        spawnInfosList[i.first];
    }

    for(auto &i : Loader<EntityScatterer>::loadingInfos)
    {
        entityScattererList[i.first];
    }

    for(auto &i : Loader<EntityRef>::loadingInfos)
    {
        entityList[i.first];
    }
    
    // for(auto &i : Loader<EntityScatterer>::loadingInfos)
    // {
    //     entityScattererList[i.first];
    // }

    // if(false)
    {
        Loader<ScriptInstance>::get("Shadowmap Fit").run(PlayerCameraStateSave, vec3(globals.getScene()->getLights().front()->getInfos()._direction));
    
        if(playerCamhelper && ShadowCamhelper1 && ShadowCamhelper3 && ShadowCamhelper3)
        {
            globals.getScene()->remove(playerCamhelper);
            globals.getScene()->remove(ShadowCamhelper1);
            globals.getScene()->remove(ShadowCamhelper2);
            globals.getScene()->remove(ShadowCamhelper3);
        }
    
        Camera cam1 = threadState["RETURN_Camera_1"];
        // cam1.setType(CameraType::ORTHOGRAPHIC_FORCED_CORNER);
    
        Camera cam2 = threadState["RETURN_Camera_2"];
        // cam2.setType(CameraType::ORTHOGRAPHIC_FORCED_CORNER);
    
        Camera cam3 = threadState["RETURN_Camera_3"];
        // cam3.setType(CameraType::ORTHOGRAPHIC_FORCED_CORNER);
    
        playerCamhelper = std::make_shared<ClusteredFrustumHelper>(PlayerCameraStateSave, ivec3(4), vec3(0, 0, 1));
        ShadowCamhelper1 = std::make_shared<FrustumHelper>(cam1, ivec3(15), vec3(1, 0, 0));
        ShadowCamhelper2 = std::make_shared<FrustumHelper>(cam2, ivec3(15), vec3(1, 1, 0));
        ShadowCamhelper3 = std::make_shared<FrustumHelper>(cam3, ivec3(15), vec3(0, 1, 0));
    
        globals.getScene()->add(playerCamhelper);
        globals.getScene()->add(ShadowCamhelper1);
        globals.getScene()->add(ShadowCamhelper2);
        globals.getScene()->add(ShadowCamhelper3);

        // globals.getScene()->getLights().front()->shadowCamera = cam1;
        // globals.getScene()->getLights().front()->shadowCamera.updateProjectionViewMatrix();
    }
}

void Apps::ForestApp::createBiomeHelper()
{
    biomeHelper = newEntity("Biome Helper");

    for(float x = -2048; x <= 2048; x += 16)
    for(float y = -2048; y <= 2048; y += 16)
    {
        float h = getTerrainHeight(vec2(x, y));
        EntityRef cube = spawnEntity("Cube Helper", vec3(y, h+32.0, x));

        cube->comp<EntityModel>()->state.setScale(vec3(16.f, 32.f, 16.f));
        cube->comp<EntityModel>()->update();

        ComponentModularity::addChild(*biomeHelper, cube);
    }
}

void Apps::ForestApp::destroyBiomeHelper()
{
    biomeHelper = EntityRef();
    GG::ManageEntityGarbage();
}

void Apps::ForestApp::clean()
{
    globals.simulationTime.pause();

    globals.currentCamera->setMouseFollow(false);
    globals.currentCamera->setPosition(vec3(0));
    globals.currentCamera->setDirection(vec3(-1, 0, 0));

    biomeHelper = EntityRef();
    biome = EntityRef();

    appRoot = EntityRef();
    GG::playerEntity = EntityRef();
    App::setController(nullptr);


    GG::sun->shadowCameraSize = vec2(0, 0);
}

