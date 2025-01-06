#include <Subapps.hpp>
#include <EntityBlueprint.hpp>
#include <AssetManager.hpp>
#include <MathsUtils.hpp>
#include <Constants.hpp>
#include <Helpers.hpp>

#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtx/string_cast.hpp>

#include <Inputs.hpp>

#include <App.hpp>

vec3 GimzmoProject(int axis, vec3 gizmoPosition, vec3 rayOrigin, vec3 rayDirection)
{
    rayOrigin -= gizmoPosition;

    int c1 = (axis + (axis == 0 ? 2 : 1))%3;
    int c2 = (axis + (axis == 0 ? 1 : 2))%3;

    vec3 projection = rayAlignedPlaneIntersect(
        rayOrigin, rayDirection, c1, 0.f
    );

    vec3 normal(0);
    normal[c2] = 1.0;
    projection = projectPointOntoPlane(projection, vec3(0), normal);

    return projection + gizmoPosition;
}


std::string formatedCounter(int n)
{
    if(n < 10) return " [00" + std::to_string(n) + "]"; 
    if(n < 100) return " [0" + std::to_string(n) + "]"; 
    return " [" + std::to_string(n) + "]"; 
}

Apps::EntityCreator::EntityCreator() : SubApps("Entity Editor")
{

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

            controlledEntity = nullptr;

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
            return currentEntity.name == e->comp<EntityInfos>().name ? 0.f : 1.f;

            // return 0.;
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
        "Current Entity Children List",
        UI_currentEntityChildren, 
        [&](Entity *e, float v)
        {
            std::string str = e->comp<EntityInfos>().name;

            controlledEntity = nullptr;

            for(auto c : this->currentEntity.ref->comp<EntityGroupInfo>().children)
                if(str == c->comp<EntityInfos>().name)
                {
                    controlledEntity = c.get();

                    controlledEntityEuleur = eulerAngles(controlledEntity->comp<EntityState3D>().initQuat);
                }
        }, 
        [&](Entity *e)
        {
            if(controlledEntity)
                return controlledEntity->comp<EntityInfos>().name == e->comp<EntityInfos>().name ? 0.f : 1.f;
            else
                return 1.f;
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
        .setbackGroundStyle(UiTileType::SQUARE)
        ;

    AddChildren->set<WidgetBackground>(WidgetBackground());
    AddChildren->comp<WidgetStyle>()
        .setbackgroundColor1(EDITOR::MENUS::COLOR::HightlightColor2*vec4(1,1,1,0)+vec4(0,0,0,0.5))
        .setbackGroundStyle(UiTileType::SQUARE)
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
                , WidgetStyle().setautomaticTabbing(4)
                , EntityGroupInfo({


                    Blueprint::EDITOR_ENTITY::INO::NamedEntry(U"Position", 
                        newEntity("Position controls"
                            , UI_BASE_COMP
                            , WidgetBox()
                            , WidgetStyle().setautomaticTabbing(3)
                            , EntityGroupInfo({

                                Blueprint::EDITOR_ENTITY::INO::NamedEntry(U"X",
                                    Blueprint::EDITOR_ENTITY::INO::ValueInput("X Position"
                                        , [&](float f)
                                        {
                                            if(!controlledEntity) return;
                                            auto &s = controlledEntity->comp<EntityState3D>();
                                            s.position.x = s.initPosition.x = f;
                                        }
                                        , [&]()
                                        {
                                            if(!controlledEntity) return 0.f;
                                            return controlledEntity->comp<EntityState3D>().position.x;
                                        }, 
                                        -1e6f, 1e6f, 0.1f, 1.f
                                    ), 0.25, false, EDITOR::MENUS::COLOR::HightlightColor1
                                ),
                                Blueprint::EDITOR_ENTITY::INO::NamedEntry(U"Y",
                                    Blueprint::EDITOR_ENTITY::INO::ValueInput("Y Position"
                                        , [&](float f)
                                        {
                                            if(!controlledEntity) return;
                                            auto &s = controlledEntity->comp<EntityState3D>();
                                            s.position.y = s.initPosition.y = f;
                                        }
                                        , [&]()
                                        {
                                            if(!controlledEntity) return 0.f;
                                            return controlledEntity->comp<EntityState3D>().position.y;
                                        }, 
                                        -1e6f, 1e6f, 0.1f, 1.f
                                    ), 0.25, false, EDITOR::MENUS::COLOR::HightlightColor2
                                ),
                                Blueprint::EDITOR_ENTITY::INO::NamedEntry(U"Z",
                                    Blueprint::EDITOR_ENTITY::INO::ValueInput("Z Position"
                                        , [&](float f)
                                        {
                                            if(!controlledEntity) return;
                                            auto &s = controlledEntity->comp<EntityState3D>();
                                            s.position.z = s.initPosition.z = f;
                                        }
                                        , [&]()
                                        {
                                            if(!controlledEntity) return 0.f;
                                            return controlledEntity->comp<EntityState3D>().position.z;
                                        }, 
                                        -1e6f, 1e6f, 0.1f, 1.f
                                    ), 0.25, false, EDITOR::MENUS::COLOR::HightlightColor3
                                ),
                            })
                        ),
                        0.25, true
                    ),


                    Blueprint::EDITOR_ENTITY::INO::NamedEntry(U"Rotation", 
                        newEntity("Rotation controls"
                            , UI_BASE_COMP
                            , WidgetBox()
                            , WidgetStyle().setautomaticTabbing(3)
                            , EntityGroupInfo({
                                Blueprint::EDITOR_ENTITY::INO::ValueInputSlider("X Rotation",
                                    0.f, 360.f, 360/5, 
                                    [&](float f)
                                    {
                                        if(!controlledEntity) return;
                                        controlledEntityEuleur.x = radians(f - 180.f);
                                        controlledEntity->comp<EntityState3D>().initQuat = quat(controlledEntityEuleur);
                                    }
                                    , [&]()
                                    {
                                        if(!controlledEntity) return 0.f;
                                        return degrees(controlledEntityEuleur.x) + 180.f;
                                    },
                                    EDITOR::MENUS::COLOR::HightlightColor1
                                ),
                                Blueprint::EDITOR_ENTITY::INO::ValueInputSlider("Y Rotation",
                                    0.f, 360.f, 360/5, 
                                    [&](float f)
                                    {
                                        if(!controlledEntity) return;
                                        controlledEntityEuleur.y = radians(f - 180.f);
                                        controlledEntity->comp<EntityState3D>().initQuat = quat(controlledEntityEuleur);
                                    }
                                    , [&]()
                                    {
                                        if(!controlledEntity) return 0.f;
                                        return degrees(controlledEntityEuleur.y) + 180.f;
                                    },
                                    EDITOR::MENUS::COLOR::HightlightColor2
                                ),
                                Blueprint::EDITOR_ENTITY::INO::ValueInputSlider("Z Rotation",
                                    0.f, 360.f, 360/5, 
                                    [&](float f)
                                    {
                                        if(!controlledEntity) return;
                                        controlledEntityEuleur.z = radians(f - 180.f);
                                        controlledEntity->comp<EntityState3D>().initQuat = quat(controlledEntityEuleur);
                                    }
                                    , [&]()
                                    {
                                        if(!controlledEntity) return 0.f;
                                        return degrees(controlledEntityEuleur.z) + 180.f;
                                    },
                                    EDITOR::MENUS::COLOR::HightlightColor3
                                )
                            })
                        ),
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

            Blueprint::EDITOR_ENTITY::INO::Toggable("Use Gizmo", "icon_gizmo", 
            [&](Entity *e, float v)
            {
                this->gizmoActivated = v == 0.f;
            },
            [&](Entity *e)
            {
                return this->gizmoActivated ? 0.f : 1.f;
            }),


            Blueprint::EDITOR_ENTITY::INO::Toggable("Toggle Gravity", "", 
            [&](Entity *e, float v)
            {
                PG::world->setIsGravityEnabled(v == 0.f);
                globals.enablePhysics = v == 0.f;
            },
            [&](Entity *e)
            {
                return PG::world->isGravityEnabled() && globals.enablePhysics ? 0.f : 1.f;
            }),

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
            }),
        })
    );
}

vec4 gizmoColorsBase[3] = 
{
    EDITOR::MENUS::COLOR::HightlightColor1, 
    EDITOR::MENUS::COLOR::HightlightColor2,
    EDITOR::MENUS::COLOR::HightlightColor3 
};

vec4 gizmoColors[3] = 
{
    EDITOR::MENUS::COLOR::HightlightColor1, 
    EDITOR::MENUS::COLOR::HightlightColor2,
    EDITOR::MENUS::COLOR::HightlightColor3 
};

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

        // GG::sun->cameraResolution = vec2(8192);
        // GG::sun->shadowCameraSize = vec2(256, 256);

        // GG::skybox->state.setHideStatus(ModelStatus::HIDE);

        globals.simulationTime.resume();
        // globals.simulationTime.speed = 0.0001f;
        globals.enablePhysics = false;

        GG::skyboxType = 2;

        PG::world->setIsGravityEnabled(false);

        glLineWidth(5);
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


    /****** Creating Gizmo Helper ******/
    {
        // LineHelperRef x(new LineHelper(vec3(0), vec3(1, 0, 0), EDITOR::MENUS::COLOR::HightlightColor1));
        // LineHelperRef y(new LineHelper(vec3(0), vec3(0, 1, 0), EDITOR::MENUS::COLOR::HightlightColor2));
        // LineHelperRef z(new LineHelper(vec3(0), vec3(0, 0, 1), EDITOR::MENUS::COLOR::HightlightColor3));

        EntityModel model = EntityModel{ObjectGroupRef(newObjectGroup())};
        auto aabbhelper = CubeHelperRef(new CubeHelper(vec3(-0.5), vec3(0.5), EDITOR::MENUS::COLOR::LightBackgroundColor1));
        model->add(aabbhelper);

        ModelRef gizmoArrows[3];

        for(int i = 0; i < 3; i++)
        {
            gizmoArrows[i] = newModel(Loader<MeshMaterial>::get("basicHelper"), Loader<MeshVao>::get("arrow"));
            gizmoArrows[i]->uniforms.add(ShaderUniform((vec3*)&gizmoColors[i], 20));
            model->add(gizmoArrows[i]);
            gizmoArrows[i]->depthWrite = false;
            gizmoArrows[i]->sorted = false;
            gizmoArrows[i]->defaultMode = GL_TRIANGLES;
            // gizmoArrows[i]->state.scaleScalar(1.0).rotation[i] = i == 0 ? 0 : radians(90.f);
            // if(i == 2)
            //     gizmoArrows[i]->state.rotation[0] = radians(-90.f);
            if(i == 0)
            {
                gizmoArrows[i]->state.rotation[0] = radians(90.f);
            }
            if(i == 1)
            {
                gizmoArrows[i]->state.rotation[1] = radians(180.f);
                // gizmoArrows[i]->state.rotation[0] = radians(-90.f);
                gizmoArrows[i]->state.rotation[2] = radians(90.f);
            }
            if(i == 2)
            {
                gizmoArrows[i]->state.rotation[1] = radians(-90.f);
                gizmoArrows[i]->state.rotation[0] = radians(90.f); 
                gizmoArrows[i]->state.rotation[2] = radians(90.f);
            }
        }

        ComponentModularity::addChild(*appRoot, 
            gizmo = newEntity("Gizmo Helper"
                , model
            )
        );
    }

    /****** Creating World Grid Helper ******/
    {
        EntityModel model = EntityModel{ObjectGroupRef(newObjectGroup())};

        float inf = 5000;

        vec3 color = EDITOR::MENUS::COLOR::DarkBackgroundColor1;

        // model->add(
        //     LineHelperRef(new LineHelper(
        //         vec3(+inf, 0, 0), 
        //         vec3(-inf, 0, 0), 
        //         0.5f * EDITOR::MENUS::COLOR::HightlightColor1))
        // );

        // model->add(
        //     LineHelperRef(new LineHelper(
        //         vec3(0, +inf, 0), 
        //         vec3(0, -inf, 0), 
        //         0.5f * EDITOR::MENUS::COLOR::HightlightColor2))
        // );

        // model->add(
        //     LineHelperRef(new LineHelper(
        //         vec3(0, 0, +inf), 
        //         vec3(0, 0, -inf), 
        //         0.5f * EDITOR::MENUS::COLOR::HightlightColor3))
        // );

        int size = 50;
        for(int i = -size; i <= size; i+=5)
        {
            // if(i == 0) continue;

            model->add(
                LineHelperRef(new LineHelper(
                    vec3(+size, 0, i), 
                    vec3(-size, 0, i), 
                    i == 0 ? color*2.f : color))
            );

            model->add(
                LineHelperRef(new LineHelper(
                    vec3(i, 0, +size), 
                    vec3(i, 0, -size), 
                    i == 0 ? color*2.f : color))
            );
        }


        ComponentModularity::addChild(*appRoot, 
            newEntity("World Grid Helper"
                , model
            )
        );
    }
}

void Apps::EntityCreator::update()
{
    // if(!PG::world->isGravityEnabled())
    ComponentModularity::synchronizeChildren(appRoot);

    vec2 screenPos = globals.mousePosition();
    screenPos = (screenPos/vec2(globals.windowSize()))*2.f - 1.f;

    auto &box = EDITOR::MENUS::GameScreen->comp<WidgetBox>();
    vec2 cursor = ((screenPos-box.min)/(box.max - box.min));

    bool cursorOnGameScreen = !(cursor.x < 0 || cursor.y < 0 || cursor.x > 1 || cursor.y > 1);

    globals.currentCamera->setMouseFollow(cursorOnGameScreen);
    

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


    /****** Mouse Ray Casting ******/
    vec2 cursorPos = vec2(1, -1) * ((globals.mousePosition()/vec2(globals.windowSize()))*2.f - 1.f);

    auto gsb = EDITOR::MENUS::GameScreen->comp<WidgetBox>();
    float tmpf = gsb.min.y;
    gsb.min.y = -gsb.max.y;
    gsb.max.y = -tmpf;

    vec2 gameScreenPos = (cursorPos-gsb.min)/(gsb.max - gsb.min);

    vec4 ndc = vec4(
        gameScreenPos*2.f - 1.f,
        globals.currentCamera->getProjectionMatrix()[3][2] / (1e2),
        -1.0
    );

    vec4 viewpos = inverse(globals.currentCamera->getProjectionMatrix()) * ndc;
    viewpos /= viewpos.w;
    viewpos.z *= -1;
    vec4 world = inverse(globals.currentCamera->getViewMatrix()) * viewpos;

    vec3 dir = normalize(vec3(world) - globals.currentCamera->getPosition());

    rp3d::Ray ray(
        PG::torp3d(globals.currentCamera->getPosition()),
        PG::torp3d(world)
    );



    if(globals.mouseRightClick() && cursorOnGameScreen)
    {
        /**** Computing click intersection with current entity child *****/
        float mindist = 1e6;
        EntityRef mindistChild;
        if(currentEntity.ref)
        {
            for(auto c : currentEntity.ref->comp<EntityGroupInfo>().children)
            {
                auto &lodi = c->comp<LevelOfDetailsInfos>();

                rp3d::AABB aabb(
                    PG::torp3d(lodi.aabbmin), PG::torp3d(lodi.aabbmax)
                );

                rp3d::Vector3 hitpoint;

                if(aabb.raycast(ray, hitpoint))
                {
                    float dist = distance(PG::toglm(hitpoint), globals.currentCamera->getPosition());
                    if(dist < mindist)
                    {
                        dist = mindist;
                        mindistChild = c;
                    }
                }
            }
        }
        if(mindistChild)
        {
            controlledEntity = mindistChild.get();
            controlledEntityEuleur = eulerAngles(controlledEntity->comp<EntityState3D>().initQuat);
        }
    }

    if(controlledEntity && gizmoActivated)
    {
        auto &s = controlledEntity->comp<EntityState3D>();

        if(!globals.mouseLeftClickDown())
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

            const vec3 cpos = s.position;
            const float delta = globals.appTime.getDelta();
            const float dspeed = speed * delta * (sprintActivated ? sprintFactor : 1.f);

            s.position += dspeed*vec3(frontFactor, upFactor, rightFactor);
        }


        // auto entity = controlledEntity->comp<EntityGroupInfo>().children[0];
        // vec3 pos = entity->comp<EntityState3D>().position;
        // orbitController.position = pos;
        // gizmo->comp<EntityModel>()->state.setPosition(pos);


        if(controlledEntity->hasComp<LevelOfDetailsInfos>())
        {
            auto &lodi = controlledEntity->comp<LevelOfDetailsInfos>();

            vec3 extent = lodi.aabbmax - lodi.aabbmin;
            vec3 position = lodi.aabbmin + extent*0.5f;

            static vec3 aabbCenterToEntityPos = position - s.position;

            gizmo->comp<EntityModel>()->getMeshes()[0]->state.setScale(extent);
            gizmo->comp<EntityModel>()->state.setPosition(position);

            gizmo->comp<EntityModel>()->state.setHideStatus(ModelStatus::SHOW);

            float scale = orbitController.distance / 3.5;
            gizmo->comp<EntityModel>()->getMeshes()[1]->state.scaleScalar(scale);
            gizmo->comp<EntityModel>()->getMeshes()[2]->state.scaleScalar(scale);
            gizmo->comp<EntityModel>()->getMeshes()[3]->state.scaleScalar(scale);

            gizmoColors[0] = gizmoColorsBase[0];
            gizmoColors[1] = gizmoColorsBase[1];
            gizmoColors[2] = gizmoColorsBase[2];

            static int lastClickedGizmo = -1;
            static vec3 lastCenter;
            static vec3 lastClickAxePos;

            bool hit = false;
            
            if(!(globals.mouseLeftClickDown()))
            {
                aabbCenterToEntityPos = position - s.position;

                globals.currentCamera->setPosition(
                    globals.currentCamera->getPosition() - (orbitController.position - position)
                );
                orbitController.position = position;

                for(int i = 0; i < 3; i++)
                {
                    vec3 aabbmin(-0.15);
                    vec3 aabbmax(+0.15);

                    aabbmin[i] = 0;
                    aabbmax[i] = 1.75;

                    rp3d::AABB aabb(
                            PG::torp3d(scale*aabbmin + position), 
                            PG::torp3d(scale*aabbmax + position)
                        );

                    rp3d::Vector3 hitpoint;

                    if((hit = aabb.raycast(ray, hitpoint)))
                    {
                        lastClickedGizmo = i;
                        lastCenter = position;
                        lastClickAxePos = GimzmoProject(
                            i, position,
                            globals.currentCamera->getPosition(), 
                            world
                        );

                        break;
                    }
                }

                if(!hit)
                {
                    for(int i = 0; i < 3; i++)
                    {
                        vec3 aabbmin(0.2);
                        vec3 aabbmax(0.6);

                        aabbmin[i] = -0.01;
                        aabbmax[i] = +0.01;

                        rp3d::AABB aabb(
                            PG::torp3d(scale*aabbmin + position), 
                            PG::torp3d(scale*aabbmax + position)
                        );

                        rp3d::Vector3 hitpoint;

                        if((hit = aabb.raycast(ray, hitpoint)))
                        {
                            lastClickedGizmo = i+3;
                            lastCenter = position;
                            lastClickAxePos = rayAlignedPlaneIntersect(
                                globals.currentCamera->getPosition(), world, i, position[i]
                            );

                            break;
                        }
                    }
                }


                if(!hit)
                    lastClickedGizmo = -1;
            }
            else if(lastClickedGizmo != -1 && lastClickedGizmo < 3)
            {
                vec3 p = GimzmoProject(
                    lastClickedGizmo, lastCenter,
                    globals.currentCamera->getPosition(), 
                    world
                );
                
                s.position[lastClickedGizmo] = p[lastClickedGizmo] - (lastClickAxePos[lastClickedGizmo] - lastCenter[lastClickedGizmo]);
                s.position[lastClickedGizmo] -= aabbCenterToEntityPos[lastClickedGizmo];
            }
            else if(lastClickedGizmo != -1 && lastClickedGizmo >= 3)
            {
                int axis = lastClickedGizmo-3;

                vec3 p = rayAlignedPlaneIntersect(
                    globals.currentCamera->getPosition(), world, axis, lastCenter[axis]
                );

                s.position = p 
                - (lastClickAxePos - lastCenter)
                ;
                s.position[axis] = lastCenter[axis];

                // s.position[axis] += (lodi.aabbmin-position)[axis];
                // s.position[axis] -= (extent*0.5f)[axis];
                s.position -= aabbCenterToEntityPos;
            }

            
            if(lastClickedGizmo != -1 && lastClickedGizmo < 3)
            {
                gizmo->comp<EntityModel>()->getMeshes()[lastClickedGizmo + 1]->state.scaleScalar(scale*1.2);
            }
            if(lastClickedGizmo != -1 && lastClickedGizmo >= 3)
            {
                // gizmo->comp<EntityModel>()->getMeshes()[lastClickedGizmo - 2]->state.scaleScalar(scale*0.8);
                gizmoColors[lastClickedGizmo - 3] = gizmoColorsBase[lastClickedGizmo - 3]*2.f;
            }
        
        }

        s.initPosition = s.position;
    }
    else
        gizmo->comp<EntityModel>()->state.setHideStatus(ModelStatus::HIDE);


    if(currentEntity.ref && !PG::world->isGravityEnabled())
    {
        physicsMutex.lock();
        ComponentModularity::ReparentChildren(*currentEntity.ref);
        physicsMutex.unlock();
    }
}


void Apps::EntityCreator::clean()
{
    globals.simulationTime.pause();
    globals.simulationTime.speed = 1.f;
    globals.enablePhysics = true;

    globals.currentCamera->setMouseFollow(false);
    globals.currentCamera->setPosition(vec3(0));
    globals.currentCamera->setDirection(vec3(-1, 0, 0));
    globals.currentCamera->getState().FOV = radians(90.f);

    appRoot = EntityRef();
    currentEntity.ref = EntityRef();
    gizmo = EntityRef();

    physicsMutex.lock();
    GG::ManageEntityGarbage__WithPhysics();
    physicsMutex.unlock();

    App::setController(nullptr);

    UI_loadableEntity.clear();
    UI_loadableChildren.clear();
    UI_currentEntityComponent.clear();
    UI_currentEntityChildren.clear();

    GG::sun->shadowCameraSize = vec2(0, 0);
    GG::skybox->state.setHideStatus(ModelStatus::SHOW);
    GG::skyboxType = 0;

    PG::world->setIsGravityEnabled(true);

    glLineWidth(1.0);
}

