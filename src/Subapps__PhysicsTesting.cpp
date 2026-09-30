#include <App.hpp>
#include <Subapps.hpp>
#include <EntityBlueprint.hpp>


#include <JoltIntegration/PhysicsDebugRenderer.hpp>
#include <PlayerController2.hpp>
#include <EnvironementGenerator.hpp>

PlayerController2 playerControl;

Apps::PhysicsTestingApp::PhysicsTestingApp() : SubApps("Physics Testing")
{
    inputs.push_back(&
        InputManager::addEventInput(
            "input exemple", GLFW_KEY_ENTER, 0, GLFW_PRESS, [&]() {
                
                globals.simulationTime.toggle();

            },
            InputManager::Filters::always, false)
    );    

    JoltVulpine::useVelocityBasedMovement = true;

    if(JoltVulpine::useVelocityBasedMovement)
        NOTIF_MESSAGE("Using VERSION 2 deplacement")
    else 
        NOTIF_MESSAGE("Using VERSION 1 deplacement")

    inputs.push_back(&
        InputManager::addEventInput(
            "input exemple", GLFW_KEY_F, 0, GLFW_PRESS, [&]() {
                
                JoltVulpine::useVelocityBasedMovement = !JoltVulpine::useVelocityBasedMovement;

                if(JoltVulpine::useVelocityBasedMovement)
                    NOTIF_MESSAGE("Using VERSION 2 deplacement")
                else 
                    NOTIF_MESSAGE("Using VERSION 1 deplacement")

            },
            InputManager::Filters::always, false)
    );    

    inputs.push_back(&
        InputManager::addEventInput(
            "input exemple", GLFW_KEY_KP_SUBTRACT, 0, GLFW_PRESS, [&]() {
                if(!globals.simulationTime.isPaused())
                    globals.simulationTime.speed /= 2.f;
            },
            InputManager::Filters::always, false)
    );    

    inputs.push_back(&
        InputManager::addEventInput(
            "input exemple", GLFW_KEY_KP_ADD, 0, GLFW_PRESS, [&]() {
                if(!globals.simulationTime.isPaused())
                    globals.simulationTime.speed *= 2.f;
            },
            InputManager::Filters::always, false)
    );    


    for(auto &i : inputs)
        i->activated = false;
};

EntityRef Apps::PhysicsTestingApp::UImenu()
{
    return newEntity("PhysicsTesting APP MENU"
        , UI_BASE_COMP
        , WidgetBox()
    );
}

bool dither(ivec2 uvi, int lod)
{
    switch(lod)
    {
        case -1 : return true;
        case 0  : return true;
        case 1  : return uvi.x%2 == uvi.y%2;
        default : return uvi.x%(1<<lod) == 0 && uvi.y%(1<<lod) == 0;
    }
}


ObjectGroupRef models[4];
ObjectGroupRef grass;

void Apps::PhysicsTestingApp::init()
{
    const vec3 origin(-675, 37, -624);

    /***** Preparing App Settings *****/
    {
        // appRoot = newEntity("AppRoot", state3D(true));
        appRoot = newEntity("AppRoot", State3D());
        // App::setController(&orbitController);

        App::setController(&playerControl);

        GG::sun->shadowCameraSize = vec2(2048);
        // GG::sun->cameraResolution = vec2(1024);
        // ComponentModularity::addChild(
        //     *appRoot,
        //     GG::playerEntity = spawnEntity("Jolt Player", origin + vec3(0, 8, 0))
        // );

        HierarchyState3D s;
        s.position = origin + vec3(0, 8, 0);
        GG::playerEntity = spawnEntityToParent("Jolt Player", *appRoot, s);

        // ComponentModularity::synchronizeChildren(appRoot);
        
        // orbitController.position = vec3(0, 40, 0);
        // globals.currentCamera->setPosition(vec3(32, 64, 0));
    }
    
    
    for(int x = 0; x < 4; x++)
        for(int y = 0; y < 4; y++)
        {
                // ComponentModularity::addChild(*appRoot, 
                //     spawnEntity("Jolt Test Ball", origin + vec3(0, 8 + x, y) + (rand()%8)/8.f)
                // );
        
                // ComponentModularity::addChild(*appRoot, 
                //     spawnEntity("Jolt Test Suzanne", origin + vec3(4 + rand()%2, 8 + x, y) + (rand()%8)/8.f)
                // );

                // ComponentModularity::addChild(*appRoot, 
                //     spawnEntity("Jolt Player Alt", origin + vec3(4 + rand()%2, 8 + x, y) + (rand()%8)/8.f)
                // );
            }
        
    // ComponentModularity::addChild(*appRoot, spawnEntity("Jolt Test Scene", origin));
    HierarchyState3D s;
    s.position = origin;
    spawnEntityToParent("Jolt Test Scene", *appRoot, s);

    ComponentModularity::addChild(*appRoot, Blueprint::SpawnMainGameTerrain());
    

    bool spawnGrass = false;

    // if(spawnGrass)
    // {
    //     const float areaSize = (4096-512);

    //     // const float grassPatchSize = areaSize/sqrt(grassPatchCNT);
    //     // const float grassPatchSize = 22;
    //     const float grassPatchSize = 16;

    //     for(float i = -areaSize*0.5; i <= areaSize*0.5; i+=grassPatchSize)
    //     for(float j = -areaSize*0.5; j <= areaSize*0.5; j+=grassPatchSize)
    //     {
    //         vec2 pos = vec2(i, j);
    //         float h0 = getTerrainHeight(pos);

    //         ComponentModularity::addChild(*appRoot, spawnEntity(
    //             // "Grass Patch 2",
    //             "Grass Patch 4",
    //             // "Grass Patch 5",
    //             vec3(pos.y, h0, pos.x)
    //         ));
    //     }

    //     // ComponentModularity::ReparentChildren(*appRoot);
    // }
    if(spawnGrass)
    {
        std::string models[4] = {
            // "Grass Patch 4x64x64",
            // "Grass Patch 4x32x32",
            // "Grass Patch 2x16x16",
            // "Grass Patch 1x8x8"
            "Grass Patch 4x64x64",
            "Grass Patch 2x32x32",
            "Grass Patch 1x16x16",
            "Grass Patch 1x8x8"
        };

        for(int i = 0; i < 4; i++)
            Loader<ObjectGroup>::get(models[i]).
            getInstances()[0]
                .originalModel
                ->baseUniforms.add(ShaderUniform(float(i), 31));

        constexpr float patchSize = 16.f;
        constexpr int gridRes = 23;

        for(int i = -gridRes+1; i < gridRes; i++)
        for(int j = 0; j < gridRes; j++)
        {
            ivec2 uvi(i, j);
            vec2 uv(uvi);

            float d = length(uv/(float)gridRes);
            d = log2(max(2.f, 64.f*d))-2.f;
            int lod = max(floor(d), 0.f);

            // WARNING_MESSAGE(PRINTVAR(d), PRINTVAR(lod))

            if(lod >= 4) continue;

            EntityModel model = Loader<ObjectGroup>::get(models[lod]).copy();
            HierarchyState3D state;
            state.position = -vec3(uv.x, 0, uv.y)*patchSize;

            ComponentModularity::addChild(*appRoot, 
                newEntity("Grass Patch", State3D(state), state, model)
            );
        }
    }

    // {
    //     constexpr int gridDim = 128;
    
    //     int unDiscarded = 0;
    //     int total = 0;
    
    //     for(int i = -gridDim/2; i <= gridDim/2; i++)
    //     for(int j = -gridDim/2; j <= gridDim/2; j++)
    //     {
    //         ivec2 uvi(i, j);
    //         float d = length(vec2(uvi)/(float)gridDim)*16.0;
    //         d = log2(max(2.0, 4.0*d));
    //         int lod = floor(d)-1;

    //         bool d1 = dither(uvi, lod);
    //         // bool d2 = dither(uvi, lod+1);

    //         unDiscarded += d1;

    //         total++;
    //     }

    //     WARNING_MESSAGE(
    //         PRINTVAR(gridDim),
    //         PRINTVAR(total), 
    //         PRINTVAR(unDiscarded)
    //     )

    // }
    
    // JoltVulpine::jPhysicsSystem->OptimizeBroadPhase();

    JoltVulpine::enablePhysics = true;
    globals.simulationTime.resume();


    GrassGenerator::active = true;
}

void Apps::PhysicsTestingApp::update()
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

    // ERROR_MESSAGE(&JoltVulpine::jPhysicsSystem)

    // NOTIF_MESSAGE(globals.currentCamera->getPosition())
    // NOTIF_MESSAGE(GG::playerEntity->comp<state3D>().position)

    // GG::draw->drawSphere(GG::playerEntity->comp<ComplexMovements>().closestSurface, 0.20);
    // GG::draw->drawSphere(GG::playerEntity->comp<ComplexMovements>().closestWall, 0.20, 0.f, ModelState3D(), vec3(1, 1, 0));
    // GG::draw->drawSphere(GG::playerEntity->comp<ComplexMovements>().closestEdge, 0.20, 0.f, ModelState3D(), vec3(1, 0.5, 0));

    // NOTIF_MESSAGE(
    //     GG::playerEntity->comp<AnimationControllerRef>()->getCurrentAnimation()->getName()
    // )
}


void Apps::PhysicsTestingApp::clean()
{
    globals.simulationTime.pause();
    globals.simulationTime.speed = 1.f;
    JoltVulpine::enablePhysics = false;

    globals.currentCamera->setMouseFollow(false);
    globals.currentCamera->setPosition(vec3(0));
    globals.currentCamera->setDirection(vec3(-1, 0, 0));

    appRoot = EntityRef();
    App::setController(nullptr);

    GG::sun->shadowCameraSize = vec2(0, 0);
}

