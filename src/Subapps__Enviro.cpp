#include <App.hpp>
#include <Subapps.hpp>
#include <EntityBlueprint.hpp>
#include <Game.hpp>
#include <EnvironementGenerator.hpp>

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
        ++> Apps::EnviroApp Enviro;
        [...]
*/

Apps::EnviroApp::EnviroApp() : SubApps("Enviro")
{
    for(auto &i : inputs)
        i->activated = false;
};

EntityRef Apps::EnviroApp::UImenu()
{
    auto biomeGenProgress = VulpineBlueprintUI::ColoredConstEntry("Gen Progress", 
        []()
        {
            return ftou32str(Loader<EntityScatterer>::get("Biomes").generateGetProgress()*100.f) + U"%";
        }
    );

    return newEntity("Enviro APP MENU"
        , UI_BASE_COMP
        , WidgetStyle().setautomaticTabbing(16)
        , EntityGroupInfo({biomeGenProgress})
    );
}

void Apps::EnviroApp::init()
{
    /***** Preparing App Settings *****/
    {
        appRoot = newEntity("AppRoot", state3D(true));
        App::setController(&Game::spectator);        

        GG::sun->shadowCameraSize = vec2(2048);
    }


    Loader<EntityScatterer>::get("Biomes").generateInit(vec2(-2048+256), vec2(2048-256), appRoot);

    for(auto &i : Loader<EntityScatterer>::get("Biomes").spawnsNames)
    {
        Loader<EntityScatterer::SpawnInfo>::get(i).densityPerCell *= 5.0;
    }

    appRoot->set<state3D>(true);
    ComponentModularity::addChild(*appRoot, Blueprint::SpawnMainGameTerrain());

    // if(false)
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

    ComponentModularity::ReparentChildren(*appRoot);
}

void Apps::EnviroApp::update()
{
    // ComponentModularity::synchronizeChildren(appRoot);

    Loader<EntityScatterer>::get("Biomes").generateFrame(20.f);
}


void Apps::EnviroApp::clean()
{
    globals.simulationTime.pause();

    globals.currentCamera->setMouseFollow(false);
    globals.currentCamera->setPosition(vec3(0));
    globals.currentCamera->setDirection(vec3(-1, 0, 0));

    appRoot = EntityRef();
    App::setController(nullptr);

    GG::sun->shadowCameraSize = vec2(0, 0);
}

