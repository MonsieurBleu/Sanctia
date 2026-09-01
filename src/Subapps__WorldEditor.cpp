#include <App.hpp>
#include <Subapps.hpp>
#include <EntityBlueprint.hpp>
#include <Scripting/ScriptInstance.hpp>
#include <AssetManager.hpp>
#include <EnvironementGenerator.hpp>
#include <ModManager.hpp>

#include <JoltIntegration/PhysicsDebugRenderer.hpp>

EntityRef gizmo;
EntityRef gizmoPos;
EntityRef gizmoRot;
EntityRef gizmoScale;

#define FOLDER_CHAR '#'
#define FOLDER_STR "##"

uint gizmoHistoricMaxSize = 1e4;
std::list<State3D> gizmoHistoric;
std::list<State3D>::iterator gizmoHistoricCurrent;

bool gizmoFollowTerrain = true;

namespace EntitySpawn 
{
    namespace Menu {
        EntityRef parent;
        EntityRef entities;
        EntityRef category;
        EntityRef type;
    }

    namespace list {
        std::unordered_map<std::string, EntityRef> category; // list of category (mods)
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
        for(auto i : AssetLoadInfos::assetList["EntityRef"])
        {
            for(auto k : i.second)
            {
                EntitySpawn::list::category[k.version->name] = EntityRef();
            }
        }

        EntitySpawn::refreshEntityList("", "");

        /*
            Creating the per Type sort menu
        */
        EntitySpawn::Menu::type = newEntity("Spawn Menu - Type", UI_BASE_COMP, WidgetStyle().setautomaticTabbing(8));

        for(auto &i : EntitySpawn::types)
        {
            ComponentModularity::addChild(*EntitySpawn::Menu::type, 
                VulpineBlueprintUI::Toggable(i, "",
                    [](Entity *e, float f){EntitySpawn::refreshEntityList(EntitySpawn::selectedCategory, e->comp<EntityInfos>().name);},
                    [](Entity *e){return e->comp<EntityInfos>().name == EntitySpawn::selectedType ? 0.f : 1.f;},
                    VulpineColorUI::HightlightColorPink
                )
            );
        }

        /*
            Creating the per Category sort menu
        */
        EntitySpawn::Menu::category = VulpineBlueprintUI::StringListSelectionMenu("Spawn Menu - Category",
            EntitySpawn::list::category, 
            [](Entity *e, float f){EntitySpawn::refreshEntityList(e->comp<EntityInfos>().name, EntitySpawn::selectedType);},
            [](Entity *e){return e->comp<EntityInfos>().name == EntitySpawn::selectedCategory ? 0.f : 1.f;},
            -2.,
            VulpineColorUI::HightlightColorRed,
            0.
        );

        /*
            Creating the per menu that actually spawn entities
        */
        EntitySpawn::Menu::entities = VulpineBlueprintUI::StringListSelectionMenu("Spawn Menu - Entites",
            EntitySpawn::list::entities, 
            [](Entity *e, float f)
            {
                auto &n = e->comp<EntityInfos>().name;
                selectedEntity = selectedEntity == n ? "" : n;
            },
            [](Entity *e){return selectedEntity == e->comp<EntityInfos>().name ? 0.f : 1.f;},
            -2.,
            VulpineColorUI::HightlightColorPurple,
            0.
        );

        EntitySpawn::Menu::parent = newEntity("Entity Spawn Menu", 
            UI_BASE_COMP, 
            WidgetStyle().setautomaticTabbing(1),
            EntityGroupInfo({
                // EntitySpawn::Menu::sortTypeSelection, EntitySpawn::Menu::entities

                VulpineBlueprintUI::NamedEntry(U"Category", EntitySpawn::Menu::category, 0.125, true, VulpineColorUI::HightlightColorRed),
                VulpineBlueprintUI::NamedEntry(U"Type", EntitySpawn::Menu::type, 0.125, true, VulpineColorUI::HightlightColorPink),
                VulpineBlueprintUI::NamedEntry(U"Entities To Spawn", EntitySpawn::Menu::entities, 0.125, true, VulpineColorUI::HightlightColorPurple)
            })
        );
    }
}

std::string getFileExtension2(const std::string &fileName)
{
    std::string result;

    auto i = fileName.rbegin();
    while (i != fileName.rend())
    {
        if (*i == FOLDER_CHAR)
            break;

        result = *i + result;

        i++;
    }

    return result;
}


EntityRef SimpleFolderView(
    const std::string name,
    std::vector<Entity*> & list,
    vec4 folderColor,
    vec4 fileColor,
    int columns,
    int rowsOnScreen,
    WidgetButton::InteractFunc ifunc, 
    WidgetButton::UpdateFunc ufunc,
    bool doSorting = true
)
{
    auto listScreen = newEntity(name + " - List Screen",
        UI_BASE_COMP,
        WidgetStyle(),
        WidgetBox([&list, ufunc, ifunc, columns, rowsOnScreen, folderColor, fileColor, doSorting](Entity *parent, Entity *child)
        {            
            auto &children = child->comp<EntityGroupInfo>().children;
            std::vector<EntityRef> childrenToKeep;
            std::vector<Entity*> childrenToAdd;

            /*
                Checking if any children need to be added or removed
            */
            for(const auto &i : list)
            {
                bool isFound = false;
                for(int j = 0; j < children.size(); j++)
                {
                    if(children[j]->comp<EntityGroupInfo>().children[0]->comp<WidgetButton>().usr == (uint64)i)
                    {
                        isFound = true;
                        childrenToKeep.push_back(children[j]);
                        break;
                    }
                }
                if(!isFound)
                    childrenToAdd.push_back(i);
            }

            // If no children need to be added/removed, just stop here
            if(childrenToAdd.empty() and childrenToKeep.size() == children.size())
                return;

            children.clear();
            GG::ManageEntityGarbage();

            // Creating new children
            for(auto &i : childrenToAdd)
            {
                auto name = getFileExtension2(i->comp<EntityInfos>().name);

                auto button = VulpineBlueprintUI::Toggable(name, "", ifunc, ufunc);
                // vec4 color = name.size() > 1 and name[0] == '.' ? folderColor : fileColor;
                vec4 color = name == i->comp<EntityInfos>().name ? fileColor : folderColor;
                // button->comp<WidgetStyle>().setbackgroundColor1(mix(color, VulpineColorUI::LightBackgroundColor1, 0.25f));
                button->comp<WidgetStyle>().setbackgroundColor1(color);
                button->comp<WidgetStyle>().setbackgroundColor2(vec4(vec3(color), 0.65));
                // button->comp<WidgetStyle>().setbackgroundColor2(mix(color, VulpineColorUI::LightBackgroundColor2, 0.35f));
                button->comp<WidgetButton>().setusr((uint64)i);

                auto buttonParent = newEntity(name, 
                    UI_BASE_COMP,
                    WidgetStyle(),
                    EntityGroupInfo({button})
                );

                buttonParent->comp<WidgetBox>().useClassicInterpolation = true;
                // buttonParent->comp<WidgetBox>().specialFittingScript = [](Entity *p, Entity *e)
                // {
                //     e->comp<EntityGroupInfo>().children[0]->comp<WidgetStyle>().setbackGroundStyle(
                //         e->comp<WidgetBox>().isUnderCursor ? UiTileType::SQUARE : UiTileType::SQUARE_ROUNDED
                //     );
                // };

                childrenToKeep.push_back(buttonParent);
            }

            // Sorting and re-ading all children
            if(doSorting)
                std::sort(
                    childrenToKeep.begin(),
                    childrenToKeep.end(),
                    [](const EntityRef &a, const EntityRef &b)
                    {
                        // bool isAFolder = strstr(((Entity*)a->comp<EntityGroupInfo>().children[0]->comp<WidgetButton>().usr)->comp<EntityInfos>().name.c_str(), FOLDER_STR);
                        // bool isBFolder = strstr(((Entity*)b->comp<EntityGroupInfo>().children[0]->comp<WidgetButton>().usr)->comp<EntityInfos>().name.c_str(), FOLDER_STR);

                        bool isAFolder = a->comp<EntityInfos>().name != ((Entity*)a->comp<EntityGroupInfo>().children[0]->comp<WidgetButton>().usr)->comp<EntityInfos>().name;
                        bool isBFolder = b->comp<EntityInfos>().name != ((Entity*)b->comp<EntityGroupInfo>().children[0]->comp<WidgetButton>().usr)->comp<EntityInfos>().name;
                        return isAFolder > isBFolder or strcasecmp(a->comp<EntityInfos>().name.c_str(), b->comp<EntityInfos>().name.c_str()) < 0;
                    }
                );

            for(auto i : childrenToKeep)
                ComponentModularity::addChild(*child, i);

            // Refreshing children transform
            int id = 0;
            for(auto i : child->comp<EntityGroupInfo>().children)
            {
                int columnID = id%columns;
                int rowID = id/columns;
                vec2 tableDim(1.0/(float)columns, 1.0/(float)rowsOnScreen);

                vec2 xrange(tableDim.x*(float)columnID, tableDim.x*(1.f + (float)columnID));
                vec2 yrange(tableDim.y*(float)rowID, tableDim.y*(1.f + (float)rowID));

                i->comp<WidgetBox>().set(xrange*2.f - 1.f, yrange*2.f - 1.f);
                id ++;
            }
        })
    );


    auto scrollBar = newEntity(name + " - Scroll Bar",
        UI_BASE_COMP,
        WidgetStyle(),
        WidgetButton(WidgetButton::Type::SLIDER_2D,
            [rowsOnScreen, columns](Entity *e, vec2 a)
            {
                auto p = e->comp<EntityGroupInfo>().parent->comp<EntityGroupInfo>().children[0];
                int imax = p->comp<EntityGroupInfo>().children[0]->comp<EntityGroupInfo>().children.size()/rowsOnScreen;
                float max = (float)imax/(float)(columns) - 0.75f;
                p->comp<WidgetBox>().scrollOffset.y = -a.y*max;

                p->comp<WidgetBox>().smoothingAnimationSpeed = 0.f;
            },
            [](Entity *e){return vec2(e->comp<WidgetButton>().cur, e->comp<WidgetButton>().cur2);}
        ),
        EntityGroupInfo({
            newEntity(name + " - Scroll Bar 2",
                UI_BASE_COMP,
                WidgetBackground(),
                WidgetStyle()
                    .setbackGroundStyle(UiTileType::SQUARE_ROUNDED)
                    .setbackgroundColor1(fileColor),
                WidgetBox([rowsOnScreen, columns](Entity *p, Entity *c)
                {
                    p = p->comp<EntityGroupInfo>().parent->comp<EntityGroupInfo>().children[0].get();
                    float o = -p->comp<WidgetBox>().scrollOffset.y;

                    int imax = p->comp<EntityGroupInfo>().children[0]->comp<EntityGroupInfo>().children.size()/rowsOnScreen;
                    float max = (float)imax/(float)(columns) - 0.75f + 1.f;

                    float ymin = o/max;
                    float ymax = ymin + 1.f/max;


                    c->comp<WidgetBox>().useClassicInterpolation = true;
                    c->comp<WidgetBox>().smoothingAnimationSpeed = 0.f;

                    if(c->comp<WidgetBox>().initMin.y != ymin*2.f - 1.f)
                        c->comp<WidgetBox>().set(vec2(-1, 1), vec2(ymin, ymax)*2.f - 1.f);
                })
            )
        })
    );

    auto scrollZone = newEntity(name + " - Scroll Zone"
        , UI_BASE_COMP
        , WidgetStyle()
            // .setautomaticTabbing(1)
            .setbackgroundColor1(VulpineColorUI::DarkBackgroundColor1)
            .setbackGroundStyle(UiTileType::SQUARE_ROUNDED)
        , WidgetBackground()
        , WidgetBox([rowsOnScreen, columns](Entity *parent, Entity *child){
            auto &box = child->comp<WidgetBox>();

            int imax = child->comp<EntityGroupInfo>().children[0]->comp<EntityGroupInfo>().children.size()/rowsOnScreen;
            float max = (float)imax/(float)(columns);
            
            if(box.isUnderCursor)
            {
                vec2 off = globals.mouseScrollOffset();
                box.scrollOffset.y += off.y*0.05;

                box.scrollOffset.y = clamp(box.scrollOffset.y,-(float)max+0.75f,0.f);

                globals.clearMouseScroll();
            }

            box.displayRangeMin = box.min;
            box.displayRangeMax = box.max;

            auto &b1 = parent->comp<EntityGroupInfo>().children[0]->comp<WidgetBox>();
            auto &b2 = parent->comp<EntityGroupInfo>().children[1]->comp<WidgetBox>();
            if(max >= 1e-4)
            {
                if(b1.initMax.x != 0.9f)
                {
                    b1.set(vec2(-1., 0.9), vec2(-1., 1.));
                    b2.set(vec2(0.9, 1.0), vec2(-1., 1.));
                }
            }
            else
            {
                if(b1.initMax.x != 1.f)
                {
                    b1.set(vec2(-1., 1.0), vec2(-1., 1.));
                    b2.set(vec2(0.0, 0.0), vec2(-1., 1.));
                }
            }
        })
        , EntityGroupInfo({listScreen})
    );

    scrollZone->comp<WidgetBox>().set(vec2(-1., 0.9), vec2(-1., 1.));
    scrollBar->comp<WidgetBox>().set(vec2(0.9, 1.0), vec2(-1., 1.));

    return newEntity(name + " - Folder View", UI_BASE_COMP, EntityGroupInfo({scrollZone, scrollBar}));
}


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

    void selectFolder(Entity *p)
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

    void createMenu()
    {
        Menu::folderView = SimpleFolderView("World Entities - Folder View",
            selectedFolderContent, 
            VulpineColorUI::HightlightColorGreen,
            VulpineColorUI::HightlightColorCyan,
            3,
            16,
            [](Entity *e, float f)
            {
                selectedFile = (Entity *)e->comp<WidgetButton>().usr;

                const auto &name = e->comp<EntityInfos>().name;
                if(f == 1.f and selectedFile->comp<EntityInfos>().name != name)
                {
                    selectFolder(selectedFile);
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

                    auto folder = newEntity(text, HierarchyState3D(), EntityGroupInfo()); 
                    ComponentModularity::addChild(*selectedFolder, folder);
                    selectedFolderContent.push_back(folder.get());

                    if(!selectedFolder->has<EntitySpawner>())
                        selectedFolder->set<EntitySpawner>(EntitySpawner());

                    EntitySpawner::info i;
                    i.name = text;
                    selectedFolder->comp<EntitySpawner>().onLoading.push_back(i);
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

        

        Menu::hierarchyView = SimpleFolderView("World Entities - Hierarchy View",
            selectedFolderParents, 
            VulpineColorUI::HightlightColorGreen,
            VulpineColorUI::HightlightColorGreen,
            1,
            16,
            [](Entity *e, float f)
            {
                selectFolder((Entity *)e->comp<WidgetButton>().usr);
            },
            [](Entity *e){return (uint64)selectedFolder == e->comp<WidgetButton>().usr ? 0.f : 1.f;},
            false
        );

        
        auto tmp1 = VulpineBlueprintUI::NamedEntry(U"Hierarchy", Menu::hierarchyView, 0.05, true, VulpineColorUI::HightlightColorGreen);
        tmp1->comp<WidgetBox>().set(vec2(-1., -0.5), vec2(-1, 1));
        auto tmp2 = VulpineBlueprintUI::NamedEntry(U"Folder Content", folderViewParent, 0.05, true, VulpineColorUI::HightlightColorCyan);
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
        
        auto name = e->comp<EntityInfos>().name;
        auto filename = saveFolder + name + ".vEntity";
        NOTIF_MESSAGE("Saving Entity : ", filename);

        VulpineTextOutputRef out(new VulpineTextOutput(1 << 16));
        DataLoader<EntityRef>::write(e, out);
        out->saveAs(filename.c_str());

        if(Loader<EntityRef>::loadingInfos.find(name) == Loader<EntityRef>::loadingInfos.end())
            Loader<EntityRef>::addInfos(filename.c_str());
    };
}

EntityRef Apps::WorldEditorApp::UImenu()
{
    WorldEntities::createMenu();

    EntitySpawn::createMenu();

    // Closing all global infos tab menus
    for(auto i : GlobalInfosTitleTab->comp<EntityGroupInfo>().children)
        i->comp<WidgetState>().statusToPropagate = ModelStatus::HIDE;

    // Add the new entity spawn menu
    VulpineBlueprintUI::AddToSelectionMenu(
        GlobalInfosTitleTab, GlobalInfosSubTab,
        EntitySpawn::Menu::parent,
        "Place Entity", ""
    );

    // Automaticcly open the newly created menu
    GlobalInfosTitleTab->comp<EntityGroupInfo>().children.back()->comp<WidgetState>().statusToPropagate = ModelStatus::SHOW;

    return newEntity("WorldEditor APP MENU"
        , UI_BASE_COMP
        , WidgetStyle().setautomaticTabbing(2)
        , EntityGroupInfo(
            {
                WorldEntities::Menu::parent
            })
    );
}

Apps::WorldEditorApp::WorldEditorApp() : SubApps("WorldEditor")
{
    inputs.push_back(&
        InputManager::addEventInput(
            "GIZMO : Translate Mode", GLFW_KEY_T, 0, GLFW_PRESS, [&]() {
                gizmoPos->comp<HierarchyState3D>().isActive = ModelStatus::SHOW;
                gizmoRot->comp<HierarchyState3D>().isActive = ModelStatus::HIDE;
                gizmoScale->comp<HierarchyState3D>().isActive = ModelStatus::HIDE;
            },
            InputManager::Filters::always, false)
    );    

    inputs.push_back(&
        InputManager::addEventInput(
            "GIZMO : Rotate Mode", GLFW_KEY_R, 0, GLFW_PRESS, [&]() {
                gizmoPos->comp<HierarchyState3D>().isActive = ModelStatus::HIDE;
                gizmoRot->comp<HierarchyState3D>().isActive = ModelStatus::SHOW;
                gizmoScale->comp<HierarchyState3D>().isActive = ModelStatus::HIDE;
            },
            InputManager::Filters::always, false)
    );    

    inputs.push_back(&
        InputManager::addEventInput(
            "GIZMO : Scale Mode", GLFW_KEY_E, 0, GLFW_PRESS, [&]() {
                gizmoPos->comp<HierarchyState3D>().isActive = ModelStatus::HIDE;
                gizmoRot->comp<HierarchyState3D>().isActive = ModelStatus::HIDE;
                gizmoScale->comp<HierarchyState3D>().isActive = ModelStatus::SHOW;
            },
            InputManager::Filters::always, false)
    ); 

    inputs.push_back(&
        InputManager::addEventInput(
            "GIZMO : Revert", GLFW_KEY_W, GLFW_MOD_CONTROL, GLFW_PRESS, [&]() {
                if(gizmoHistoricCurrent != gizmoHistoric.begin())
                    gizmo->comp<State3D>() = *(--gizmoHistoricCurrent);
            },
            InputManager::Filters::always, false)
    ); 

    inputs.push_back(&
        InputManager::addEventInput(
            "GIZMO : Revert", GLFW_KEY_Y, GLFW_MOD_CONTROL, GLFW_PRESS, [&]() {
                if(gizmoHistoricCurrent != --gizmoHistoric.end())
                    gizmo->comp<State3D>() = *(++gizmoHistoricCurrent);
            },
            InputManager::Filters::always, false)
    ); 

    inputs.push_back(&
        InputManager::addEventInput(
            "GIZMO : Switch between world and relative translation mode", GLFW_KEY_Z, 0, GLFW_PRESS, [&]() {
                auto & t = gizmoPos->comp<HierarchyState3D>().type;
                t = t == HierarchyState3D::ALL ? HierarchyState3D::SCALE_AND_POS : HierarchyState3D::ALL;

                gizmoPos->comp<State3D>().rotation = quat(1, 0, 0, 0);
            },
            InputManager::Filters::always, false)
    ); 

    inputs.push_back(&
        InputManager::addEventInput(
            "GIZMO : Toggle terrain height follow", GLFW_KEY_X, 0, GLFW_PRESS, [&]() {
                gizmoFollowTerrain = !gizmoFollowTerrain;
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

    gizmoPos = newEntity("Gizmo Pos", State3D(), HierarchyState3D(HierarchyState3D::POS_AND_ROT));
    gizmoRot = newEntity("Gizmo Rot", State3D(), HierarchyState3D(HierarchyState3D::POS_ONLY));
    gizmoScale = newEntity("Gizmo Scale", State3D(), HierarchyState3D(HierarchyState3D::POS_AND_ROT));
    gizmoPos->comp<HierarchyState3D>().isActive = ModelStatus::SHOW;
    gizmoRot->comp<HierarchyState3D>().isActive = ModelStatus::HIDE;
    gizmoScale->comp<HierarchyState3D>().isActive = ModelStatus::HIDE;
    gizmo = newEntity("Gizmo", State3D(), HierarchyState3D(HierarchyState3D::NOTHING), EntityGroupInfo({gizmoPos, gizmoRot, gizmoScale}));

    #define ADD_GIZMO_ELEM(parent, name, pos, rot, color) { \
        EntityRef e = spawnEntity(name, pos, rot); \
        auto mesh = e->comp<EntityModel>()->getChildren()[0]->getMeshes()[0]; \
        mesh->uniforms.add(ShaderUniform(vec3(color), 20)); \
        mesh->depthWrite = false; \
        mesh->sorted = false; \
        ComponentModularity::addChild(*parent, e); \
        e->set<HierarchyState3D>(HierarchyState3D()); \
    }

    ADD_GIZMO_ELEM(gizmoPos, "Gizmo Arrow", vec3(0), quat(radians(vec3(0., 0., 0.))), VulpineColorUI::HightlightColorOrange);
    ADD_GIZMO_ELEM(gizmoPos, "Gizmo Arrow", vec3(0), quat(radians(vec3(0., -90., 0.))), VulpineColorUI::HightlightColorCyan);
    ADD_GIZMO_ELEM(gizmoPos, "Gizmo Arrow", vec3(0), quat(radians(vec3(0., 0., 90.))), VulpineColorUI::HightlightColorPink);

    ADD_GIZMO_ELEM(gizmoPos, "Gizmo Plane", vec3(0, 0.45, 0.45), quat(radians(vec3(0., 0., 0.))), VulpineColorUI::HightlightColorOrange);
    ADD_GIZMO_ELEM(gizmoPos, "Gizmo Plane", vec3(0.45, 0.45, 0), quat(radians(vec3(0., -90., 0.))), VulpineColorUI::HightlightColorCyan);
    ADD_GIZMO_ELEM(gizmoPos, "Gizmo Plane", vec3(0.45, 0, 0.45), quat(radians(vec3(0., 0., 90.))), VulpineColorUI::HightlightColorPink);

    ADD_GIZMO_ELEM(gizmoRot, "Gizmo Disc", vec3(0), quat(radians(vec3(0., 0., 0.))), VulpineColorUI::HightlightColorOrange);
    ADD_GIZMO_ELEM(gizmoRot, "Gizmo Disc", vec3(0), quat(radians(vec3(0., -90., 0.))), VulpineColorUI::HightlightColorCyan);
    ADD_GIZMO_ELEM(gizmoRot, "Gizmo Disc", vec3(0), quat(radians(vec3(0., 0., 90.))), VulpineColorUI::HightlightColorPink);

    ADD_GIZMO_ELEM(gizmoScale, "Gizmo Arrow", vec3(0), quat(radians(vec3(0., 0., 0.))), VulpineColorUI::HightlightColorYellow);
    ADD_GIZMO_ELEM(gizmoScale, "Gizmo Arrow", vec3(0), quat(radians(vec3(0., -90., 0.))), VulpineColorUI::HightlightColorPurple);
    ADD_GIZMO_ELEM(gizmoScale, "Gizmo Arrow", vec3(0), quat(radians(vec3(0., 0., 90.))), VulpineColorUI::HightlightColorGreen);

    ADD_GIZMO_ELEM(gizmoScale, "Gizmo Plane", vec3(0, 0.45, 0.45), quat(radians(vec3(0., 0., 0.))), VulpineColorUI::HightlightColorYellow);
    ADD_GIZMO_ELEM(gizmoScale, "Gizmo Plane", vec3(0.45, 0.45, 0), quat(radians(vec3(0., -90., 0.))), VulpineColorUI::HightlightColorPurple);
    ADD_GIZMO_ELEM(gizmoScale, "Gizmo Plane", vec3(0.45, 0, 0.45), quat(radians(vec3(0., 0., 90.))), VulpineColorUI::HightlightColorGreen);

    {
        EntityRef dummy = spawnEntity("Jolt Test Scene");
        dummy->set<HierarchyState3D>(HierarchyState3D());
        ComponentModularity::addChild(*gizmo, dummy);
    }

    ComponentModularity::addChild(*appRoot, gizmo);

    ComponentModularity::addChild(*appRoot, Blueprint::SpawnMainGameTerrain());

    gizmoHistoric.push_back(gizmo->comp<State3D>());
    gizmoHistoricCurrent = gizmoHistoric.begin();

    JoltVulpine::enablePhysics = true;
    globals.simulationTime.resume();


    ///  TEST
    // {
    //     EntitySpawner comp;
    //     // comp.onLoading.push_back({"Jolt Test Scene", "", HierarchyState3D(HierarchyState3D::POS_AND_ROT)});

    //     comp.onLoading.push_back({"Jolt Test Scene", "EntitySpawnerTest1", HierarchyState3D({vec3(0, 50, 0), quat(1, 0, 0, 0), vec3(0.1)})});
    //     comp.onLoading.push_back({"Jolt Test Scene", "EntitySpawnerTest2", HierarchyState3D({vec3(5, 60, 0), quat(1, 0, 0, 0), vec3(2.0)})});

    //     HierarchyState3D comp2;
    //     comp2.position = vec3(1, 2, 30);
    //     comp2.rotation = angleAxis(0.5f, vec3(0, 1, 0));
    //     comp2.type = HierarchyState3D::NOTHING;
    //     EntityRef test = newEntity("Entity Spawner Test", comp, comp2, State3D());

    //     VulpineTextOutputRef out(new VulpineTextOutput(1 << 16));
    //     DataLoader<EntityRef>::write(test, out);
    //     out->saveAs("data/Entity Spawner Test.vEntity");
    // }


    WorldEntities::world = spawnEntityToParent("World", *appRoot, HierarchyState3D());


    // spawnEntityToParent("Entity Spawner Test", *WorldEntities::world, HierarchyState3D());

    for(int i = 0; i < 128; i++)
    {
        // ComponentModularity::addChild(*WorldEntities::world, newEntity(std::string(FOLDER_STR) + "Folder Number " + std::to_string(i)));
        ComponentModularity::addChild(*WorldEntities::world, newEntity("File Number " + std::to_string(i)));
    }

    WorldEntities::selectFolder(WorldEntities::world.get());
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

    /*
        Gizmo Stuff
    */
    vec3 &p = gizmo->comp<State3D>().position;
    vec3 p2 = p;

    Loader<ScriptInstance>::get("Gizmo Update").run(cursor);
    float d = distance(globals.currentCamera->getPosition(), p)*0.25;
    gizmoPos->comp<State3D>().scale = vec3(d);
    gizmoRot->comp<State3D>().scale = vec3(d);
    gizmoScale->comp<State3D>().scale = vec3(d);

    /*
        Set gizmo height to be constant from terrain if the relevant option is active
    */
    if(gizmoFollowTerrain)
    {
        static float gizmoTerrainHeight = 0.f;

        if(distance(p2.y, p.y) < 1e-4)
            p.y = getTerrainHeight(vec2(p.z, p.x)) - gizmoTerrainHeight;
        else
            gizmoTerrainHeight = getTerrainHeight(vec2(p.z, p.x)) - p.y;
    }
    
    ComponentModularity::synchronizeChildren(gizmo); // Update all child of the gizmo after it has been moved

    /*
        Updating cltr+z/y historic for the gizmo
    */
    if(!threadState["GIZMO_Controled"] and gizmo->comp<State3D>() != *gizmoHistoricCurrent)
    {
        gizmoHistoric.erase(++gizmoHistoricCurrent, gizmoHistoric.end());
        gizmoHistoric.push_back(gizmo->comp<State3D>());
        if(gizmoHistoric.size() > gizmoHistoricMaxSize) gizmoHistoric.pop_front();
        gizmoHistoricCurrent = --gizmoHistoric.end();
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

    EntitySpawn::Menu::parent = 
    gizmo = gizmoPos = gizmoRot = gizmoScale = EntityRef();
    appRoot = EntityRef();
    App::setController(nullptr);

    GG::sun->shadowCameraSize = vec2(0, 0);
}

