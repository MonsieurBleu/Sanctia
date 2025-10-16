#include <App.hpp>
#include <Subapps.hpp>
#include <EntityBlueprint.hpp>

/*
    REMEMBER TO DO :

    To enable your application, you must create an instance of it in src/Game__mainloop.cpp :
        [...]
        Apps::MainGameApp testsubapps1;
        Apps::EntityCreator entityCreator;
        Apps::AssetListViewer assetView;
        Apps::MaterialViewerApp materialView;
        Apps::EventGraphApp eventGraph;
        Apps::SceneMergeApp sceneMerge;
        ++> Apps::__TEMPLATE__App __TEMPLATE__;
        [...]
*/

Apps::__TEMPLATE__App::__TEMPLATE__App() : SubApps("__TEMPLATE__")
{
    inputs.push_back(&
        InputManager::addEventInput(
            "input exemple", GLFW_KEY_SPACE, 0, GLFW_PRESS, [&]() {
                
                NOTIF_MESSAGE("SPACE BAR PRESSED")

            },
            InputManager::Filters::always, false)
    );    

    for(auto &i : inputs)
        i->activated = false;
};

EntityRef Apps::__TEMPLATE__App::UImenu()
{
    return newEntity("__TEMPLATE__ APP MENU"
        , UI_BASE_COMP
        , WidgetBox()
    );
}

void Apps::__TEMPLATE__App::init()
{
    /***** Preparing App Settings *****/
    {
        appRoot = newEntity("AppRoot");
        App::setController(&orbitController);
    }

}

void Apps::__TEMPLATE__App::update()
{
    ComponentModularity::synchronizeChildren(appRoot);

    /* 
        UPDATING ORBIT CONTROLLER ACTIVATION 
    */
    vec2 screenPos = globals.mousePosition();
    screenPos = (screenPos/vec2(globals.windowSize()))*2.f - 1.f;

    auto &box = EDITOR::MENUS::GameScreen->comp<WidgetBox>();
    vec2 cursor = ((screenPos-box.min)/(box.max - box.min));

    if(cursor.x < 0 || cursor.y < 0 || cursor.x > 1 || cursor.y > 1)
        globals.currentCamera->setMouseFollow(false);
    else
        globals.currentCamera->setMouseFollow(true);
}


void Apps::__TEMPLATE__App::clean()
{
    globals.simulationTime.pause();

    globals.currentCamera->setMouseFollow(false);
    globals.currentCamera->setPosition(vec3(0));
    globals.currentCamera->setDirection(vec3(-1, 0, 0));

    appRoot = EntityRef();
    App::setController(nullptr);

    GG::sun->shadowCameraSize = vec2(0, 0);
}

