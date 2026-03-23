#include <App.hpp>

#include <Scripting/ScriptInstance.hpp>

#include <Subapps.hpp>
#include <EntityBlueprint.hpp>
#include <Game.hpp>


Camera PlayerCameraStateSave;
ClusteredFrustumHelperRef playerCamhelper;
FrustumHelperRef ShadowCamhelper1;
FrustumHelperRef ShadowCamhelper2;
FrustumHelperRef ShadowCamhelper3;

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

EntityRef Apps::ForestApp::UImenu()
{
    return newEntity("Forest APP MENU"
        , UI_BASE_COMP
        , WidgetBox()
    );
}

float getTerrainHeight(vec2 pos)
{
    static auto &terrain = Loader<Texture2D>::get("Herault_4096");

    float *pixels = (float *)terrain.getPixelSource();

    ivec2 pixelPos = ivec2(round(2048.f + pos));

    const ivec2 res = terrain.getResolution();
    pixelPos = clamp(pixelPos, ivec2(0), res);

    return pixels[pixelPos.x*res.x + pixelPos.y]*512.f;
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

        Game::playerControl = PlayerController(globals.currentCamera);
        App::setController(&Game::playerControl);
    }

    ComponentModularity::addChild(*appRoot, Blueprint::SpawnMainGameTerrain());

    appRoot->set<state3D>(true);

    srand(0);

    // if(false)
    {
        /*
            Precisly the number of tree in skyrim : 46225
        */
        // const float numberOfTree = 128000;
        // const float numberOfTree = 65000;
        const float numberOfTree = 46430;
        // const float numberOfTree = 4096;
        // const float numberOfTree = 512;
    
        // const float worldChunkSegment = 16;
        // const float worldChunkSize = 4096/worldChunkSegment;
    
        const float chunkSize = 4096;
        const float subChunkSize = chunkSize/sqrt(numberOfTree);
    
        int cnt = 0;
    
        std::vector<std::string> spawnables = {
            "Tree Test 7", 
            // "Chair 00", 
            // "Table 00"
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
    
    // if(false)
    {

        // const float grassPatchCNT = 32000;

        const float areaSize = (4096-512);

        // const float grassPatchSize = areaSize/sqrt(grassPatchCNT);
        const float grassPatchSize = 22;

        int cnt = 0;

        for(int i = -areaSize*0.5; i <= areaSize*0.5; i+=grassPatchSize)
        for(int j = -areaSize*0.5; j <= areaSize*0.5; j+=grassPatchSize)
        {
            vec2 pos = vec2(i, j);
            float h0 = getTerrainHeight(pos);

            ComponentModularity::addChild(*appRoot, spawnEntity(
                "Grass Patch 2",
                vec3(pos.y, h0, pos.x)
            ));

            cnt ++;
        }

        NOTIF_MESSAGE("Number of cells ", cnt);
    }

    
    ComponentModularity::ReparentChildren(*appRoot);
    physicsMutex.unlock();

    glLineWidth(4.0);

    globals.simulationTime.resume();
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


void Apps::ForestApp::clean()
{
    globals.simulationTime.pause();

    globals.currentCamera->setMouseFollow(false);
    globals.currentCamera->setPosition(vec3(0));
    globals.currentCamera->setDirection(vec3(-1, 0, 0));

    appRoot = EntityRef();
    GG::playerEntity = EntityRef();
    App::setController(nullptr);

    GG::sun->shadowCameraSize = vec2(0, 0);
}

