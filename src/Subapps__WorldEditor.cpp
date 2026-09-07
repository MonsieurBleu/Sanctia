#include <App.hpp>
#include <Subapps.hpp>
#include <AssetManager.hpp>
#include <ModManager.hpp>
#include <Draw.hpp>

#include <JoltIntegration/PhysicsDebugRenderer.hpp>

#define FOLDER_CHAR '#'
#define FOLDER_STR "##"

#include <Gizmo.hpp>

Gizmo gizmo;

namespace WorldEntities {

    namespace Menu {
        EntityRef parent;
        EntityRef folderView;
        EntityRef hierarchyView;
    }

    std::vector<Entity*> selectedFolderContent;
    std::vector<Entity*> selectedFolderParents;
    Entity *selectedFile = nullptr;
    Entity *selectedFolder = nullptr;
    EntityRef world;

    std::string saveFolder = "data/[0] World/";

    void clear()
    {
        Menu::parent = Menu::folderView = Menu::hierarchyView = world = EntityRef();
        selectedFile = selectedFolder = nullptr;
        selectedFolderContent.clear();
        selectedFolderParents.clear();
    }

    void openFolder(Entity *p)
    {
        if(!p or p == selectedFolder) return;

        selectedFolder = p;
        selectedFile = nullptr;
        selectedFolderContent.clear();
        selectedFolderParents.clear();

        for(auto i : p->comp<EntityGroupInfo>().children)
        {
            selectedFolderContent.push_back(i.get());
        }

        while(p and p != SubApps::getCurrentRoot().get())
        {
            selectedFolderParents.push_back(p);
            p = p->comp<EntityGroupInfo>().parent;
        }

        std::reverse(selectedFolderParents.begin(), selectedFolderParents.end());
    }

    void addElement(EntityRef child)
    {
        selectedFolderContent.push_back(child.get());

        if(!selectedFolder->has<EntitySpawner>())
            selectedFolder->set<EntitySpawner>(EntitySpawner());

        if(!selectedFolder->has<State3D>())
            selectedFolder->set<State3D>(State3D());

        EntitySpawner::info i;
        i.name = child->comp<EntityInfos>().name;
        i.child = child;
        selectedFolder->comp<EntitySpawner>().onLoading.push_back(i);

        ComponentModularity::synchronizeChildren(WorldEntities::selectedFolder);
    }

    void addFile(const std::string name)
    {
        auto child = spawnEntityToParent(name, *WorldEntities::selectedFolder, HierarchyState3D());
        addElement(child);
    }

    void addFolder(const std::string name)
    {
        auto folder = newEntity(name, State3D(), HierarchyState3D(), EntityGroupInfo()); 
        ComponentModularity::addChild(*selectedFolder, folder);
        addElement(folder);
    }

    void createMenu()
    {
        Menu::folderView = VulpineBlueprintUI::SimpleFolderView("World Entities - Folder View",
            selectedFolderContent, 
            VulpineColorUI::HightlightColorGreen,
            VulpineColorUI::HightlightColorYellow,
            3,
            16,
            [](Entity *e, float f)
            {
                selectedFile = (Entity *)e->comp<WidgetButton>().usr;

                const auto &name = e->comp<EntityInfos>().name;
                if(f == 1.f and selectedFile->comp<EntityInfos>().name != name)
                {
                    openFolder(selectedFile);
                }
                else
                {
                    gizmo.parent->comp<State3D>() = selectedFile->comp<State3D>();

                    // setEntityTransform(*gizmo.parent, selectedFile->comp<State3D>());
                    // ComponentModularity::synchronizeChildren(WorldEntities::selectedFolder);
                    // gizmo.parent->comp<State3D>() = selectedFile->comp<State3D>();
                    // WARNING_MESSAGE(
                    //     PRINTVAR(selectedFile->comp<State3D>().position), 
                    //     PRINTVAR(selectedFile->comp<HierarchyState3D>().position),
                    //     PRINTVAR((int)selectedFile->comp<HierarchyState3D>().type)
                    // );
                }
            },
            [](Entity *e){return (uint64)selectedFile == e->comp<WidgetButton>().usr ? 0.f : 1.f;}
        );
        
        auto createFolderText = newEntity("Create Folder Text"
            , UI_BASE_COMP
            , WidgetBox()
            , WidgetBackground()
            , WidgetStyle()
                .setbackGroundStyle(UiTileType::SQUARE_ROUNDED)
                .setbackgroundColor1(VulpineColorUI::DarkBackgroundColor1)
                .setbackgroundColor2(VulpineColorUI::DarkBackgroundColor2)
                .settextColor1(VulpineColorUI::LightBackgroundColor1)
                .settextColor2(VulpineColorUI::HightlightColorGreen)
            , WidgetText(U".")
            , WidgetButton(WidgetButton::Type::TEXT_INPUT,
                    [&](Entity *e, float v)
                    {
                    },
                    [&](Entity *e)
                    {
                        return 0.f;
                    }
                )
        );

        Entity *createFolderTextPTR = createFolderText.get();

        auto createFolderButton = VulpineBlueprintUI::Toggable("Add Folder", "", 
            [createFolderTextPTR](Entity *e, float f)
            {
                auto text = UFTconvert.to_bytes(createFolderTextPTR->comp<WidgetText>().text);
                if(e->comp<WidgetStyle>().backgroundColor1 == VulpineColorUI::HightlightColorGreen)
                {
                    text = selectedFolder->comp<EntityInfos>().name + FOLDER_STR + text;
                    createFolderTextPTR->comp<WidgetText>().text.clear();
                    addFolder(text);
                }
            },
            [createFolderTextPTR](Entity *e)
            {
                auto text = UFTconvert.to_bytes(createFolderTextPTR->comp<WidgetText>().text);

                vec4 color = VulpineColorUI::HightlightColorRed;

                if(!text.empty() and text[0] != ' ')
                {
                    color = VulpineColorUI::HightlightColorGreen;

                    text = selectedFolder->comp<EntityInfos>().name + FOLDER_STR + text;

                    for(auto &i : selectedFolderContent)
                        if(text == i->comp<EntityInfos>().name)
                        {
                            color = VulpineColorUI::HightlightColorRed;
                            break;
                        }
                }

                e->comp<WidgetStyle>().setbackgroundColor1(color);
                createFolderTextPTR->comp<WidgetStyle>().settextColor2(color);

                return 0.f;
            },
            VulpineColorUI::HightlightColorRed
        );

        Menu::folderView->comp<WidgetBox>().set(vec2(-1, 1), vec2(-1, 0.9));

        createFolderButton->comp<WidgetBox>().set(vec2(-1, -0.5), vec2(0.9, 1));
        createFolderButton->comp<WidgetStyle>().useInternalSpacing = false;

        createFolderText->comp<WidgetBox>().set(vec2(-0.5, 1), vec2(0.9, 1));

        auto folderViewParent = newEntity("Folder View - Parent", UI_BASE_COMP, EntityGroupInfo({Menu::folderView, createFolderButton, createFolderText}));

        

        Menu::hierarchyView = VulpineBlueprintUI::SimpleFolderView("World Entities - Hierarchy View",
            selectedFolderParents, 
            VulpineColorUI::HightlightColorGreen,
            VulpineColorUI::HightlightColorGreen,
            1,
            16,
            [](Entity *e, float f)
            {
                openFolder((Entity *)e->comp<WidgetButton>().usr);
            },
            [](Entity *e){return (uint64)selectedFolder == e->comp<WidgetButton>().usr ? 0.f : 1.f;},
            false
        );

        
        auto tmp1 = VulpineBlueprintUI::NamedEntry(U"Hierarchy", Menu::hierarchyView, 0.05, true, VulpineColorUI::HightlightColorGreen);
        tmp1->comp<WidgetBox>().set(vec2(-1., -0.5), vec2(-1, 1));
        auto tmp2 = VulpineBlueprintUI::NamedEntry(U"Folder Content", folderViewParent, 0.05, true, VulpineColorUI::HightlightColorGreen);
        tmp2->comp<WidgetBox>().set(vec2(-0.5, 1), vec2(-1, 1));

        Menu::parent = newEntity("World Entities", 
            UI_BASE_COMP, 
            WidgetStyle(), 
            EntityGroupInfo({
                tmp1, tmp2
            })
        );
    };

    void saveEntity(EntityRef e)
    {
        if(e->has<EntityGroupInfo>())
            for(auto & i : e->comp<EntityGroupInfo>().children)
                if(strstr(i->comp<EntityInfos>().name.c_str(), FOLDER_STR))
                    saveEntity(i);
        
        if(e->has<EntitySpawner>())
            for(auto &i : e->comp<EntitySpawner>().onLoading)
            {
                if(i.child and i.child->has<HierarchyState3D>())
                    i.state = i.child->comp<HierarchyState3D>();
                else
                    WARNING_MESSAGE("Entity '", i.name, "' is either empty or don't have HierarchicalState3D component.")
            }

        auto name = e->comp<EntityInfos>().name;
        auto filename = saveFolder + name + ".vEntity";
        NOTIF_MESSAGE("Saving Entity : ", filename);

        VulpineTextOutputRef out(new VulpineTextOutput(1 << 16));

        State3D tmp1 = e->comp<State3D>();
        HierarchyState3D tmp2 = e->comp<HierarchyState3D>();

        // e->remove<State3D>();
        // e->remove<HierarchyState3D>();

        e->comp<State3D>() = State3D();
        e->comp<HierarchyState3D>() = HierarchyState3D();

        DataLoader<EntityRef>::write(e, out);
        out->saveAs(filename.c_str());

        e->comp<State3D>() = tmp1;
        e->comp<HierarchyState3D>() = tmp2;

        if(Loader<EntityRef>::loadingInfos.find(name) == Loader<EntityRef>::loadingInfos.end())
            Loader<EntityRef>::addInfos(filename.c_str());
    };
}

namespace EntitySpawn 
{
    namespace Menu {
        EntityRef parent;
        EntityRef entities;
        EntityRef category;
        EntityRef type;
    }

    namespace list {
        // std::unordered_map<std::string, EntityRef> category; // list of category (mods)
        std::vector<Entity *> category;
        std::unordered_map<std::string, EntityRef> entities; // list of entities to spawn depending on the current sorting
    }

    std::string selectedCategory = "";
    std::string selectedType = "";
    std::string selectedEntity = "";
    std::string types[] = {"Light Sources", "Static Decor", "Items"};

    void clear()
    {
        Menu::parent = Menu::category = Menu::entities = Menu::type = EntityRef();
        list::category.clear();
        list::entities.clear();
    }

    void refreshEntityList(const std::string &category, const std::string &type)
    {
        selectedCategory = selectedCategory == category and &category != &selectedCategory ? "" : category;
        selectedType = selectedType == type and &type != &selectedType ? "" : type;
        EntitySpawn::list::entities.clear();
        std::vector<std::string> list;

        if(selectedType.empty())
        {
            for(auto i : AssetLoadInfos::assetList["EntityRef"])
                list.push_back(i.first);
        }
        else
        {
            if(selectedType == "Light Sources")
            {
                // If the word 'pointLights' is present in any .vGroup file correspond to a LOD model of this entity,
                // we consider that this entity has source lights
                for(auto &e : Loader<EntityRef>::loadingInfos)
                {
                    for(int i = 0; i < 4; i++)
                    {
                        std::string nameCheck = e.first + " LOD" + (char)('0'+i);
                        auto elem = Loader<ObjectGroup>::loadingInfos.find(nameCheck);

                        if(
                            elem != Loader<ObjectGroup>::loadingInfos.end() and 
                            STR_CASE_STR(elem->second->buff->originalData, "pointLights")
                        )
                        {
                            list.push_back(e.first);
                            break;
                        }
                    }
                }
            }
            else
            if(selectedType == "Static Decor")
            {
                // If the world STATIC is written after the definition of a JoltBody,
                // we consider this entity to be a static decor
                for(auto &e : Loader<EntityRef>::loadingInfos)
                {
                    void * body = STR_CASE_STR(e.second->buff->originalData, "JoltBody");
                    void * static_ = STR_CASE_STR(e.second->buff->originalData, "STATIC") ;
                    if(body and static_ and body < static_)
                        list.push_back(e.first);
                }
            }
            else
            if(selectedType == "Items")
            {
                for(auto &e : Loader<EntityRef>::loadingInfos)
                    if(STR_CASE_STR(e.second->buff->originalData, "ItemInfos"))
                        list.push_back(e.first);
            }
        }


        if(selectedCategory.empty())
        {
            for(auto &i : list)
                list::entities[i] = EntityRef();
        }
        else
        {
            for(auto &i : list)
                for(auto j : AssetLoadInfos::assetList["EntityRef"][i])
                    if(j.version->name == EntitySpawn::selectedCategory)
                        list::entities[i] = EntityRef();
        }
    }

    void createMenu()
    {
        EntitySpawn::refreshEntityList("", "");

        /*
            Creating the per Type sort menu
        */
        EntitySpawn::Menu::type = newEntity(
            "Spawn Menu - Type", 
            UI_BASE_COMP, 
            WidgetStyle()
                .setautomaticTabbing(8)
                .setbackgroundColor1(VulpineColorUI::DarkBackgroundColor1)
                .setbackGroundStyle(UiTileType::SQUARE_ROUNDED),
            WidgetBackground()
        );

        for(auto &i : EntitySpawn::types)
        {
            ComponentModularity::addChild(*EntitySpawn::Menu::type, 
                VulpineBlueprintUI::Toggable(i, "",
                    [](Entity *e, float f){EntitySpawn::refreshEntityList(EntitySpawn::selectedCategory, e->comp<EntityInfos>().name);},
                    [](Entity *e){return e->comp<EntityInfos>().name == EntitySpawn::selectedType ? 0.f : 1.f;},
                    VulpineColorUI::HightlightColorOrange
                )
            );
        }

        /*
            Creating the per Category sort menu
        */
        // EntitySpawn::Menu::category = VulpineBlueprintUI::StringListSelectionMenu("Spawn Menu - Category",
        //     EntitySpawn::list::category, 
        //     [](Entity *e, float f){EntitySpawn::refreshEntityList(e->comp<EntityInfos>().name, EntitySpawn::selectedType);},
        //     [](Entity *e){return e->comp<EntityInfos>().name == EntitySpawn::selectedCategory ? 0.f : 1.f;},
        //     -2.,
        //     VulpineColorUI::HightlightColorRed,
        //     0.
        // );

        EntitySpawn::Menu::category = VulpineBlueprintUI::SimpleFolderView("Spawn Menu - Category",
            EntitySpawn::list::category, VulpineColorUI::HightlightColorRed, VulpineColorUI::HightlightColorRed, 2, 8,
            [](Entity *e, float f){EntitySpawn::refreshEntityList(e->comp<EntityInfos>().name, EntitySpawn::selectedType);},
            [](Entity *e){return e->comp<EntityInfos>().name == EntitySpawn::selectedCategory ? 0.f : 1.f;},
            true
        );

        std::unordered_map<std::string, EntityRef> tmp;
        for(auto i : AssetLoadInfos::assetList["EntityRef"])
        {
            for(auto k : i.second)
            {                
                auto c = newEntity(k.version->name);
                ComponentModularity::addChild(*Menu::category, c);
                tmp[k.version->name] = c;
            }
        }
        for(auto i : tmp)
            list::category.push_back(i.second.get());

        /*
            Creating the per menu that actually spawn entities
        */
        EntitySpawn::Menu::entities = VulpineBlueprintUI::StringListSelectionMenu("Spawn Menu - Entites",
            EntitySpawn::list::entities, 
            [](Entity *e, float f)
            {
                auto &n = e->comp<EntityInfos>().name;
                // selectedEntity = selectedEntity == n ? "" : n;
                selectedEntity = n;

                WorldEntities::addFile(n);

                gizmo.parent->comp<State3D>() = WorldEntities::selectedFile->comp<State3D>();
                WARNING_MESSAGE(WorldEntities::selectedFile->comp<State3D>().position);
            },
            [](Entity *e){return selectedEntity == e->comp<EntityInfos>().name ? 0.f : 1.f;},
            -0.125,
            VulpineColorUI::HightlightColorYellow,
            0.
        );

        EntitySpawn::Menu::entities->comp<EntityGroupInfo>().children[0]->set<WidgetBackground>(WidgetBackground());
        EntitySpawn::Menu::entities->comp<EntityGroupInfo>().children[0]->comp<WidgetStyle>()
                .setbackgroundColor1(VulpineColorUI::DarkBackgroundColor1)
                .setbackgroundColor2(VulpineColorUI::DarkBackgroundColor1)
                .setbackGroundStyle(UiTileType::SQUARE_ROUNDED);

        EntitySpawn::Menu::parent = newEntity("Entity Spawn Menu", 
            UI_BASE_COMP, 
            WidgetStyle().setautomaticTabbing(1),
            EntityGroupInfo({
                newEntity("Entity Spawm Menu 2",
                    UI_BASE_COMP,
                    WidgetStyle().setautomaticTabbing(2),
                    EntityGroupInfo({
                        VulpineBlueprintUI::NamedEntry(U"Category", EntitySpawn::Menu::category, 0.1, true, VulpineColorUI::HightlightColorRed),
                        VulpineBlueprintUI::NamedEntry(U"Type", EntitySpawn::Menu::type, 0.1, true, VulpineColorUI::HightlightColorOrange)
                    })
                ),
                VulpineBlueprintUI::NamedEntry(U"Entities To Spawn", EntitySpawn::Menu::entities, 0.05, true, VulpineColorUI::HightlightColorYellow)
            })
        );
    }
}

EntityRef Apps::WorldEditorApp::UImenu()
{
    EntitySpawn::createMenu();
    WorldEntities::createMenu();

    return newEntity("WorldEditor - APP MENU"
        , UI_BASE_COMP
        , WidgetStyle().setautomaticTabbing(2)
        , EntityGroupInfo(
            {
                WorldEntities::Menu::parent,
                EntitySpawn::Menu::parent
            })
    );
}

EntityRef Apps::WorldEditorApp::UIcontrols()
{

    return newEntity("World Editor - APP CONTROL",
        UI_BASE_COMP,
        WidgetStyle().setautomaticTabbing(1),
        EntityGroupInfo({

            VulpineBlueprintUI::Toggable("Translate", "Gizmo Translate", 
                [](Entity *e, float f){gizmo.translationMode();},
                [](Entity *e){return gizmo.isTranslationMode() ? 0. : 1.;}
                // VulpineColorUI::HightlightColorBlue
            ),

            VulpineBlueprintUI::Toggable("Rotate", "Gizmo Rotation", 
                [](Entity *e, float f){gizmo.rotationMode();},
                [](Entity *e){return gizmo.isRotationMode() ? 0. : 1.;}
                // VulpineColorUI::HightlightColorBlue
            ),

            VulpineBlueprintUI::Toggable("Scale", "Gizmo Scale", 
                [](Entity *e, float f){gizmo.scalingMode();},
                [](Entity *e){return gizmo.isScalingMode() ? 0. : 1.;}
                // VulpineColorUI::HightlightColorBlue
            ),

            VulpineBlueprintUI::Toggable("Follow Terrain Height", "", 
                [](Entity *e, float f){gizmo.toggleFollowTerrain();},
                [](Entity *e){return gizmo.isFollowingTerrain() ? 0. : 1.;},
                VulpineColorUI::HightlightColorPurple
            ),
    
            VulpineBlueprintUI::Toggable("Snap To Grid", "", 
                [](Entity *e, float f){gizmo.toggleSnapping();},
                [](Entity *e){return gizmo.isSnapingEnable() ? 0. : 1.;},
                VulpineColorUI::HightlightColorPurple
            ),

            // newEntity("Blank Space"),

            VulpineBlueprintUI::NamedEntry(U"Gizmo Mode",
                newEntity("",
                    UI_BASE_COMP,
                    WidgetStyle().setautomaticTabbing(2),
                    EntityGroupInfo({
    
                        VulpineBlueprintUI::Toggable("World", "", 
                            [](Entity *e, float f){gizmo.translateModeWorld();},
                            [](Entity *e){return gizmo.isTranslateModeWorld() ? 0. : 1.;},
                            VulpineColorUI::HightlightColorPink
                        ),
                        VulpineBlueprintUI::Toggable("Relative", "", 
                            [](Entity *e, float f){gizmo.translateModeRelative();},
                            [](Entity *e){return gizmo.isTranslateModeRelative() ? 0. : 1.;},
                            VulpineColorUI::HightlightColorPink
                        )
                    })
                ), 0.5, false, VulpineColorUI::HightlightColorPink
            )

            
        })
    );
}

Apps::WorldEditorApp::WorldEditorApp() : SubApps("WorldEditor")
{
    inputs.push_back(&
        InputManager::addEventInput(
            "GIZMO : Translate Mode", GLFW_KEY_T, 0, GLFW_PRESS, [&]() {
                gizmo.translationMode();
            },
            InputManager::Filters::always, false)
    );    

    inputs.push_back(&
        InputManager::addEventInput(
            "GIZMO : Rotate Mode", GLFW_KEY_R, 0, GLFW_PRESS, [&]() {
                gizmo.rotationMode();
            },
            InputManager::Filters::always, false)
    );    

    inputs.push_back(&
        InputManager::addEventInput(
            "GIZMO : Scale Mode", GLFW_KEY_E, 0, GLFW_PRESS, [&]() {
                gizmo.scalingMode();
            },
            InputManager::Filters::always, false)
    ); 

    inputs.push_back(&
        InputManager::addEventInput(
            "GIZMO : Undo", GLFW_KEY_W, GLFW_MOD_CONTROL, GLFW_PRESS, [&]() {
                gizmo.undo();
            },
            InputManager::Filters::always, false)
    ); 

    inputs.push_back(&
        InputManager::addEventInput(
            "GIZMO : Redo", GLFW_KEY_Y, GLFW_MOD_CONTROL, GLFW_PRESS, [&]() {
                gizmo.redo();
            },
            InputManager::Filters::always, false)
    ); 

    inputs.push_back(&
        InputManager::addEventInput(
            "GIZMO : Switch between world and relative translation mode", GLFW_KEY_Z, 0, GLFW_PRESS, [&]() {
                gizmo.translateModeSwitch();
            },
            InputManager::Filters::always, false)
    ); 

    inputs.push_back(&
        InputManager::addEventInput(
            "GIZMO : Toggle terrain height follow", GLFW_KEY_X, 0, GLFW_PRESS, [&]() {
                gizmo.toggleFollowTerrain();
            },
            InputManager::Filters::always, false)
    ); 

    inputs.push_back(&
        InputManager::addEventInput(
            "Save World", GLFW_KEY_S, GLFW_MOD_CONTROL, GLFW_PRESS, [&]() {
                WorldEntities::saveEntity(WorldEntities::world);
            },
            InputManager::Filters::always, false)
    ); 

    inputs.push_back(&
        InputManager::addContinuousInput(
            "Brought Snapping", GLFW_KEY_LEFT_SHIFT, 
            [&]() 
            {
                gizmo.enableBroughtSnapping();
            },
            InputManager::Filters::always,
            [&]()
            {
                gizmo.disableBroughtSnapping();
            }
        
        )
    ); 

    inputs.push_back(&
        InputManager::addEventInput(
            "Save World", GLFW_KEY_G, 0, GLFW_PRESS, [&]() {
                if(gizmo.isSnapingEnable())
                    gizmo.disableSnapping();
                else
                    gizmo.enableSnapping();
            },
            InputManager::Filters::always, false)
    ); 

    for(auto &i : inputs)
        i->activated = false;
};

void Apps::WorldEditorApp::init()
{
    /***** Preparing App Settings *****/
    {
        appRoot = newEntity("AppRoot");
        App::setController(&orbitController);
        globals.currentCamera->setPosition(vec3(10, getTerrainHeight(vec2(0)), 0));

        GG::sun->shadowCameraSize = vec2(2048);
    }

    gizmo.create();

    // {
    //     EntityRef dummy = spawnEntity("Jolt Test Scene");
    //     dummy->set<HierarchyState3D>(HierarchyState3D());
    //     ComponentModularity::addChild(*gizmo.parent, dummy);
    // }

    // ComponentModularity::addChild(*appRoot, gizmo.parent);

    ComponentModularity::addChild(*appRoot, Blueprint::SpawnMainGameTerrain());

    gizmo.resetHistoric();

    JoltVulpine::enablePhysics = true;
    globals.simulationTime.resume();

    WorldEntities::world = spawnEntityToParent("World", *appRoot, HierarchyState3D());

    // for(int i = 0; i < 128; i++)
    // {
    //     // ComponentModularity::addChild(*WorldEntities::world, newEntity(std::string(FOLDER_STR) + "Folder Number " + std::to_string(i)));
    //     ComponentModularity::addChild(*WorldEntities::world, newEntity("File Number " + std::to_string(i)));
    // }

    WorldEntities::openFolder(WorldEntities::world.get());
}

void getEntityAABB(
    vec3 &aabbmin, vec3 &aabbmax, Entity *e
)
{
    if(e->has<EntityModel>())
    {
        auto &m = e->comp<EntityModel>();
        auto aabb = m->getMeshesBoundingBox();
        aabbmin = min(aabbmin, aabb.first);
        aabbmax = max(aabbmax, aabb.second);   
    }

    if(e->has<JoltBody>())
    {
        // TODO
    }

    if(e->has<EntityGroupInfo>())
        for(auto c : e->comp<EntityGroupInfo>().children)
            getEntityAABB(aabbmin, aabbmax, c.get());
}

void Apps::WorldEditorApp::update()
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

    gizmo.update(cursor);

    if(WorldEntities::selectedFile)
    {
        gizmo.enable();
        // gizmo.isEnable = true;
        // gizmo.parent->comp<State3D>().isActive = ModelStatus::UNDEFINED;

        if(!WorldEntities::selectedFile->has<HierarchyState3D>())
            WorldEntities::selectedFile->set(HierarchyState3D());

        HierarchyState3D &h = WorldEntities::selectedFile->comp<HierarchyState3D>();
        HierarchyState3D s = gizmo.parent->comp<State3D>();
        HierarchyState3D p = WorldEntities::selectedFolder->comp<State3D>();
        HierarchyState3D inv = h;

        if(h.type & 0b001)
            inv.position = (inverse(p.rotation)*(s.position - p.position))/p.scale;

        if(h.type & 0b010)
            inv.rotation = inverse(p.rotation) * s.rotation;

        if(h.type & 0b100)
            inv.scale = s.scale/p.scale;

        h = inv;

        ComponentModularity::synchronizeChildren(WorldEntities::selectedFolder);

        // if(!WorldEntities::selectedFile->has<LevelOfDetailsInfos>())
        //     WorldEntities::selectedFile->set(LevelOfDetailsInfos());

        // auto &lod = WorldEntities::selectedFile->comp<LevelOfDetailsInfos>();
        // lod.computeEntityAABB(WorldEntities::selectedFile);

        vec3 aabbmin = vec3(1e12);
        vec3 aabbmax = vec3(-1e12);
        getEntityAABB(aabbmin, aabbmax, WorldEntities::selectedFile);

        GG::draw->drawBox(
            aabbmin, aabbmax, 0.0, ModelState3D(), vec3(1)
        );

        glLineWidth(10.0);
    }
    else
    {
        // gizmo.isEnable = false;
        gizmo.disable();
    }

    
}

void Apps::WorldEditorApp::clean()
{
    globals.simulationTime.pause();

    globals.currentCamera->setMouseFollow(false);
    globals.currentCamera->setPosition(vec3(0));
    globals.currentCamera->setDirection(vec3(-1, 0, 0));

    ComponentModularity::removeChild(*GlobalInfosSubTab, EntitySpawn::Menu::parent);
    ComponentModularity::removeChild(*GlobalInfosTitleTab, GlobalInfosTitleTab->comp<EntityGroupInfo>().children.back());

    EntitySpawn::clear();
    WorldEntities::clear();

    EntitySpawn::Menu::parent = EntityRef();
    gizmo.clear();
    appRoot = EntityRef();
    App::setController(nullptr);

    GG::sun->shadowCameraSize = vec2(0, 0);
}

