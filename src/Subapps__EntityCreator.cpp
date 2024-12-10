#include <Subapps.hpp>
#include <EntityBlueprint.hpp>
#include <AssetManager.hpp>
#include <MathsUtils.hpp>

#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtx/string_cast.hpp>

#include <Inputs.hpp>

#include <App.hpp>

std::string formatedCounter(int n)
{
    if(n < 10) return " [00" + std::to_string(n) + "]"; 
    if(n < 100) return " [0" + std::to_string(n) + "]"; 
    return " [" + std::to_string(n) + "]"; 
}



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

    inputs.push_back(&
        InputManager::addEventInput(
            "test moving", GLFW_KEY_LEFT, 0, GLFW_PRESS, [&]() {
                
                std::cout << "MOVING ENTITY\n";

                if(!controlledEntity)
                    return;

                auto &s = controlledEntity->comp<EntityState3D>();
                s.position.y += 0.1;
                s.initPosition.y += 0.1;
                
                // UpdateCurrentEntityTransform();
            },
            InputManager::Filters::always, false)
    );



    for(auto &i : inputs)
        i->activated = false;
};

void Apps::EntityCreator::UpdateCurrentEntityTransform()
{
    ComponentModularity::ReparentChildren(*controlledEntity);

    // auto children = controlledEntity->comp<EntityGroupInfo>().children;

    // controlledEntity->comp<EntityGroupInfo>().children.clear();
    
    // for(auto c : children)
    // {
    //     ComponentModularity::addChild(
    //         *controlledEntity,
    //         c
    //     );
    // }
}

std::string Apps::EntityCreator::processNewLoadedChild(Entity *c)
{
    std::string str = c->comp<EntityInfos>().name;

    int nb = 0;
    while(UI_currentEntityChildren.find(str + formatedCounter(++nb)) != UI_currentEntityChildren.end());

    std::string finalName = str + formatedCounter(nb);
    UI_currentEntityChildren[finalName] = EntityRef();

    return finalName;
}

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

            UI_currentEntityComponent.clear();
            UI_currentEntityChildren.clear();

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

            UI_currentEntityChildren.clear();

            for(auto c : this->currentEntity.ref->comp<EntityGroupInfo>().children)
            {
                for(auto c2 : c->comp<EntityGroupInfo>().children)
                {
                    processNewLoadedChild(c2.get());
                }
                
            }


            physicsMutex.unlock();

        }, 
        [&](Entity *e)
        {

            return 0.;
        }
    );


    auto componentNameViewer = Blueprint::EDITOR_ENTITY::INO::StringListSelectionMenu(
        "Current Entity Component List",
        UI_currentEntityComponent, 
        [&](Entity *e, float v)
        {

        }, 
        [&](Entity *e)
        {

            return 0.;
        }
    );


    auto childrenToLoadMenu = Blueprint::EDITOR_ENTITY::INO::StringListSelectionMenu(
        "Children To Load",
        UI_loadableChildren, 
        [&](Entity *e, float v)
        {
            physicsMutex.lock();

            std::string str = e->comp<EntityInfos>().name;

            this->currentEntity.name = "new Entity";

            if(!this->currentEntity.ref.get())
            {
                this->currentEntity.ref = newEntity(this->currentEntity.name, EntityState3D(true));
            
            //     ComponentModularity::removeChild(
            //         *this->appRoot,
            //         this->currentEntity.ref
            //     );

            //     this->currentEntity.ref = EntityRef();
            //     GG::ManageEntityGarbage__WithPhysics();
            }
            
            VulpineTextBuffRef source(new VulpineTextBuff(
                Loader<EntityRef>::loadingInfos[str]->buff->getSource().c_str()
            ));

            auto c = DataLoader<EntityRef>::read(source);

            EntityState3D transform = EntityState3D(true);

            static int cnt = 0;
            // transform.useinit = true;
            transform.position.x = transform.initPosition.x = cnt++;
            
            // int nb = 0;
            // // for(auto c : this->currentEntity.ref->comp<EntityGroupInfo>().children)
            // //     for(auto c2 : c->comp<EntityGroupInfo>().children)
            // //         if(c2->comp<EntityInfos>().name == str)
            // //             nb++;
                    
            // while(UI_currentEntityChildren.find(str + formatedCounter(++nb)) != UI_currentEntityChildren.end());

            // std::string finalName = str + formatedCounter(nb);
            // UI_currentEntityChildren[finalName] = EntityRef();

            auto p = newEntity(processNewLoadedChild(e), transform);

            ComponentModularity::addChild(*p, c);

            ComponentModularity::addChild(*this->currentEntity.ref,p);

            physicsMutex.unlock();

        }, 
        [&](Entity *e)
        {

            return 0.;
        }
    );


    auto currentChildren = Blueprint::EDITOR_ENTITY::INO::StringListSelectionMenu(
        "Current Entity Component List",
        UI_currentEntityChildren, 
        [&](Entity *e, float v)
        {
            std::string str = e->comp<EntityInfos>().name;

            controlledEntity = nullptr;

            for(auto c : this->currentEntity.ref->comp<EntityGroupInfo>().children)
                if(str == c->comp<EntityInfos>().name)
                    controlledEntity = c.get();
        }, 
        [&](Entity *e)
        {

            return 0.;
        }
    );


    auto LoadEntity = Blueprint::EDITOR_ENTITY::INO::NamedEntry(
        U"Load Entity", toLoadMenu, 
        0.05, true
    );

    auto AddChildren = Blueprint::EDITOR_ENTITY::INO::NamedEntry(
        U"Add Children", childrenToLoadMenu, 
        0.05, true
    );

    auto CurrentComponent = Blueprint::EDITOR_ENTITY::INO::NamedEntry(
        U"Current Components", componentNameViewer, 
        0.05, true
    );

    LoadEntity->set<WidgetBackground>(WidgetBackground());
    LoadEntity->comp<WidgetStyle>()
        .setbackgroundColor1(EDITOR::MENUS::COLOR::HightlightColor1*vec4(1,1,1,0)+vec4(0,0,0,0.5))
        .setbackGroundStyle(UiTileType::SQUARE_ROUNDED)
        ;

    AddChildren->set<WidgetBackground>(WidgetBackground());
    AddChildren->comp<WidgetStyle>()
        .setbackgroundColor1(EDITOR::MENUS::COLOR::HightlightColor2*vec4(1,1,1,0)+vec4(0,0,0,0.5))
        .setbackGroundStyle(UiTileType::SQUARE_ROUNDED)
        ;

    CurrentComponent->set<WidgetBackground>(WidgetBackground());
    CurrentComponent->comp<WidgetStyle>()
        .setbackgroundColor1(EDITOR::MENUS::COLOR::HightlightColor3*vec4(1,1,1,0)+vec4(0,0,0,0.5))
        .setbackGroundStyle(UiTileType::SQUARE)
        ;

    auto referencedEntityMenu = newEntity("Referenced Entities Menu"
        , UI_BASE_COMP
        , WidgetBox()
        , WidgetStyle()
            .setautomaticTabbing(1)
        , EntityGroupInfo({
            LoadEntity, AddChildren, CurrentComponent
        })
    );

    auto ChildrenManipMenu = newEntity("Children Manipulation Menu"
        , UI_BASE_COMP
        , WidgetBox()
        , WidgetStyle()
            .setautomaticTabbing(1)
        , EntityGroupInfo({

            currentChildren,

            newEntity("Current Entity Control"
                , UI_BASE_COMP
                , WidgetBox()
                , WidgetStyle().setautomaticTabbing(3)
                , EntityGroupInfo({

                    Blueprint::EDITOR_ENTITY::INO::NamedEntry(U"Position", 
                        newEntity("Position Selector"
                            , UI_BASE_COMP
                            , WidgetBox()
                            , WidgetStyle().setautomaticTabbing(3)
                            , EntityGroupInfo({
                                Blueprint::EDITOR_ENTITY::INO::TextInput("X Position", 
                                [&](std::u32string &t)
                                {
                                    if(!controlledEntity) return;

                                    auto &s = controlledEntity->comp<EntityState3D>();
                                    s.position.x = u32strtof2(t, s.position.x);
                                    s.initPosition = s.position;
                                },
                                [&]()
                                {
                                    if(!controlledEntity) return std::u32string();

                                    float x = controlledEntity->comp<EntityState3D>().position.x;
                                    return ftou32str(x);
                                }
                                )
                            })
                        )
                        , 
                        0.25, true
                    )

                })
            )
        })
    );

    return newEntity("ENTITY EDITOR APP CONTROLS"
        , UI_BASE_COMP
        , WidgetBox()
        , WidgetStyle()
            .setautomaticTabbing(2)
        , EntityGroupInfo({
            referencedEntityMenu, ChildrenManipMenu
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
            }),

            Blueprint::EDITOR_ENTITY::INO::Toggable("Save Entity", "", 
            [&](Entity *e, float v)
            {
                VulpineTextOutputRef out(new VulpineTextOutput(1 << 16));
                DataLoader<EntityRef>::write(this->currentEntity.ref, out);
                out->saveAs("data/EditorTest.vulpineEntity");
            },
            [&](Entity *e)
            {
                return 0.f;
            })
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
    {
        UI_loadableEntity[i.first] = EntityRef();

        UI_loadableChildren[i.first] = EntityRef();
    }

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

    // globals.simulationTime.resume();
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

        // UI_currentEntityComponent.clear();

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

    if(controlledEntity)
    {
        bool sprintActivated = false;
        int upFactor = 0;
        int frontFactor = 0;
        int rightFactor = 0;

        GLFWwindow *window = globals.getWindow();

        if(glfwGetKey(window, GLFW_KEY_UP) == GLFW_PRESS)
            frontFactor ++;

        if(glfwGetKey(window, GLFW_KEY_DOWN) == GLFW_PRESS)
            frontFactor --;

        if(glfwGetKey(window, GLFW_KEY_RIGHT) == GLFW_PRESS)
            rightFactor ++;

        if(glfwGetKey(window, GLFW_KEY_LEFT) == GLFW_PRESS)
            rightFactor --;

        if(glfwGetKey(window, GLFW_KEY_SPACE) == GLFW_PRESS)
            upFactor ++;

        if(glfwGetKey(window, GLFW_KEY_LEFT_CONTROL) == GLFW_PRESS)
            upFactor --;

        if(glfwGetKey(window, GLFW_KEY_LEFT_SHIFT) == GLFW_PRESS)
            sprintActivated = true;

        const float speed = 0.1;
        const float sprintFactor = 5;

        auto &s = controlledEntity->comp<EntityState3D>();
        const vec3 cpos = s.position;
        const float delta = globals.appTime.getDelta();
        const float dspeed = speed * delta * (sprintActivated ? sprintFactor : 1.f);

        s.position += dspeed*vec3(frontFactor, upFactor, rightFactor);
        s.initPosition = s.position;

        orbitController.position = 
        controlledEntity
            ->comp<EntityGroupInfo>().children[0]
            ->comp<EntityState3D>().position;
    }

    if(currentEntity.ref)
    {
        physicsMutex.lock();
        ComponentModularity::ReparentChildren(*currentEntity.ref);
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
    GG::ManageEntityGarbage__WithPhysics();
    physicsMutex.unlock();

    App::setController(nullptr);

    UI_loadableEntity.clear();
    UI_loadableChildren.clear();
    UI_currentEntityComponent.clear();
    UI_currentEntityChildren.clear();

    GG::sun->shadowCameraSize = vec2(0, 0);

    PG::world->setIsGravityEnabled(true);
}

