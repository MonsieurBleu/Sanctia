#include <App.hpp>
#include <Subapps.hpp>
#include <EntityBlueprint.hpp>
#include <Game.hpp>
#include <EnvironementGenerator.hpp>

ObjectGroupRef hairDemo;
int hairDemoNumber = 0;

EntityRef hairDemoEntity;
EntityRef beardDemoEntity;

Apps::CharaApp::CharaApp() : SubApps("Character Creator")
{
    for(auto &i : inputs)
        i->activated = false;
};

EntityRef Apps::CharaApp::UImenu()
{
    return newEntity("Character APP MENU"
        , UI_BASE_COMP
        , WidgetStyle().setautomaticTabbing(2).setuseInternalSpacing(true)
        , EntityGroupInfo({
            // VulpineBlueprintUI::ColoredConstEntry("Strand Count", [](){return ftou32str(hairDemoNumber);})

            hairDemoEntity->comp<HairInfos>().createMenu(),
            beardDemoEntity->comp<HairInfos>().createMenu()
        })
    );
}


void Apps::CharaApp::init()
{
    /***** Preparing App Settings *****/
    {
        appRoot = newEntity("AppRoot");
        // App::setController(&Game::spectator);        
        App::setController(&orbitController);

        GG::sun->shadowCameraSize = vec2(2048);
    }

    GG::skyboxType = 2;
    GG::useCustomSunPos = true;
    GG::customSunTheta = 0.25f;
    GG::customSunPhi = 0.25f;
    globals.simulationTime.resume();

    globals.currentCamera->setPosition(vec3(0, 0, 4));

    orbitController.position = vec3(0);
    orbitController.distance = 4.0;

    // ComponentModularity::addChild(*appRoot, Blueprint::SpawnMainGameTerrain());

    // ComponentModularity::addChild(*appRoot, hairDemoEntity = newEntity("Hair Demo",
    //     HairInfos()
    // ));

    {
        auto m = Loader<ObjectGroup>::get("Hair Test").copy();
        EntityModel m2 = newObjectGroup();
        m2->add(m);
        hairDemoEntity = newEntity("Hair Demo", HairInfos(), m2);
        ComponentModularity::addChild(*appRoot, hairDemoEntity);
    }
    {
        auto m = Loader<ObjectGroup>::get("Beard Test").copy();
        EntityModel m2 = newObjectGroup();
        m2->add(m);
        beardDemoEntity = newEntity("Beard Demo", HairInfos(), m2);
        ComponentModularity::addChild(*appRoot, beardDemoEntity);
    }

    hairDemo = Loader<ObjectGroup>::get("Mannequin Wig").copy();
    hairDemo->propagateHideStatus();
    globals.getScene()->add(hairDemo);

    PaintShaderPass::enableFog = false;
}

void Apps::CharaApp::update()
{
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

    // for(int i = 0; i < hairDemo->getChildren()[0]->getChildren().size(); i++)
    // {
    //     hairDemo->getChildren()[0]->getChildren()[i]->
    //         setInstancesUserData(
    //             hairDemoEntity->comp<HairInfos>().toUserInfo(
    //                 i, 
    //                 hairDemoNumber
    //             )
    //         )
    //     ;
    // }

    glLineWidth(1.0);

    // glPatchParameteri(GL_PATCH_VERTICES, 6);
}

void Apps::CharaApp::clean()
{
    globals.simulationTime.pause();

    globals.currentCamera->setMouseFollow(false);
    globals.currentCamera->setPosition(vec3(0));
    globals.currentCamera->setDirection(vec3(-1, 0, 0));

    appRoot = EntityRef();
    hairDemoEntity = EntityRef();
    beardDemoEntity = EntityRef();
    App::setController(nullptr);

    globals.getScene()->remove(hairDemo);
    hairDemo = ObjectGroupRef();

    GG::sun->shadowCameraSize = vec2(0, 0);

    GG::skyboxType = 0;
    GG::useCustomSunPos = false;

    PaintShaderPass::enableFog = true;
}

