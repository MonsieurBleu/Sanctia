#include <App.hpp>
#include <Subapps.hpp>
#include <AssetManager.hpp>
#include <ModManager.hpp>
#include <Draw.hpp>
#include <Constants.hpp>

#include <PlayerController2.hpp>
#include <Game.hpp>
#include <JoltIntegration/PhysicsDebugRenderer.hpp>

#define FOLDER_CHAR '#'
#define FOLDER_STR "##"

#include <Gizmo.hpp>
#include <Flags.hpp>

#include <tinyexr.h>

Gizmo gizmo;
EditorTerrainControler *controllerPTR = nullptr;
uint unsavedChanges = 0;
int selectedFileUpdateCounter = 0;

vec3 camDirTmp = vec3(1, 0, 0);
vec3 camPosTmp = vec3(0);

vec2 mouseDragCommandLastPos = vec2(0);
vec3 mouseDragCommandLastPos3D = vec3(0);

bool showPositionHelper = true;

void setTopDownView()
{
    if(controllerPTR->enable2DView) return;
    controllerPTR->enable2DView = true;

    camDirTmp = globals.currentCamera->getDirection();
    camPosTmp = globals.currentCamera->getPosition();
    // WARNING_MESSAGE(camDirTmp)

    globals.currentCamera->setType(CameraType::ORTHOGRAPHIC);
    controllerPTR->View2DLock = normalize(vec3(0, 1, 0));
    controllerPTR->orthoNear = 512.f;
    globals.currentCamera->wup = vec3(1, 0, 0);
    // float a = globals.windowWidth()/globals.windowHeight();
    GG::skybox->state.scaleScalar(1e3);
}

void clearTopDownView()
{
    if(!controllerPTR->enable2DView) return;
    controllerPTR->enable2DView = false;

    globals.currentCamera->setType(CameraType::PERSPECTIVE);
    globals.currentCamera->wup = vec3(0, 1, 0);
    globals.currentCamera->dimentionFactor = 1.f;
    // globals.currentCamera->setPosition(normalize(vec3(-0.5, 0.5, 0)));
    GG::skybox->state.scaleScalar(1e6);

    globals.currentCamera->setDirection(camDirTmp);
    globals.currentCamera->setPosition(camPosTmp);

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

    void openFolder(Entity *p)
    {
        if(!p or p == selectedFolder) return;

        selectedFolder = p;
        selectedFile = nullptr;
        gizmo.resetHistoric();
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
        unsavedChanges++;

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

    EntityRef addFile(const std::string name)
    {
        selectedFileUpdateCounter = 0;
        auto child = spawnEntityToParent(name, *WorldEntities::selectedFolder, HierarchyState3D());
        addElement(child);

        return child;
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
                gizmo.resetHistoric();
                selectedFileUpdateCounter = 0;
                controllerPTR->targetPosition = selectedFile->comp<State3D>().position;

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
            [](Entity *e)
            {
                auto file = (Entity *)e->comp<WidgetButton>().usr;
                bool isHidden = file->comp<State3D>().isActive == ModelStatus::HIDE;
                bool isFile = !STR_CASE_STR(file->comp<EntityInfos>().name.c_str(), FOLDER_STR);
                bool isConditionnal = false;

                // if(!isFile)
                    for(auto &i : WorldEntities::selectedFolder->comp<EntitySpawner>().onLoading)
                        if(i.child.get() == file and !i.cond.empty())
                        {
                            isConditionnal = true;
                            break;
                        }

                auto &s = e->comp<WidgetStyle>();

                s.textColor1.a = s.textColor2.a = isHidden ? 0.65 : 1.0;

                vec4 color = isFile ? VulpineColorUI::HightlightColorYellow : VulpineColorUI::HightlightColorGreen;
                color = isConditionnal ? VulpineColorUI::HightlightColorBlue : color;
                s.backgroundColor1 = color;
                s.backgroundColor2 = color;
                s.backgroundColor1.a = isHidden ? BASE_ALPHA*0.75 : BASE_ALPHA;
                s.backgroundColor2.a = isHidden ? ALPHA2*0.75 : ALPHA2;


                return (uint64)selectedFile == e->comp<WidgetButton>().usr ? 0.f : 1.f;
            }
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

    void saveWorld()
    {
        unsavedChanges = 0;
        VulpineTextOutputRef out(new VulpineTextOutput(1<<16));
        DataLoader<Flags>::write(Loader<Flags>::get("World"), out);
        out->saveAs((saveFolder + "World.vFlags").c_str());

        saveEntity(world);
    }
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
                .setbackGroundStyle(UiTileType::SQUARE_ROUNDED)
                .setuseInternalSpacing(true)
            // , WidgetBackground()
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

                WorldEntities::selectedFile = WorldEntities::addFile(n).get();
                gizmo.resetHistoric();

               
                gizmo.parent->comp<State3D>() = WorldEntities::selectedFile->comp<State3D>();
                // WARNING_MESSAGE(WorldEntities::selectedFile->comp<State3D>().position);
            },
            [](Entity *e){return 0.f;},
            -0.125,
            VulpineColorUI::HightlightColorYellow,
            0.
        );

        // EntitySpawn::Menu::entities->comp<EntityGroupInfo>().children[0]->set<WidgetBackground>(WidgetBackground());
        // EntitySpawn::Menu::entities->comp<EntityGroupInfo>().children[0]->comp<WidgetStyle>()
        //         .setbackgroundColor1(VulpineColorUI::DarkBackgroundColor1)
        //         .setbackgroundColor2(VulpineColorUI::DarkBackgroundColor1)
        //         .setbackGroundStyle(UiTileType::SQUARE_ROUNDED);

        EntitySpawn::Menu::parent = newEntity("Entity Spawn Menu", 
            UI_BASE_COMP, 
            WidgetStyle().setautomaticTabbing(1),
            EntityGroupInfo({
                newEntity("Entity Spawm Menu 2",
                    UI_BASE_COMP,
                    WidgetStyle().setautomaticTabbing(2),
                    EntityGroupInfo({
                        VulpineBlueprintUI::NamedEntry(U"Filter by Category", EntitySpawn::Menu::category, 0.1, true, VulpineColorUI::HightlightColorRed),
                        VulpineBlueprintUI::NamedEntry(U"Filter by Type", EntitySpawn::Menu::type, 0.1, true, VulpineColorUI::HightlightColorOrange)
                    })
                ),
                VulpineBlueprintUI::NamedEntry(U"Entities To Spawn", EntitySpawn::Menu::entities, 0.05, true, VulpineColorUI::HightlightColorYellow)
            })
        );
    }
}


namespace TerrainEditor 
{
    EntityRef terrain;

    int active = false;
    int textureView = 0;

    float camToBrushDistance = 0.f;

    EntityRef secondaryBrushMenu;
    EntityRef currentToolHelper;
    EntityRef arrow;
    ObjectGroupRef arrowModel;

    std::string currentTarget = Blueprint::terrainConst::mapFileName;
    std::string viableTargets[]{
        Blueprint::terrainConst::mapFileName, "Water Level", "Water Flow", "Grassyness", "Forest Density"
    };

    FrameBuffer FBO;
    FrameBuffer FBO_Copy; // Copy of the current target from the previous draw frame
    FrameBuffer FBO_Copy2; // Copy of the current target before the last stroke, used for undo/redo

    namespace shader 
    {
        ShaderProgram copy;
        ShaderProgram brush;    
    }

    namespace save
    {
        void toDisk()
        {
            auto assetInfos = modImportanceList.getCorrectVersionToUse(
                AssetLoadInfos::assetList["Texture2D"][currentTarget], 
                "Texture 2D", 
                currentTarget
            );

            // auto filename = Loader<Texture2D>::loadingInfos[currentTarget]->buff->getSource();
            // auto filename = assetInfos.file + "_SaveText.exr";
            auto filename = assetInfos.file;
            auto &t = Loader<Texture2D>::get(currentTarget);

            // WARNING_MESSAGE(filename);
            // return;

            EXRImage image;
            EXRHeader header;

            InitEXRHeader(&header);
            InitEXRImage(&image);

            header.num_channels = image.num_channels = 1;
            header.channels = new EXRChannelInfo[1];
            header.channels[0].name[0] = 'V';
            header.channels[0].name[1] = '\0';
            header.pixel_types = new int[1];
            header.pixel_types[0] = TINYEXR_PIXELTYPE_FLOAT;
            header.requested_pixel_types = new int[1];
            header.requested_pixel_types[0] = TINYEXR_PIXELTYPE_HALF;

            header.compression_type = TINYEXR_COMPRESSIONTYPE_ZIP;
            header.pixel_aspect_ratio = 1.f;
            header.tile_level_mode = -1;
            

            image.width = t.getResolution().x;
            image.height = t.getResolution().y;

            unsigned char *source = (unsigned char *)t.getPixelSource();

            image.images = &source;
            
            const char *err;
            // EXRVersion version;
            // ParseEXRVersionFromFile(&version, assetInfos.file.c_str());
            // ParseEXRHeaderFromFile(&header, &version, assetInfos.file.c_str(), &err);


            // ERROR_MESSAGE(
            //     PRINTVAR(filename),
            //     PRINTVAR(header.compression_type),
            //     PRINTVAR(header.tiled),
            //     PRINTVAR(header.chunk_count),
            //     PRINTVAR(header.line_order),
            //     PRINTVAR(header.custom_attributes),
            //     PRINTVAR(header.multipart),
            //     PRINTVAR(header.tile_level_mode),
            //     PRINTVAR(header.num_channels),
            //     PRINTVAR(header.long_name),
            //     PRINTVAR(header.pixel_aspect_ratio)
            // );

            
            int res = SaveEXRImageToFile(&image, &header, filename.c_str(), &err);

            if(res != TINYEXR_SUCCESS)
            {
                ERROR_MESSAGE("Error, can't save terrain texture : ", err);
            }
            else
            {
                NOTIF_MESSAGE("Saved Terrain Texture ", filename);
            }

            delete[] header.channels;
            delete[] header.pixel_types;
            delete[] header.requested_pixel_types;
            // FreeEXRHeader(&header);
        }
    }

    namespace historic 
    {
        struct Node
        {
            Texture2D &texture;
            std::vector<float> changes;
            std::vector<float> original;
            ivec2 res;
            ivec2 off;

            bool changesApplyed = true;

            Node(Texture2D &texture) : texture(texture)
            {
            };

            void applyChanges()
            {
                if(changesApplyed) return;
                changesApplyed = true;

                glBindTexture(GL_TEXTURE_2D, texture.getHandle());
                glTexSubImage2D(GL_TEXTURE_2D,
                    0,
                    off.x, off.y,
                    res.x, res.y,
                    GL_RED, GL_FLOAT,
                    changes.data()
                );

                glBindTexture(GL_TEXTURE_2D, FBO_Copy.getTexture(0).getHandle());
                glTexSubImage2D(GL_TEXTURE_2D,
                    0,
                    off.x, off.y,
                    res.x, res.y,
                    GL_RED, GL_FLOAT,
                    changes.data()
                );

                // WARNING_MESSAGE("REDO",
                //     PRINTVAR(changesApplyed),
                //     PRINTVAR(off),
                //     PRINTVAR(res)
                // )

                //  Loader<Texture2D>::get(currentTarget).updateSourceFromGPU();
            }

            void undoChanges()
            {
                if(!changesApplyed) return;
                changesApplyed = false;

                glBindTexture(GL_TEXTURE_2D, texture.getHandle());
                glTexSubImage2D(GL_TEXTURE_2D,
                    0,
                    off.x, off.y,
                    res.x, res.y,
                    GL_RED, GL_FLOAT,
                    original.data()
                );

                glBindTexture(GL_TEXTURE_2D, FBO_Copy.getTexture(0).getHandle());
                glTexSubImage2D(GL_TEXTURE_2D,
                    0,
                    off.x, off.y,
                    res.x, res.y,
                    GL_RED, GL_FLOAT,
                    original.data()
                );

                // WARNING_MESSAGE("UNDO",
                //     PRINTVAR(changesApplyed),
                //     PRINTVAR(off),
                //     PRINTVAR(res)
                // )

                //  Loader<Texture2D>::get(currentTarget).updateSourceFromGPU();
            }

            void fromTexture()
            {
                changes.resize(res.x*res.y);
                original.resize(res.x*res.y);

                glGetTextureSubImage(texture.getHandle(), 0, 
                    off.x, off.y, 0,
                    res.x, res.y, 1, 
                    GL_RED, GL_FLOAT, sizeof(float)*res.x*res.y, changes.data()
                );

                glGetTextureSubImage(FBO_Copy2.getTexture(0).getHandle(), 0, 
                    off.x, off.y, 0,
                    res.x, res.y, 1, 
                    GL_RED, GL_FLOAT, sizeof(float)*res.x*res.y, original.data()
                );
            }

        };

        vec2 lastChangeMin = vec3(1e6);
        vec2 lastChangeMax = vec3(-1e6);
        bool needUpdate = false;

        const int maxSize = 128;

        struct TargetHistoric : public std::list<Node>
        {
            std::list<Node>::iterator current;
        };

        std::unordered_map<std::string, TargetHistoric> historics;

        void reset()
        {
            FBO.bindTexture(0, 0);
            FBO_Copy2.activate();
            FBO_Copy2.enableDrawBuffers(VulpineTextureAttachement::StaticAndDynamic);
            shader::copy.activate();

            globals.drawFullscreenQuad();

            FBO_Copy2.deactivate();
            shader::copy.deactivate();

            lastChangeMin = vec3(1e6);
            lastChangeMax = vec3(-1e6);
            needUpdate = false;
        }

        void addNode()
        {
            if(!needUpdate) return;

            auto &t = Loader<Texture2D>::get(currentTarget);

            auto &h = historics[currentTarget];

            if(!h.empty())
                h.erase(++h.current, h.end());

            lastChangeMin = floor(vec2(t.getResolution())*(lastChangeMin + 2048.f)/4096.f);
            lastChangeMax = ceil(vec2(t.getResolution())*(lastChangeMax + 2048.f)/4096.f);

            h.push_back(Node(t));
            h.back().off = ivec2(lastChangeMin);
            h.back().res = ivec2(lastChangeMax-lastChangeMin+1.f);
            h.back().fromTexture();

            if(h.size() > maxSize) h.pop_front();

            h.current = --h.end();

            reset();
        }

        void undo()
        {
            auto &h = historics[currentTarget];

            if(h.empty()) return;
            
            h.current->undoChanges();

            if(h.current != h.begin())
                h.current--;
        }

        void redo()
        {
            auto &h = historics[currentTarget];
            
            if(h.empty()) return;
            
            // Since we can't go before the start of the historic,
            // we must check if the changes of the first node is
            // applyed before incrementing the historic any further.
            // This works like a virtual 'node -1'
            if(h.current == h.begin() and !h.current->changesApplyed)
                h.current->applyChanges();
            else
            {
                if(h.current != --h.end())
                    h.current++;
    
                h.current->applyChanges();
            }
        }
    };

    namespace heightChange {
        vec2 uvmin = vec3(1e6);
        vec2 uvmax = vec3(-1e6);
        bool needRefresh = false;

        void reset(){uvmin = vec2(1e6); uvmax = vec2(-1e6); needRefresh = false;};
        void apply()
        {
            if(!needRefresh) return;

            uvmin += Blueprint::terrainConst::terrainSize.x*0.5f;
            uvmax += Blueprint::terrainConst::terrainSize.z*0.5f;

            int imin = floor(uvmin.x/(float)Blueprint::terrainConst::cellSize);
            int jmin = floor(uvmin.y/(float)Blueprint::terrainConst::cellSize);

            int imax = ceil(uvmax.x/(float)Blueprint::terrainConst::cellSize);
            int jmax = ceil(uvmax.y/(float)Blueprint::terrainConst::cellSize);

            ivec2 gridDim = ivec2(Blueprint::terrainConst::terrainSize.x, Blueprint::terrainConst::terrainSize.z)/Blueprint::terrainConst::cellSize;

            auto &heightMap = Loader<Texture2D>::get(Blueprint::terrainConst::mapFileName);
            heightMap.updateSourceFromGPU();

            imin = clamp(imin, 0, gridDim.x);
            imax = clamp(imax, 0, gridDim.x);

            jmin = clamp(jmin, 0, gridDim.y);
            jmax = clamp(jmax, 0, gridDim.y);

            for(int i = imin; i < imax; i++)
                for(int j = jmin; j < jmax; j++)
                {
                    int id = i*gridDim.y + j;

                    auto chunk = terrain->comp<EntityGroupInfo>().children[id];

                    // chunk->comp<JoltBody>()
                    Component<JoltBody>::elements[chunk->ids[PHYSIC]].clean();
                    // chunk->remove<JoltBody>();

                    Blueprint::terrainChunk(
                        chunk, i, j, Blueprint::terrainConst::terrainSize, vec3(0), 
                        heightMap.getResolution(), 
                        (const float *)heightMap.getPixelSource(), 
                        Blueprint::terrainConst::cellSize
                    );

                    __ReparFunc__State3D(*terrain, chunk, *terrain);
                }

            reset();
        }
    }

    namespace brush
    {
        float delta = 0.f;
        vec2 pos = vec2(0);
        vec3 pos3D = vec3(0);
        float range = 1.f;
        float force = 1.f;
        float intensity = 1.f;
        int tool = 0;
        int target = 0;
        float minHeight = 0.f;

        vec3 color = VulpineColorUI::HightlightColorPurple;
        
        GENERATE_ENUM_FAST_REVERSE(BrushTool, Add, Substract, Flatten, Slope, Smooth, Noise, Erosion)

        vec2 forceIntensity[BrushTool::Erosion+1] = {vec2(1, 0.), vec2(1, 0), vec2(1, 0.), vec2(1, 0.5), vec2(1, 0.) ,vec2(1, 0.5), vec2(1, 0.5)};
    }

    void setTarget(std::string target)
    {
        currentTarget = target;

        FBO = FrameBuffer();
        
        auto &texture = Loader<Texture2D>::get(target);
        FBO.addTexture(texture.setAttachement(GL_COLOR_ATTACHMENT0)).generate();

        FBO_Copy.resizeAll(texture.getResolution());
        FBO_Copy2.resizeAll(texture.getResolution());

        for(auto i : terrain->comp<EntityGroupInfo>().children)
        {
            i->comp<EntityModel>()->getMeshes()[0]->setMap(4, texture);
        }
    
        FBO.bindTexture(0, 0);
        FBO_Copy.activate();
        FBO_Copy.enableDrawBuffers(VulpineTextureAttachement::StaticAndDynamic);
        shader::copy.activate();

        globals.drawFullscreenQuad();

        FBO_Copy.deactivate();
        shader::copy.deactivate();


        FBO.bindTexture(0, 0);
        FBO_Copy2.activate();
        FBO_Copy2.enableDrawBuffers(VulpineTextureAttachement::StaticAndDynamic);
        shader::copy.activate();

        globals.drawFullscreenQuad();

        FBO_Copy2.deactivate();
        shader::copy.deactivate();
    }

    EntityRef getMenu();

    vec3 getToolColor(int t)
    {
        switch (t) {
            case brush::BrushTool::Add : 
            return VulpineColorUI::HightlightColorGreen;
            break;

            case brush::BrushTool::Substract : 
            return VulpineColorUI::HightlightColorRed;
            break;
            
            case brush::BrushTool::Smooth : 
            return VulpineColorUI::HightlightColorYellow;
            break;

            case brush::BrushTool::Slope : 
            return VulpineColorUI::HightlightColorPink;
            break;

            case brush::BrushTool::Flatten : 
            return VulpineColorUI::HightlightColorOrange;
            break;

            case brush::BrushTool::Noise : 
            return  VulpineColorUI::HightlightColorCyan;
            break;

            case brush::BrushTool::Erosion : 
            return VulpineColorUI::HightlightColorBlue;
            break;
        };

        return vec3(0);
    }

    void init(EntityRef appRoot)
    {
        arrow = spawnEntityToParent("Gizmo Arrow", *appRoot, HierarchyState3D());
        // arrowModel = Loader<ObjectGroup>::get("Gizmo Arrow").copy();
        auto mesh = arrow->comp<EntityModel>()->getChildren()[0]->getMeshes()[0]; 
        static vec3 color = vec3(1, 0, 0);
        mesh->uniforms.add(ShaderUniform(vec3(color), 20)); 
        mesh->depthWrite = true; 
        mesh->sorted = true; 

        auto &terrainShader = Loader<MeshMaterial>::get("terrain_paintPBR");
        terrainShader->uniforms.add(ShaderUniform(&active,       36));
        terrainShader->uniforms.add(ShaderUniform(&textureView,  37));
        terrainShader->uniforms.add(ShaderUniform(&brush::range, 38));
        terrainShader->uniforms.add(ShaderUniform(&mouseDragCommandLastPos3D,   39));
        terrainShader->uniforms.add(ShaderUniform(&brush::color, 40));
        terrainShader->uniforms.add(ShaderUniform(&brush::force, 41));
        terrainShader->uniforms.add(ShaderUniform(&brush::intensity, 42));
        terrainShader->uniforms.add(ShaderUniform(&brush::target, 43));


        shader::copy = ShaderProgram(
            Loader<ShaderFragPath>::get("Auto Exposure Pass COPY").path,
            Loader<ShaderVertPath>::get("PP_basic").path,
            "",
            globals.standartShaderUniform2D()
        );

        shader::brush = ShaderProgram(
            Loader<ShaderFragPath>::get("Terrain Brush").path,
            Loader<ShaderVertPath>::get("Terrain Brush").path,
            "",
            globals.standartShaderUniform2D()
        );

        shader::brush.addUniform(ShaderUniform(&brush::delta, 32));
        shader::brush.addUniform(ShaderUniform(&brush::pos, 33));
        shader::brush.addUniform(ShaderUniform(&brush::range, 34));
        shader::brush.addUniform(ShaderUniform(&brush::force, 35));
        shader::brush.addUniform(ShaderUniform(&brush::intensity, 36));
        shader::brush.addUniform(ShaderUniform(&brush::tool, 37));
        shader::brush.addUniform(ShaderUniform(&brush::target, 38));
        shader::brush.addUniform(ShaderUniform(&brush::minHeight, 39));

        FBO_Copy.addTexture(
            Texture2D()
                .setResolution(vec2(1024))
                .setInternalFormat(GL_R32F)
                .setFormat(GL_RED)
                .setPixelType(GL_FLOAT)
                .setFilter(GL_LINEAR)
                .setWrapMode(GL_CLAMP_TO_EDGE)
                .setAttachement(GL_COLOR_ATTACHMENT0)
        ).generate();

        FBO_Copy2.addTexture(
            Texture2D()
                .setResolution(vec2(1024))
                .setInternalFormat(GL_R32F)
                .setFormat(GL_RED)
                .setPixelType(GL_FLOAT)
                .setFilter(GL_LINEAR)
                .setWrapMode(GL_CLAMP_TO_EDGE)
                .setAttachement(GL_COLOR_ATTACHMENT0)
        ).generate();

        setTarget(Blueprint::terrainConst::mapFileName);

        secondaryBrushMenu = getMenu();
        // secondaryBrushMenu->set(WidgetBackground());
        secondaryBrushMenu->comp<WidgetStyle>()
            .setautomaticTabbing(3)
            .setbackgroundColor1(VulpineColorUI::DarkBackgroundColor2Opaque)
            .setbackGroundStyle(UiTileType::SQUARE_ROUNDED)
            ;
        secondaryBrushMenu->set<WidgetBox>(WidgetBox([](Entity *p, Entity *c){
            c->comp<WidgetState>().statusToPropagate = c->comp<WidgetState>().status =
                TerrainEditor::active and
                p->comp<WidgetBox>().max == vec2(1) and
                p->comp<WidgetBox>().min == vec2(-1)
                ? 
                ModelStatus::SHOW : ModelStatus::HIDE;            
        }));

        secondaryBrushMenu->comp<WidgetBox>().set(vec2(-1.0, -0.75), vec2(-0.6, 0.6));

        ComponentModularity::addChild(*EDITOR::MENUS::GameScreen, TerrainEditor::secondaryBrushMenu);
    
        currentToolHelper = newEntity("Current Tool Helper", UI_BASE_COMP,
            WidgetBackground(),
            WidgetText(),
            WidgetStyle()
                .setbackGroundStyle(UiTileType::SQUARE_ROUNDED)
                .settextColor1(VulpineColorUI::HightlightColorYellow)
                .setbackgroundColor1(VulpineColorUI::DarkBackgroundColor2),
            WidgetBox([](Entity *p, Entity *c)
            {
                vec2 screenPos = (globals.mousePosition()/vec2(globals.windowSize()))*2.f - 1.f;

                c->comp<WidgetBox>().useClassicInterpolation = true;
                c->comp<WidgetBox>().smoothingAnimationSpeed = 0.f;
                c->comp<WidgetBox>().set(
                    vec2(screenPos.x-0.05, screenPos.x+0.05),
                    vec2(screenPos.y+0.04, screenPos.y+0.08)
                );


                // WARNING_MESSAGE(screenPos)

                c->comp<WidgetText>().text = UFTconvert.from_bytes(
                    TerrainEditor::brush::BrushToolReverseMap[TerrainEditor::brush::tool]
                );

                c->comp<WidgetStyle>().settextColor1(vec4(TerrainEditor::brush::color, BASE_ALPHA));

                // WARNING_MESSAGE(TerrainEditor::brush::BrushToolReverseMap[TerrainEditor::brush::tool])

            })
        );
        
        // ComponentModularity::addChild(*EDITOR::MENUS::GameScreen, currentToolHelper);
        // WARNING_MESSAGE(SubApps::getCurrentRoot()->toStr())
        ComponentModularity::addChild(*appRoot, currentToolHelper);
    }

    void clear()
    {
        FBO = FrameBuffer();
        terrain = EntityRef();
        ComponentModularity::removeChild(*EDITOR::MENUS::GameScreen, secondaryBrushMenu);
        // ComponentModularity::removeChild(*EDITOR::MENUS::GameScreen, currentToolHelper);
        currentToolHelper = secondaryBrushMenu = EntityRef();
    }

    void update(vec2 cursor)
    {
        // cursor = (mouseDragCommandLastPos/vec2(globals.windowSize()))*2.f - 1.f;
        // auto &box = EDITOR::MENUS::GameScreen->comp<WidgetBox>();
        // cursor = ((cursor-box.min)/(box.max - box.min));

        brush::color = getToolColor(brush::tool);
        brush::force = brush::forceIntensity[TerrainEditor::brush::tool].x;

        // brush::force *= currentTarget == viableTargets[0] ? 1.f : 256.f;
        for(int i = 0; i < viableTargets[0].size(); i++)
            if(currentTarget == viableTargets[i])
                brush::target = i;


        brush::intensity = brush::forceIntensity[TerrainEditor::brush::tool].y;

        GrassGenerator::active = !textureView;

        Loader<ScriptInstance>::get("Terrain Editor Update").run(cursor);

        shader::copy.reset();
        shader::brush.reset();

        vec3 raydir = threadState["TERRAIN_EDITOR_RayDirection"];
        vec3 rayOrigin = globals.currentCamera->getPosition();

        const int maxIt = 128;
        const float step = 5.f;
        const vec2 dir = normalize(vec2(raydir.z, raydir.x));
        const float stepH = raydir.y / length(vec2(raydir.z, raydir.x));
        vec2 pos = vec2(rayOrigin.z, rayOrigin.x);
        float rayH = globals.currentCamera->getPosition().y;

        for(int i = 0; i < maxIt; i++)
        {
            float h = getTerrainHeight(pos);
            
            if(rayH <= h) break;
            
            rayH += stepH*step;
            pos += dir*step;
        }

        float minDist = 1e6;
        vec3 minDistPos = vec3(pos.y, rayH, pos.x);
        for(int i = 0; i < int(step); i++)
        {
            float h = getTerrainHeight(pos);
            float d = distance(h, rayH);
            
            if(d < minDist)
            {
                minDist = d;
                minDistPos = vec3(pos.y, rayH, pos.x);
            }

            rayH -= stepH;
            pos -= dir;
        }
        
        // if(minDist < 2.f)
        // {
        //     const float maxAngleIt = 16.0;
            
        //     for(float i = 0; i < maxAngleIt; i++)
        //     {
        //         float a = PI*2.f*i/maxAngleIt;
        //         vec2 np = vec2(mouseDragCommandLastPos3D.x, mouseDragCommandLastPos3D.z) + brush::range*vec2(cos(a), sin(a));
        //         GG::draw->drawSphere(
        //             vec3(np.x, getTerrainHeight(vec2(np.y, np.x)), np.y), 
        //             brush::range/8.0, 
        //             0., 
        //             ModelState3D(), 
        //             VulpineColorUI::HightlightColorPurple
        //         );
        //     }
            
        //     GG::draw->drawSphere(mouseDragCommandLastPos3D, brush::range/4.0, 0., ModelState3D(), VulpineColorUI::HightlightColorPurple);
        // }

        camToBrushDistance = distance(minDistPos, globals.currentCamera->getPosition());
        brush::pos3D = minDistPos;

        if(currentTarget == viableTargets[2])
        {
            arrow->comp<State3D>().isActive = ModelStatus::SHOW;
            arrow->comp<State3D>().rotation = quat(vec3(0., brush::intensity*PI*2.f, 0.));
            arrow->comp<State3D>().scale = vec3(5.f);

            vec3 p(
                brush::pos3D.x, 
                getBiomeMap(vec2(brush::pos3D.z, brush::pos3D.x), "Water Level")*512.f + 2.f,
                // brush::pos3D.y + 2.0,
                brush::pos3D.z
            );
            arrow->comp<State3D>().position = p;
            // setEntityTransform(*arrow, arrow->comp<State3D>());

            // WARNING_MESSAGE(
            //     PRINTVAR(arrow->comp<State3D>().rotation),
            //     PRINTVAR(arrow->comp<State3D>().position),
            //     PRINTVAR(arrow->comp<EntityModel>().inScene)
            // )

            arrow->remove<HierarchyState3D>();
        }
        else
        {
            // arrow->comp<State3D>().isActive = ModelStatus::HIDE;
            // setEntityTransform(*arrow, arrow->comp<State3D>());
        }

        bool doEdition = 
            globals.mouseLeftClickDown() and 
            (secondaryBrushMenu->comp<WidgetState>().statusToPropagate == ModelStatus::HIDE or
            !secondaryBrushMenu->comp<WidgetBox>().isUnderCursor);

        if(minDist < 2.f and doEdition and globals.currentCamera->getMouseFollow())
        {
            /*
                Pass 1 : copying the texture to have it as inputs
            */
            FBO_Copy.bindTexture(0, 0);
            FBO.activate();
            FBO.enableDrawBuffers(VulpineTextureAttachement::StaticAndDynamic);
            shader::copy.activate();

            globals.drawFullscreenQuad();

            FBO.deactivate();
            shader::copy.deactivate();

            /*
                Pass 2 : applying the brush
            */
            FBO.bindTexture(0, 0);
            FBO_Copy.activate();
            FBO_Copy.enableDrawBuffers(VulpineTextureAttachement::StaticAndDynamic);
            shader::brush.activate();
            
            brush::pos = vec2(minDistPos.x, minDistPos.z)/2048.f;
            brush::delta = globals.appTime.getDelta();

            brush::minHeight = currentTarget == viableTargets[1] ? minDistPos.y/512.f : 0.f;

            globals.drawFullscreenQuad();

            FBO_Copy.deactivate();
            shader::brush.deactivate();

            if(currentTarget == viableTargets[0])
            {
                heightChange::uvmax = max(heightChange::uvmax, vec2(minDistPos.x, minDistPos.z)+brush::range);
                heightChange::uvmin = min(heightChange::uvmin, vec2(minDistPos.x, minDistPos.z)-brush::range);
                heightChange::needRefresh = true;
            }

            historic::lastChangeMin = min(historic::lastChangeMin, vec2(minDistPos.x, minDistPos.z) - brush::range);
            historic::lastChangeMax = max(historic::lastChangeMax, vec2(minDistPos.x, minDistPos.z) + brush::range);
            historic::needUpdate = true;
        }
        else
        {
            historic::addNode();
        }

        /*
            The erosion tool needs up-to date mip maps from the previous changes.
            No need to update it each frame, because it'll break the erosion tool.
            Only one update is needed, if the current tool is erosion and they are
            changes that aren't represented in the mip maps.
        */
        static bool mipmapNeedUpdate = false;
        if(doEdition and brush::tool != brush::BrushTool::Erosion)
        {
            mipmapNeedUpdate = true;
        }
        
        if(brush::tool == brush::BrushTool::Erosion and mipmapNeedUpdate)
        {
            Loader<Texture2D>::get(currentTarget).generate(true);
            mipmapNeedUpdate = false;
        }
        
        if(globals.appTime.getUpdateCounter()%8 == 0 and doEdition)
        {
            // TODO : update with a version using sub image function to just pull the changes AABB
            Loader<Texture2D>::get(currentTarget).updateSourceFromGPU();
        }


    }

    EntityRef getMenu()
    {
        auto brushOptions = VulpineBlueprintUI::NamedEntry(U"Brush Options", 
            newEntity("Brush Options", UI_BASE_COMP, 
                WidgetStyle().setautomaticTabbing(6).setuseInternalSpacing(true),
                EntityGroupInfo({

                    VulpineBlueprintUI::NamedEntry(U"Range",
                        VulpineBlueprintUI::ValueInput("Range",
                            [](float f){brush::range = f;}, 
                            [](){return brush::range;}, 
                            1.f, 1024.f, 0.25, 2.0, VulpineColorUI::HightlightColorBlue
                        ), 0.25
                    ),
                    
                    VulpineBlueprintUI::NamedEntry(U"Force",
                        VulpineBlueprintUI::ValueInputSlider("Force", 0.f, 10.f, 1e3, 
                            [](float f){brush::forceIntensity[brush::tool].x = f;}, 
                            [](){return brush::forceIntensity[brush::tool].x;}, 
                            VulpineColorUI::HightlightColorBlue
                        ), 0.25
                    ),
                    
                    VulpineBlueprintUI::NamedEntry(U"Intensity",
                        VulpineBlueprintUI::ValueInputSlider("Intensity", 0.f, 1.f, 1e2, 
                            [](float f){brush::forceIntensity[brush::tool].y = f;}, 
                            [](){return brush::forceIntensity[brush::tool].y;}, 
                            VulpineColorUI::HightlightColorBlue
                        ), 0.25
                    ),

                    VulpineBlueprintUI::Toggable2(
                        "Show Target Texture", "",
                        [](Entity *e, float f){textureView = !textureView;}, 
                        [](Entity *e){return textureView ? 0.f : 1.f;}, 
                        VulpineColorUI::HightlightColorBlue
                    )
                    
                })
            ), 
            0.125, true
        );

        auto brushToolMenu = newEntity("Brush Tool", UI_BASE_COMP,
            WidgetStyle().setuseInternalSpacing(true).setautomaticTabbing(7)
        );

        for(int i = 0; i < brush::BrushToolMap.size(); i++)
        {
            ComponentModularity::addChild(*brushToolMenu, VulpineBlueprintUI::Toggable(
                brush::BrushToolReverseMap[i], "",
                [i](Entity *e, float f){brush::tool = i;}, 
                [i](Entity *e){return brush::tool == i ? 0.f : 1.f;}, 
                getToolColor(i)
            ));
        }

        auto brushTools = VulpineBlueprintUI::NamedEntry(U"Brush Tool", brushToolMenu,0.125, true);

        auto brushTargetMenu = newEntity("Brush Tool", UI_BASE_COMP,
            WidgetStyle().setuseInternalSpacing(true).setautomaticTabbing(6)
        );

        for(auto i : viableTargets)
        {
            ComponentModularity::addChild(*brushTargetMenu, VulpineBlueprintUI::Toggable(
                i, "",
                [i](Entity *e, float f){setTarget(i);}, 
                [i](Entity *e){return currentTarget == i ? 0.f : 1.f;}, 
                VulpineColorUI::HightlightColorCyan
            ));
        }

        auto brushTarget = VulpineBlueprintUI::NamedEntry(U"Target", brushTargetMenu, 0.125, true);

        return newEntity("Terrain Editor Menu", UI_BASE_COMP, 
            WidgetStyle().setautomaticTabbing(-3).setuseInternalSpacing(true),
            EntityGroupInfo({
                brushOptions, brushTools, brushTarget
            })        
        );
    }
};

namespace StreetView
{
    bool active = false;
    bool inStreetView = false;

    PlayerController2 playerControl;
    Controller *tmpControl;

    vec3 spawnPosition = vec3(0);

    EntityRef button;

    void enable()
    {
        TerrainEditor::heightChange::apply();

        camDirTmp = globals.currentCamera->getDirection();
        camPosTmp = globals.currentCamera->getPosition();

        HierarchyState3D s;
        s.position = spawnPosition;
        GG::playerEntity = spawnEntityToParent("Jolt Player", *SubApps::getCurrentRoot(), s);

        tmpControl = globals.getController();
        App::setController(&playerControl);
        inStreetView = true;
    }

    void disable()
    {
        // Component<JoltBody>::elements[GG::playerEntity->ids[PHYSIC]].clean();
        // Component<JoltBody>::elements[GG::playerEntity->ids[GRAPHIC]].clean();
        ComponentModularity::removeChild(*SubApps::getCurrentRoot(), GG::playerEntity);
        // WARNING_MESSAGE(PRINTVAR(GG::playerEntity), PRINTVAR(GG::playerEntity.use_count()))
        GG::playerEntity = EntityRef();

        GG::ManageEntityGarbage__WithPhysics();
        
        App::setController(tmpControl);
        playerControl = PlayerController2();
        inStreetView = false;
        active = false;

        globals.currentCamera->setDirection(camDirTmp);
        globals.currentCamera->setPosition(camPosTmp);
    }


    void init()
    {
        button = newEntity("Street View Toggle", UI_BASE_COMP, 
            WidgetBox([](Entity *p, Entity *c){
                // c->comp<WidgetState>().statusToPropagate = c->comp<WidgetState>().status =
                //     TerrainEditor::active and
                //     p->comp<WidgetBox>().max == vec2(1) and
                //     p->comp<WidgetBox>().min == vec2(-1)
                //     ? 
                //     ModelStatus::SHOW : ModelStatus::HIDE;
            }),
            EntityGroupInfo({
                VulpineBlueprintUI::Toggable("Street View", "Street View", 
                    [](Entity *e, float f)
                    {
                        StreetView::active = true;
                    },
                    [](Entity *e)
                    {
                        return StreetView::active ? 0.f : 1.f;
                    }
                
                )
            })
        );
        
        button->comp<WidgetBox>().set(vec2(0.9, 0.98), vec2(0.9, 0.98));

        ComponentModularity::addChild(*EDITOR::MENUS::GameScreen, button);
    }

    void update(vec2 cursor)
    {
        // WARNING_MESSAGE(PRINTVAR(GG::playerEntity), PRINTVAR(GG::playerEntity.use_count()))

        if(active and !inStreetView)
        {
            cursor.y = 1.0-cursor.y;
    
            float depth = 0;
    
            auto &depthBuffer = Game::defferedBuffer->getTexture(1);
    
            glGetTextureSubImage(depthBuffer.getHandle(), 0, 
                cursor.x*depthBuffer.getResolution().x, cursor.y*depthBuffer.getResolution().y, 0,
                1, 1, 1, 
                GL_DEPTH_COMPONENT, GL_FLOAT, sizeof(float), &depth
            );
    
            vec2 uvNor = 2.f*cursor - 1.f;
            mat4 iproj = inverse(globals.currentCamera->getProjectionMatrix());
            mat4 iview = inverse(globals.currentCamera->getViewMatrix());
            vec4 ndc(uvNor, depth, 1.0);
            vec4 viewPos = iproj * ndc;
            viewPos /= viewPos.w;
            spawnPosition = vec3(iview*viewPos);
    
            GG::draw->drawSphere(vec3(0), 2.0, 0, ModelState3D().setPosition(spawnPosition + vec3(0, 2.5, 0)).setScale(vec3(0.3, 1.0, 0.3)), VulpineColorUI::HightlightColorYellow);
        }
    }

    void clear()
    {
        disable();

        ComponentModularity::removeChild(*EDITOR::MENUS::GameScreen, button);
    }
}


namespace Biome
{
    EntityScatterer *scatterer;
    EntityRef entity;

    void init()
    {
        scatterer = &Loader<EntityScatterer>::get("Forest");
        // scatterer->generateInit(vec2(-2000.f), vec2(2000.f), entity = newEntity("Biome", State3D(), EntityGroupInfo()));
    }

    void update()
    {
        // scatterer->generateFrame(10.f);
    }

    void clear()
    {
        entity = EntityRef();
    }
}


EntityRef Apps::WorldEditorApp::UImenu()
{
    EntitySpawn::createMenu();
    WorldEntities::createMenu();

    // Closing all global infos tab menus
    for(auto i : GlobalInfosTitleTab->comp<EntityGroupInfo>().children)
        i->comp<WidgetState>().statusToPropagate = ModelStatus::HIDE;

    auto selectedEntityMenu = newEntity("Selected Entity", UI_BASE_COMP,
        WidgetStyle().setautomaticTabbing(3).setuseInternalSpacing(true),
        EntityGroupInfo({
            newEntity("Selected Entity - Hide/Show/Remove", UI_BASE_COMP, 
                WidgetStyle().setautomaticTabbing(1),
                EntityGroupInfo({
                    VulpineBlueprintUI::Toggable2("Show", "",
                        [](Entity *e, float f)
                        {
                            if(WorldEntities::selectedFile and WorldEntities::selectedFile->has<State3D>())
                            {
                                auto &i = WorldEntities::selectedFile->comp<HierarchyState3D>().isActive;
                                i = i == ModelStatus::HIDE ? ModelStatus::UNDEFINED : ModelStatus::HIDE;
                            }
                        },
                        [](Entity *e)
                        {
                            return 
                                WorldEntities::selectedFile and 
                                WorldEntities::selectedFile->has<State3D>() and 
                                WorldEntities::selectedFile->comp<State3D>().isActive != ModelStatus::HIDE
                                ? 0.f : 1.f;
                        }, VulpineColorUI::HightlightColorOrange
                    ),
                    VulpineBlueprintUI::Toggable("Remove", "",
                        [](Entity *e, float f)
                        {
                            if(WorldEntities::selectedFile)
                            {
                                /* To successfully remove a file, we need to 
                                        - remove it from the child list of the folder
                                        - remove the entity spawner attached to it
                                        - re-open the selected folder to refresh everything safely
                                */
                                ComponentModularity::removeChild(*WorldEntities::selectedFolder, WorldEntities::selectedFile);
                                
                                auto &list = WorldEntities::selectedFolder->comp<EntitySpawner>().onLoading;
        
                                for(auto i = list.begin(); i != list.end(); i++)
                                    if(i->child.get() == WorldEntities::selectedFile)
                                    {
                                        list.erase(i);
                                        break;
                                    }
                                
                                auto tmp = WorldEntities::selectedFolder;
                                WorldEntities::selectedFolder = nullptr; // doing this force the refresh of all folder content
                                WorldEntities::openFolder(tmp);
                            }
                        },
                        [](Entity *e)
                        {
                            return WorldEntities::selectedFile ? 0.f : 1.f;
                        }, VulpineColorUI::HightlightColorRed
                    )
                })
            
            ),

            VulpineBlueprintUI::ColoredConstEntry("Filename", [](){
                std::u32string name = U"[No Selected Entity]";
                if(WorldEntities::selectedFile)
                {
                    auto elem = Loader<EntityRef>::loadingInfos.find(WorldEntities::selectedFile->comp<EntityInfos>().name);

                    if(elem == Loader<EntityRef>::loadingInfos.end())
                        name = U"[Entity Not Saved Yet]";
                    else
                        name = UFTconvert.from_bytes(elem->second->buff->getSource()); // long ass line
                }

                return name;
            }, VulpineColorUI::LightBackgroundColor1, false, 0.25),

            VulpineBlueprintUI::ColoredConstEntry("Fullname", [](){
                std::u32string name = U"[No Selected Entity]";
                if(WorldEntities::selectedFile)
                    name = UFTconvert.from_bytes(WorldEntities::selectedFile->comp<EntityInfos>().name);

                return name;
            }, VulpineColorUI::LightBackgroundColor1, false, 0.25)

        })
    );

    auto selectedEntityConditionMenu = newEntity("Entity Spawn Conditions", UI_BASE_COMP,
        WidgetStyle().setautomaticTabbing(3).setuseInternalSpacing(true),
        EntityGroupInfo({

            newEntity("Entity Spawn Condition - Subtab 1", UI_BASE_COMP, 
                WidgetStyle().setautomaticTabbing(1),
                EntityGroupInfo({
                    VulpineBlueprintUI::Toggable2("Is Conditionnal", "",
                        [](Entity *e, float f)
                        {
                            if(WorldEntities::selectedFile)
                            {
                                unsavedChanges++;

                                std::string name = WorldEntities::selectedFile->comp<EntityInfos>().name;

                                bool isFile = !STR_CASE_STR(name.c_str(), FOLDER_STR);

                                if(isFile) return;

                                for(auto &i : WorldEntities::selectedFolder->comp<EntitySpawner>().onLoading)
                                    if(i.child.get() == WorldEntities::selectedFile)
                                    {
                                        if(i.cond.empty())
                                        {
                                            // if(isFile)
                                                // name = WorldEntities::selectedFolder->comp<EntityInfos>().name + "##" + name;

                                            i.cond = name;

                                            Loader<Flags>::get("World").setFlag(name, true);
                                        }
                                        else
                                            i.cond.clear();
                                    }
                            }
                        },
                        [](Entity *e)
                        {
                            if(!WorldEntities::selectedFile) return 1.f;

                            for(auto &i : WorldEntities::selectedFolder->comp<EntitySpawner>().onLoading)
                                if(i.child.get() == WorldEntities::selectedFile and !i.cond.empty())
                                    return 0.f;

                            return 1.f;
                        }, VulpineColorUI::HightlightColorBlue
                    ),
                    VulpineBlueprintUI::ColoredConstEntry("Value", [](){
                        std::u32string name = U"";
                        if(WorldEntities::selectedFile)
                        {
                            for(auto &i : WorldEntities::selectedFolder->comp<EntitySpawner>().onLoading)
                                if(i.child.get() == WorldEntities::selectedFile and !i.cond.empty())
                                {
                                    auto &f = Loader<Flag>::get(i.cond);
                                    name = UFTconvert.from_bytes(f->as_string());
                                    break;
                                }
                        }
                        return name;
                    }, VulpineColorUI::HightlightColorBlue),
                })
            ),


            newEntity("Entity Spawn Condition - Subtab 2", UI_BASE_COMP, 
                WidgetStyle().setautomaticTabbing(1),
                EntityGroupInfo({
                    VulpineBlueprintUI::ColoredConstEntry("Type", [](){
                        std::u32string name = U"";
                        if(WorldEntities::selectedFile)
                        {
                            for(auto &i : WorldEntities::selectedFolder->comp<EntitySpawner>().onLoading)
                                if(i.child.get() == WorldEntities::selectedFile and !i.cond.empty())
                                {
                                    auto &f = Loader<Flag>::get(i.cond);
                                    name = UFTconvert.from_bytes(f->typeToString());
                                    break;
                                }
                        }
                        return name;
                    }, VulpineColorUI::HightlightColorBlue, false, 0.25),
                
                    VulpineBlueprintUI::ColoredConstEntry("Methode", [](){
                        std::u32string name = U"";
                        if(WorldEntities::selectedFile)
                        {
                            for(auto &i : WorldEntities::selectedFolder->comp<EntitySpawner>().onLoading)
                                if(i.child.get() == WorldEntities::selectedFile and !i.cond.empty())
                                {
                                    auto &f = Loader<Flag>::get(i.cond);

                                    if(f->isLogicBlock)
                                        name = U"Inline Logic BLock";
                                    else if(f->isScripted)
                                        name = U"Lua Scirpt";
                                    else
                                        name = U"Save File Constant";
                                    break;
                                }
                        }
                        return name;
                    }, VulpineColorUI::HightlightColorBlue, false, 0.25),
                })
            ),

            VulpineBlueprintUI::ColoredConstEntry("Formula", [](){
                std::u32string name = U"";
                if(WorldEntities::selectedFile)
                {
                    for(auto &i : WorldEntities::selectedFolder->comp<EntitySpawner>().onLoading)
                        if(i.child.get() == WorldEntities::selectedFile and !i.cond.empty())
                        {
                            auto &f = Loader<Flag>::get(i.cond);

                            if(f->isLogicBlock)
                                name = UFTconvert.from_bytes(((LogicFlag*)f.flag.get())->logicBlock);
                            else if(f->isScripted)
                                name = UFTconvert.from_bytes(((ScriptFlagBase*)f.flag.get())->luaScriptName);
                            else
                                name = UFTconvert.from_bytes(f->as_string());

                            break;
                        }
                }
                return name;
            }, VulpineColorUI::HightlightColorCyan, false, 0.25),
        })
    );



    // Add the new entity spawn menu
    VulpineBlueprintUI::AddToSelectionMenu(
        GlobalInfosTitleTab, GlobalInfosSubTab,

        newEntity("Entity Placing Menu", UI_BASE_COMP,
            WidgetStyle().setautomaticTabbing(2),
            EntityGroupInfo({gizmo.createMenu(),
                
                newEntity("World Editor Misc Menu", UI_BASE_COMP,
                    WidgetStyle().setautomaticTabbing(-2).setuseInternalSpacing(true),
                    EntityGroupInfo({
                        VulpineBlueprintUI::NamedEntry(U"Miscellaneous", selectedEntityMenu, 0.2, true, VulpineColorUI::LightBackgroundColor1),
                        VulpineBlueprintUI::NamedEntry(U"Spawn Condition", selectedEntityConditionMenu, 0.2, true, VulpineColorUI::LightBackgroundColor1)
                    })
                )
            
            })
        ),

        "Entity Edition", ""
    );

    // Automaticcly open the newly created menu
    GlobalInfosTitleTab->comp<EntityGroupInfo>().children.back()->comp<WidgetState>().statusToPropagate = ModelStatus::SHOW;


    EntityRef TerrainEditorMenu = TerrainEditor::getMenu();

    TerrainEditorMenu->set(
        WidgetBox([](Entity *p, Entity *e)
        {
            TerrainEditor::active = e->comp<WidgetState>().statusToPropagate != ModelStatus::HIDE and !StreetView::active;
        })
    );

    VulpineBlueprintUI::AddToSelectionMenu(
        GlobalInfosTitleTab, GlobalInfosSubTab,
        TerrainEditorMenu,
        "Terrain Edition", ""
    );

    return newEntity("WorldEditor - APP MENU"
        , UI_BASE_COMP
        , WidgetStyle().setautomaticTabbing(2).setuseInternalSpacing(true)
        , EntityGroupInfo(
            {
                WorldEntities::Menu::parent,
                EntitySpawn::Menu::parent
            })
    );
}

EntityRef Apps::WorldEditorApp::UIcontrols()
{
    /*
        -- Camera Control --
        Follow Terrain Height
        Type : Orthogonal, Orbit, Spectator
        ...
    */
    
    
    return newEntity("World Editor - APP CONTROL",
        UI_BASE_COMP,
        WidgetStyle().setautomaticTabbing(-6).setuseInternalSpacing(true),
        EntityGroupInfo({

            VulpineBlueprintUI::ColoredConstEntry(
                "Unsaved Changes",[](){return UFTconvert.from_bytes(std::to_string(unsavedChanges));}, 
                VulpineColorUI::LightBackgroundColor1, false, 0.75
            ),

            VulpineBlueprintUI::Toggable("Save World", "", 
                [](Entity *e, float f){WorldEntities::saveWorld();},
                [](Entity *e){e->comp<WidgetStyle>().backgroundColor1 = unsavedChanges ? VulpineColorUI::HightlightColorOrange : VulpineColorUI::HightlightColorGreen;   return 0.f;}
            ),

            VulpineBlueprintUI::Toggable("Terrain Camera", "", 
                [&](Entity *e, float f){clearTopDownView(); orbitController.enableTerrainFollow = true;},
                [&](Entity *e){return orbitController.enableTerrainFollow and !orbitController.enable2DView ? 0.f : 1.f;}, 
                VulpineColorUI::HightlightColorPurple
            ),
            VulpineBlueprintUI::Toggable("Free Camera", "", 
                [&](Entity *e, float f){clearTopDownView(); orbitController.enableTerrainFollow = false;},
                [&](Entity *e){return !orbitController.enableTerrainFollow and !orbitController.enable2DView ? 0.f : 1.f;},
                VulpineColorUI::HightlightColorPurple
            ),
            VulpineBlueprintUI::Toggable("Top Down Camera", "", 
                [&](Entity *e, float f){setTopDownView();},
                [&](Entity *e){return orbitController.enable2DView ? 0.f : 1.f;},
                VulpineColorUI::HightlightColorPurple
            ),
            VulpineBlueprintUI::Toggable2("Position Helper", "", 
                [&](Entity *e, float f){showPositionHelper = !showPositionHelper;},
                [&](Entity *e){return showPositionHelper ? 0.f : 1.f;},
                VulpineColorUI::HightlightColorPurple
            )
        })
    );
}

Apps::WorldEditorApp::WorldEditorApp() : SubApps("WorldEditor")
{
    
    static InputFilter WorldEditorOnly = [](){return !TerrainEditor::active and !StreetView::active;};
    static InputFilter GizmoOnly = [](){return !TerrainEditor::active and gizmo.isEnabled() and !StreetView::active;};
    static InputFilter TerrainEditorOnly = [](){return TerrainEditor::active and !StreetView::active;};

    inputs.push_back(&
        InputManager::addEventInput(
            "GIZMO : Translate Mode", GLFW_KEY_T, 0, GLFW_PRESS, [&]() {
                gizmo.translationMode();
            },
            GizmoOnly, false)
    );    

    inputs.push_back(&
        InputManager::addEventInput(
            "GIZMO : Rotate Mode", GLFW_KEY_R, 0, GLFW_PRESS, [&]() {
                gizmo.rotationMode();
            },
            GizmoOnly, false)
    );    

    inputs.push_back(&
        InputManager::addEventInput(
            "GIZMO : Scale Mode", GLFW_KEY_E, 0, GLFW_PRESS, [&]() {
                gizmo.scalingMode();
            },
            GizmoOnly, false)
    ); 

    inputs.push_back(&
        InputManager::addEventInput(
            "GIZMO : Undo", GLFW_KEY_W, GLFW_MOD_CONTROL, GLFW_PRESS, [&]() {
                gizmo.undo();
            },
            GizmoOnly, false)
    ); 

    inputs.push_back(&
        InputManager::addEventInput(
            "GIZMO : Redo", GLFW_KEY_Y, GLFW_MOD_CONTROL, GLFW_PRESS, [&]() {
                gizmo.redo();
            },
            GizmoOnly, false)
    ); 

    inputs.push_back(&
        InputManager::addEventInput(
            "GIZMO : Switch between world and relative translation mode", GLFW_KEY_Z, 0, GLFW_PRESS, [&]() {
                gizmo.translateModeSwitch();
            },
            GizmoOnly, false)
    ); 

    inputs.push_back(&
        InputManager::addEventInput(
            "GIZMO : Toggle terrain height follow", GLFW_KEY_X, 0, GLFW_PRESS, [&]() {
                gizmo.toggleFollowTerrain();
            },
            GizmoOnly, false)
    ); 

    inputs.push_back(&
        InputManager::addEventInput(
            "Save World", GLFW_KEY_S, GLFW_MOD_CONTROL, GLFW_PRESS, [&]() {
                WorldEntities::saveEntity(WorldEntities::world);
            },
            WorldEditorOnly, false)
    ); 

    inputs.push_back(&
        InputManager::addContinuousInput(
            "Brought Snapping", GLFW_KEY_LEFT_SHIFT, 
            [&](){gizmo.enableBroughtSnapping();},
            GizmoOnly,
            [&](){gizmo.disableBroughtSnapping();}
        
        )
    ); 

    inputs.push_back(&
        InputManager::addEventInput(
            "Toggle Grid Snapping", GLFW_KEY_G, 0, GLFW_PRESS, [&]() {
                if(gizmo.isSnapingEnable())
                    gizmo.disableSnapping();
                else
                    gizmo.enableSnapping();
            },
            GizmoOnly, false)
    ); 

    inputs.push_back(&
        InputManager::addEventInput(
            "Toggle Camera Terrain Follow", GLFW_KEY_Q, 0, GLFW_PRESS, [&]() {
                orbitController.enableTerrainFollow = !orbitController.enableTerrainFollow;
            },
            InputManager::Filters::always, false)
    ); 

    inputs.push_back(&
        InputManager::addEventInput(
            "Center Camera On Selected File/Folder", GLFW_KEY_SPACE, 0, GLFW_PRESS, [&]() {
                if(WorldEntities::selectedFile)
                    orbitController.targetPosition = WorldEntities::selectedFile->comp<State3D>().position;
            },
            WorldEditorOnly, false)
    ); 

    inputs.push_back(&
        InputManager::addEventInput(
            "Toggle Top Down View", GLFW_KEY_KP_7, 0, GLFW_PRESS, [&]() {
                if(orbitController.enable2DView)
                    clearTopDownView();
                else
                    setTopDownView();
            },
            InputManager::Filters::always, false)
    ); 

    // inputs.push_back(&
    //     InputManager::addEventInput(
    //         "DEBUG FOLDER ADD", GLFW_KEY_KP_ADD, 0, GLFW_PRESS, [&]() {
    //             static int tmpcont = 0;
    //             WorldEntities::addFolder(WorldEntities::selectedFolder->comp<EntityInfos>().name + FOLDER_STR + "New Folder " + std::to_string(tmpcont++));
    //             // WorldEntities::addFolder("New Folder " + std::to_string(tmpcont++));
    //         },
    //         InputManager::Filters::always, false)
    // ); 

    inputs.push_back(&
        InputManager::addEventInput(
            "Toggle Terrain Texture View", GLFW_KEY_B, 0, GLFW_PRESS, [&]() {
                TerrainEditor::textureView = !TerrainEditor::textureView;
            },
            TerrainEditorOnly, false)
    ); 

    inputs.push_back(&
        InputManager::addContinuousInput(
            "Scroll Tool", GLFW_KEY_TAB, 
            [&]()
            {
                if(TerrainEditorOnly())
                {
                    globals.currentCamera->setMouseFollow(false);
                    vec2 off = globals.mouseScrollOffset();
                    globals.clearMouseScroll();
                    
                    TerrainEditor::brush::tool += -sign(off.y) + TerrainEditor::brush::BrushToolMap.size();
                    TerrainEditor::brush::tool %= TerrainEditor::brush::BrushToolMap.size();
    
                    TerrainEditor::currentToolHelper->comp<WidgetState>().status = ModelStatus::SHOW;
                }
            },
            InputManager::Filters::always,
            []()
            {
                TerrainEditor::currentToolHelper->comp<WidgetState>().status = ModelStatus::HIDE;
            }
        )
    ); 

    inputs.push_back(&
        InputManager::addContinuousInput(
            "Scroll Brush Force", GLFW_KEY_F, 
            [&]()
            {
                globals.currentCamera->setMouseFollow(false);
                vec2 off = globals.mouseScrollOffset();
                globals.clearMouseScroll();
                
                TerrainEditor::brush::forceIntensity[TerrainEditor::brush::tool].x = 
                    clamp(TerrainEditor::brush::forceIntensity[TerrainEditor::brush::tool].x + sign(off.y)*0.25f, 0.f, 10.f);
            },
            TerrainEditorOnly
        )
    ); 

    inputs.push_back(&
        InputManager::addContinuousInput(
            "Scroll Brush Intensity", GLFW_KEY_E, 
            [&]()
            {
                globals.currentCamera->setMouseFollow(false);
                vec2 off = globals.mouseScrollOffset();
                globals.clearMouseScroll();
                
                float &i = TerrainEditor::brush::forceIntensity[TerrainEditor::brush::tool].y;

                if(TerrainEditor::viableTargets[2] == TerrainEditor::currentTarget)
                {
                    i = fract(i + sign(off.y)*0.025);
                }
                else
                {
                    i = clamp(i + sign(off.y)*0.1f, 0.f, 1.f);
                }

            },
            TerrainEditorOnly
        )
    ); 

    inputs.push_back(&
        InputManager::addContinuousInput(
            "Drag Brush Scale", GLFW_MOUSE_BUTTON_RIGHT, 
            [&]()
            {                
                TerrainEditor::brush::range = distance(
                    vec2(mouseDragCommandLastPos3D.x, mouseDragCommandLastPos3D.z), 
                    vec2(TerrainEditor::brush::pos3D.x, TerrainEditor::brush::pos3D.z)
                );
                
                TerrainEditor::brush::range = clamp(TerrainEditor::brush::range, 1.f, 1024.f);
            },
            TerrainEditorOnly,
            [&]()
            {
                mouseDragCommandLastPos = globals.mousePosition();
                mouseDragCommandLastPos3D = TerrainEditor::brush::pos3D;
            }
        )
    ); 

    static int brushToolTmp = 0;
    static bool smoothFastSwitchNeedUpdate = false;

    inputs.push_back(&
        InputManager::addContinuousInput(
            "Smooth Brush", GLFW_KEY_LEFT_CONTROL, 
            [&]()
            {                
                TerrainEditor::brush::tool = TerrainEditor::brush::BrushTool::Smooth;
                smoothFastSwitchNeedUpdate = true;
            },
            TerrainEditorOnly,
            [&]()
            {
                if(smoothFastSwitchNeedUpdate)
                {
                    TerrainEditor::brush::tool = brushToolTmp;
                    smoothFastSwitchNeedUpdate = false;
                }
                brushToolTmp = TerrainEditor::brush::tool;
            }
        )
    ); 

    static int brushToolTmp2 = 0;
    static bool inverseFastSwitchNeedUpdate = false;

    inputs.push_back(&
        InputManager::addContinuousInput(
            "Complemental Tool", GLFW_KEY_LEFT_ALT, 
            [&]()
            {                
                switch(brushToolTmp2)
                {
                    case TerrainEditor::brush::BrushTool::Add : 
                        TerrainEditor::brush::tool = TerrainEditor::brush::BrushTool::Substract;
                    break;

                    case TerrainEditor::brush::BrushTool::Substract : 
                        TerrainEditor::brush::tool = TerrainEditor::brush::BrushTool::Add;
                    break;

                    case TerrainEditor::brush::BrushTool::Slope : 
                        TerrainEditor::brush::tool = TerrainEditor::brush::BrushTool::Flatten;
                    break;

                    case TerrainEditor::brush::BrushTool::Flatten : 
                        TerrainEditor::brush::tool = TerrainEditor::brush::BrushTool::Slope;
                    break;
                    
                    case TerrainEditor::brush::BrushTool::Erosion : 
                        TerrainEditor::brush::tool = TerrainEditor::brush::BrushTool::Noise;
                    break;

                    case TerrainEditor::brush::BrushTool::Noise : 
                        TerrainEditor::brush::tool = TerrainEditor::brush::BrushTool::Smooth;
                    break;
                
                    case TerrainEditor::brush::BrushTool::Smooth : 
                        TerrainEditor::brush::tool = TerrainEditor::brush::BrushTool::Noise;
                    break;
                }

                inverseFastSwitchNeedUpdate = true;
            },
            TerrainEditorOnly,
            [&]()
            {
                if(inverseFastSwitchNeedUpdate)
                {
                    TerrainEditor::brush::tool = brushToolTmp2;
                    inverseFastSwitchNeedUpdate = false;
                }
                brushToolTmp2 = TerrainEditor::brush::tool;
            }
        )
    ); 

    inputs.push_back(&
        InputManager::addEventInput(
            "Terrain Editor : Undo", GLFW_KEY_W, GLFW_MOD_CONTROL | GLFW_MOD_SHIFT, GLFW_PRESS, [&]() {
                TerrainEditor::historic::undo();
            },
            TerrainEditorOnly, false)
    ); 

    inputs.push_back(&
        InputManager::addEventInput(
            "Terrain Editor : Redo", GLFW_KEY_Y, GLFW_MOD_CONTROL | GLFW_MOD_SHIFT, GLFW_PRESS, [&]() {
                TerrainEditor::historic::redo();
            },
            TerrainEditorOnly, false)
    ); 

    inputs.push_back(&
        InputManager::addEventInput(
            "Terrain Editor : Save", GLFW_KEY_S, GLFW_MOD_CONTROL | GLFW_MOD_SHIFT, GLFW_PRESS, [&]() {
                TerrainEditor::save::toDisk();
            },
            TerrainEditorOnly, false)
    ); 

    inputs.push_back(&
        InputManager::addEventInput(
            "Toggle 'Street View'", GLFW_MOUSE_BUTTON_LEFT, 0, GLFW_RELEASE, [&]() {
                if(StreetView::active and !StreetView::inStreetView)
                    StreetView::enable();
            },
            InputManager::Filters::always, false)
    ); 

    inputs.push_back(&
        InputManager::addEventInput(
            "Toggle 'Street View'", GLFW_KEY_ESCAPE, 0, GLFW_PRESS, [&]() {
                if(StreetView::inStreetView)
                    StreetView::disable();
            },
            InputManager::Filters::always, false)
    ); 

    for(auto &i : inputs)
        i->activated = false;
};

void Apps::WorldEditorApp::init()
{
    unsavedChanges = 0;
    EntitySpawner::loadAllAndHide = true;
    controllerPTR = &orbitController;
    /***** Preparing App Settings *****/
    {
        appRoot = newEntity("AppRoot", UI_BASE_COMP);
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

    ComponentModularity::addChild(*appRoot, TerrainEditor::terrain = Blueprint::SpawnMainGameTerrain());

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
    TerrainEditor::init(appRoot);
    StreetView::init();
    Biome::init();
    WaterGenerator::generate();

    GrassGenerator::active = true;
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

    if(!StreetView::active)
    {
        if(cursor.x < 0 || cursor.y < 0 || cursor.x > 1 || cursor.y > 1)
            globals.currentCamera->setMouseFollow(false);
        else
            globals.currentCamera->setMouseFollow(true);
    }

    gizmo.update(cursor);
    StreetView::update(cursor);
    Biome::update();

    if(showPositionHelper and !StreetView::active)
    {
        float size = max(1.0, orbitController.distance*0.05);
        vec3 p = orbitController.position + vec3(0, size*0.125, 0);
        GG::draw->drawSphere(p, size*0.125, 0, ModelState3D(), VulpineColorUI::LightBackgroundColor1);

        float xh1 = getTerrainHeight(vec2(p.z, p.x - size));
        float xh2 = getTerrainHeight(vec2(p.z, p.x + size));
        float zh1 = getTerrainHeight(vec2(p.z - size, p.x));
        float zh2 = getTerrainHeight(vec2(p.z + size, p.x));

        float th = getTerrainHeight(vec2(p.z, p.x));
        vec3 tc = th > p.y ? VulpineColorUI::HightlightColorRed : VulpineColorUI::HightlightColorGreen;

        float a = smoothstep(1.5f, 0.0f, distance(p.y, th));

        vec3 xp1 = p-vec3(size, 0, 0); xp1.y = mix(xp1.y, xh1, a);
        vec3 xp2 = p+vec3(size, 0, 0); xp2.y = mix(xp2.y, xh2, a);

        vec3 zp1 = p-vec3(0, 0, size); zp1.y = mix(zp1.y, zh1, a);
        vec3 zp2 = p+vec3(0, 0, size); zp2.y = mix(zp2.y, zh2, a);


        GG::draw->drawLine(xp1, p, 0, ModelState3D(), VulpineColorUI::LightBackgroundColor1);
        GG::draw->drawLine(xp2, p, 0, ModelState3D(), VulpineColorUI::LightBackgroundColor1);
        GG::draw->drawLine(zp1, p, 0, ModelState3D(), VulpineColorUI::LightBackgroundColor1);
        GG::draw->drawLine(zp2, p, 0, ModelState3D(), VulpineColorUI::LightBackgroundColor1);

        GG::draw->drawLine(p, p*vec3(1, 0, 1)+vec3(0, th, 0), 0, ModelState3D(), tc*0.5f);
    }

    glLineWidth(1);

    
    if(TerrainEditor::active and !orbitController.enable2DView and !StreetView::active)
    {
        TerrainEditor::update(cursor);
    }

    if(WorldEntities::selectedFile and !TerrainEditor::active and !StreetView::active)
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

        /*
            We only count the second update of the selected file as a unsaved change
            because the first one can be triggered by floating point imprecision
        */
        if(h != inv and ++selectedFileUpdateCounter == 2)
            unsavedChanges++;
        
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
            aabbmin, aabbmax, 0.0, ModelState3D(), VulpineColorUI::LightBackgroundColor1
        );
    }
    else
    {
        selectedFileUpdateCounter = 0;
        gizmo.disable();
    }
}

void Apps::WorldEditorApp::clean()
{
    EntitySpawner::loadAllAndHide = false;
    globals.simulationTime.pause();

    globals.currentCamera->setMouseFollow(false);
    globals.currentCamera->setPosition(vec3(0));
    globals.currentCamera->setDirection(vec3(-1, 0, 0));

    ComponentModularity::removeChild(*GlobalInfosSubTab, GlobalInfosSubTab->comp<EntityGroupInfo>().children.back());
    ComponentModularity::removeChild(*GlobalInfosTitleTab, GlobalInfosTitleTab->comp<EntityGroupInfo>().children.back());

    EntitySpawn::clear();
    WorldEntities::clear();
    TerrainEditor::clear();
    StreetView::clear();
    Biome::clear();
    WaterGenerator::clear();

    EntitySpawn::Menu::parent = EntityRef();
    gizmo.clear();
    appRoot = EntityRef();
    App::setController(nullptr);

    GG::sun->shadowCameraSize = vec2(0, 0);
}

