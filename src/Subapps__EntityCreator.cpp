#include <Subapps.hpp>
#include <EntityBlueprint.hpp>
#include <AssetManager.hpp>
#include <MathsUtils.hpp>

#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtx/string_cast.hpp>

#include <Inputs.hpp>

#include <App.hpp>

Apps::EntityCreator::EntityCreator() : SubApps("Entity Editor")
{


    for(auto &i : inputs)
        i->activated = false;
};

EntityRef Apps::EntityCreator::UImenu()
{
    // auto menu = newEntity("ENTITY EDITOR APP CONTROLS"
    //     , UI_BASE_COMP
    //     , WidgetBox()
    //     , WidgetStyle()
    //         // .setbackgroundColor1(EDITOR::MENUS::COLOR::HightlightColor1)
    //         .setautomaticTabbing(30)
    //     // , WidgetBackground()
    // );


    // for(auto &i : Loader<EntityRef>::loadingInfos)
    // {
    //     ComponentModularity::addChild(
    //         *menu,
    //         newEntity(i.second->buff->getSource()
    //             , UI_BASE_COMP
    //             , WidgetBox()
    //             , WidgetStyle()
    //                 .setbackgroundColor1(EDITOR::MENUS::COLOR::LightBackgroundColor1)
    //                 .settextColor1(EDITOR::MENUS::COLOR::DarkBackgroundColor1)
    //             , WidgetBackground()
    //             , WidgetText(UFTconvert.from_bytes(i.first))
    //             , WidgetButton()
    //         )
    //     );
    // }

    // return menu;

    auto toLoadMenu = Blueprint::EDITOR_ENTITY::INO::StringListSelectionMenu(
        "Entities To Load",
        toLoadList, 
        [](float v){}, 
        [](){return 0;}
    );

    return newEntity("ENTITY EDITOR APP CONTROLS"
        , UI_BASE_COMP
        , WidgetBox()
        , EntityGroupInfo({
            toLoadMenu
        })
    );
}


EntityRef Apps::EntityCreator::UIcontrols()
{
    auto entityNameEntry = Blueprint::EDITOR_ENTITY::INO::TextInput(
        "Entity Name",
        [&](std::u32string &t){this->currentEntity.name = UFTconvert.to_bytes(t);},
        [&](){return UFTconvert.from_bytes(this->currentEntity.name);}
    );


    return newEntity("ENTITY EDITOR APP MENU"
        , UI_BASE_COMP
        , WidgetBox()
        , WidgetStyle()
            // .settextColor1(EDITOR::MENUS::COLOR::HightlightColor1)
            // .setbackgroundColor1(EDITOR::MENUS::COLOR::DarkBackgroundColor1)
            .setautomaticTabbing(1)
        // , WidgetBackground()
        // , WidgetText()
        , EntityGroupInfo({
            Blueprint::EDITOR_ENTITY::INO::NamedEntry(U"Entity Name", entityNameEntry)

        })
    );
}


void Apps::EntityCreator::init()
{
    /***** Preparing App Settings *****/
    {
        appRoot = newEntity("AppRoot");
        App::setController(&orbitController);

        globals.currentCamera->setPosition(normalize(vec3(-0.5, 0.5, 0)));
        globals.currentCamera->getState().FOV = radians(90.f);
        // orbitController.distance = 150;
        // orbitController.distance = 75;
        orbitController.distance = 5;
        // orbitController.distance = 200;

        GG::sun->cameraResolution = vec2(8192);
        GG::sun->shadowCameraSize = vec2(256, 256);
    }

    // for(auto &i : Loader<EntityRef>::loadingInfos)
    //     toLoadList[i.first] = EntityRef();
    /* TODO : change to Entiyref later*/
    for(auto &i : Loader<Texture2D>::loadingInfos)
        toLoadList[i.first] = EntityRef();

    // globals.simulationTime.resume();
}

void Apps::EntityCreator::update()
{
    ComponentModularity::synchronizeChildren(appRoot);
}


void Apps::EntityCreator::clean()
{
    globals.simulationTime.pause();

    globals.currentCamera->setMouseFollow(false);
    globals.currentCamera->setPosition(vec3(0));
    globals.currentCamera->setDirection(vec3(-1, 0, 0));
    globals.currentCamera->getState().FOV = radians(90.f);
    appRoot = EntityRef();
    App::setController(nullptr);

    toLoadList.clear();

    GG::sun->shadowCameraSize = vec2(0, 0);
}

