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
    inputs.push_back(&
        InputManager::addEventInput(
            "Add Material Helper", GLFW_KEY_KP_ADD, 0, GLFW_PRESS, [&]() {
                static int cnt = 1;
                this->UI_loadableEntity["Hey !!!!!!" + std::to_string(cnt++)] = EntityRef();
            },
            InputManager::Filters::always, false)
    );


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
        UI_loadableEntity, 
        [&](Entity *e, float v)
        {
            physicsMutex.lock();

            std::string str = e->comp<EntityInfos>().name;

            this->currentEntity.name = str;

            if(this->currentEntity.ref.get())
            {
                ComponentModularity::removeChild(
                    *this->appRoot,
                    this->currentEntity.ref
                );

                this->currentEntity.ref = EntityRef();
                GG::ManageEntityGarbage__WithPhysics();
            }
            
            VulpineTextBuffRef source(new VulpineTextBuff(
                Loader<EntityRef>::loadingInfos[str]->buff->getSource().c_str()
            ));

            ComponentModularity::addChild(
                *this->appRoot,
                this->currentEntity.ref = DataLoader<EntityRef>::read(source)
            );

            physicsMutex.unlock();

        }, 
        [&](Entity *e)
        {

            return 0.;
        }
    );


    auto currentComponents = Blueprint::EDITOR_ENTITY::INO::StringListSelectionMenu(
        "Current Entity Component List",
        UI_currentEntityComponent, 
        [&](Entity *e, float v)
        {
            std::string str = e->comp<EntityInfos>().name;

            
        }, 
        [&](Entity *e)
        {

            return 0.;
        }
    );





    return newEntity("ENTITY EDITOR APP CONTROLS"
        , UI_BASE_COMP
        , WidgetBox()
        , WidgetStyle()
            .setautomaticTabbing(2)
        , EntityGroupInfo({
            currentComponents, toLoadMenu
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
            Blueprint::EDITOR_ENTITY::INO::NamedEntry(U"Entity Name", entityNameEntry), 
            Blueprint::EDITOR_ENTITY::INO::Toggable("Entity Auto Refresh", "", 
            [&](Entity *e, float v)
            {
                this->autoRefreshFromDisk = v == 0.f;
            },
            [&](Entity *e)
            {
                return this->autoRefreshFromDisk ? 0.f : 1.f;
            }
            )
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

        PG::world->setIsGravityEnabled(false);
    }

    // for(int cnt = 0; cnt < 128; cnt++)
    for(auto &i : Loader<EntityRef>::loadingInfos)
        UI_loadableEntity[i.first] = EntityRef();

    /* TODO : change to Entiyref later*/
    // for(auto &i : Loader<Texture2D>::loadingInfos)
    //     UI_loadableEntity[i.first] = EntityRef();

    /***** Creatign Terrain *****/
    // ComponentModularity::addChild(*appRoot,
    //     Blueprint::Terrain("ressources/maps/testPlayground.hdr",
    //                     // "ressources/maps/RuggedTerrain.hdr",
    //                     // "ressources/maps/generated_512x512.hdr",
    //                     // "ressources/maps/RT512.hdr",
    //                     // vec3(512, 64, 512),
    //                     vec3(256, 64, 256), vec3(0), 128)
    // );

    globals.simulationTime.resume();
}

void Apps::EntityCreator::update()
{
    ComponentModularity::synchronizeChildren(appRoot);

    vec2 screenPos = globals.mousePosition();
    screenPos = (screenPos/vec2(globals.windowSize()))*2.f - 1.f;

    auto &box = EDITOR::MENUS::GameScreen->comp<WidgetBox>();
    vec2 cursor = ((screenPos-box.min)/(box.max - box.min));

    if(cursor.x < 0 || cursor.y < 0 || cursor.x > 1 || cursor.y > 1)
        globals.currentCamera->setMouseFollow(false);
    else
        globals.currentCamera->setMouseFollow(true);
    

    for(auto &i : *ComponentGlobals::ComponentNamesMap)
        if(
            currentEntity.ref.get() 
            && currentEntity.ref->state[i.second] 
            && !UI_currentEntityComponent[i.first].get()
        )
            UI_currentEntityComponent[i.first] = EntityRef();

    static int cnt = 0;
    cnt ++;
    if(autoRefreshFromDisk && (cnt%144) == 0)
    {
        physicsMutex.lock();

        std::string str = this->currentEntity.name;

        if(this->currentEntity.ref.get())
        {
            ComponentModularity::removeChild(
                *this->appRoot,
                this->currentEntity.ref
            );

            this->currentEntity.ref = EntityRef();
            GG::ManageEntityGarbage__WithPhysics();
        }

        if(Loader<EntityRef>::loadingInfos[str].get())
        {
            VulpineTextBuffRef source(new VulpineTextBuff(
                Loader<EntityRef>::loadingInfos[str]->buff->getSource().c_str()
            ));

            ComponentModularity::addChild(
                *this->appRoot,
                this->currentEntity.ref = DataLoader<EntityRef>::read(source)
            );

        }
        physicsMutex.unlock();
    }
}


void Apps::EntityCreator::clean()
{
    globals.simulationTime.pause();

    globals.currentCamera->setMouseFollow(false);
    globals.currentCamera->setPosition(vec3(0));
    globals.currentCamera->setDirection(vec3(-1, 0, 0));
    globals.currentCamera->getState().FOV = radians(90.f);
    appRoot = EntityRef();
    currentEntity.ref = EntityRef();

    physicsMutex.lock();
    GG::ManageEntityGarbage__WithPhysics;
    physicsMutex.unlock();

    App::setController(nullptr);

    UI_loadableEntity.clear();

    GG::sun->shadowCameraSize = vec2(0, 0);

    PG::world->setIsGravityEnabled(true);
}

