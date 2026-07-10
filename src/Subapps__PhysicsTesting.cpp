#include <App.hpp>
#include <Subapps.hpp>
#include <EntityBlueprint.hpp>

#include <JoltIntegration/PhysicsDebugRenderer.hpp>

#include <PlayerController2.hpp>

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

    JoltVulpine::useVelocityBasedDeplacement = true;

    if(JoltVulpine::useVelocityBasedDeplacement)
        NOTIF_MESSAGE("Using VERSION 2 deplacement")
    else 
        NOTIF_MESSAGE("Using VERSION 1 deplacement")

    inputs.push_back(&
        InputManager::addEventInput(
            "input exemple", GLFW_KEY_F, 0, GLFW_PRESS, [&]() {
                
                JoltVulpine::useVelocityBasedDeplacement = !JoltVulpine::useVelocityBasedDeplacement;

                if(JoltVulpine::useVelocityBasedDeplacement)
                    NOTIF_MESSAGE("Using VERSION 2 deplacement")
                else 
                    NOTIF_MESSAGE("Using VERSION 1 deplacement")

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

void Apps::PhysicsTestingApp::init()
{
    /***** Preparing App Settings *****/
    {
        appRoot = newEntity("AppRoot", state3D(true));
        // App::setController(&orbitController);

        App::setController(&playerControl);

        GG::sun->shadowCameraSize = vec2(2048);
        ComponentModularity::addChild(
            *appRoot,
            GG::playerEntity = spawnEntity("Jolt Player", vec3(0, 50, 0))
        );
        
        // orbitController.position = vec3(0, 40, 0);
        // globals.currentCamera->setPosition(vec3(32, 64, 0));
    }
    
    
    for(int x = 0; x < 4; x++)
        for(int y = 0; y < 4; y++)
        {
                ComponentModularity::addChild(*appRoot, 
                    spawnEntity("Jolt Test Ball", vec3(0, 38 + 8 + x, y) + (rand()%8)/8.f)
                );
        
                ComponentModularity::addChild(*appRoot, 
                    spawnEntity("Jolt Test Suzanne", vec3(4 + rand()%2, 38 + 8 + x, y) + (rand()%8)/8.f)
                );

                // ComponentModularity::addChild(*appRoot, 
                //     spawnEntity("Jolt Player Alt", vec3(4 + rand()%2, 38 + 8 + x, y) + (rand()%8)/8.f)
                // );
            }
        
    ComponentModularity::addChild(*appRoot, spawnEntity("Jolt Test Scene", vec3(0, 38, 0)));
    
    ComponentModularity::addChild(*appRoot, Blueprint::SpawnMainGameTerrain());
    
    // JoltVulpine::jPhysicsSystem->OptimizeBroadPhase();

    JoltVulpine::enablePhysics = true;
    globals.simulationTime.resume();
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
    // GG::draw->drawSphere(GG::playerEntity->comp<ComplexMovements>().closestWall, 0.20, 0.f, ModelState3D(), vec3(0, 0, 1));

    // NOTIF_MESSAGE(
    //     GG::playerEntity->comp<AnimationControllerRef>()->getCurrentAnimation()->getName()
    // )
}


void Apps::PhysicsTestingApp::clean()
{
    globals.simulationTime.pause();
    JoltVulpine::enablePhysics = false;

    globals.currentCamera->setMouseFollow(false);
    globals.currentCamera->setPosition(vec3(0));
    globals.currentCamera->setDirection(vec3(-1, 0, 0));

    appRoot = EntityRef();
    App::setController(nullptr);

    GG::sun->shadowCameraSize = vec2(0, 0);
}

