#include <Subapps.hpp>
#include <EntityBlueprint.hpp>
#include <AssetManager.hpp>
#include <MathsUtils.hpp>

#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtx/string_cast.hpp>

#include <Inputs.hpp>
#include <Game.hpp>

#include <filesystem>

struct MaterialViewInfo
{
    vec3 color;
    float metalness;
    float smoothness;
    float emmisive;
    float paperness;
    float streaking;
    float bloodyness;
    float bloodynessFactor;
    float dirtyness;
    float dirtynessFactor;
};

AUTOGEN_DATA_RW_FUNC(MaterialViewInfo
    , color
    , metalness
    , smoothness
    , emmisive
    , paperness
    , streaking
    , bloodyness
    , bloodynessFactor
    , dirtyness
    , dirtynessFactor
);

struct MaterialPalette : public std::vector<MaterialViewInfo>
{
    std::string name = "Unamed Palette 000";
};

DATA_READ_FUNC_INIT(MaterialPalette)
    IF_MEMBER_READ_VALUE(MaterialPalette)
        data.name = value;
    else 
    IF_MEMBER(MaterialViewInfo)
        data.push_back(DataLoader<MaterialViewInfo>::read(buff));
DATA_READ_END_FUNC

DATA_WRITE_FUNC_INIT(MaterialPalette)

    out->write("\"", 1);
    out->write(CONST_STRING_SIZED(data.name));
    out->write("\"", 1);

    for(auto i : data)
        DataLoader<MaterialViewInfo>::write(i, out);

DATA_WRITE_END_FUNC

void Apps::MaterialViewerApp::refreshPalettesList()
{
    // palettesList.clear();

    for (auto f : std::filesystem::recursive_directory_iterator("data/editor/palettes"))
    {
        if (f.is_directory())
            continue;

        char ext[1024];
        char p[4096];

        strcpy(ext, (char *)f.path().extension().string().c_str());
        strcpy(p, (char *)f.path().string().c_str());

        if (!strcmp(ext, ".MaterialPalette"))
        {
            auto name = getNameOnlyFromPath(p);

            if(palettesList.find(name) == palettesList.end())
                palettesList[name] = EntityRef();
        }
    } 
}

void Apps::MaterialViewerApp::spawnHelper()
{
    int id = color.size();
    if(id >= 16)
        return;

    color.push_back(VulpineColorUI::HightlightColor1);
    SME.push_back(vec3(0));
    PSBD.push_back(vec4(0));
    BD.push_back(vec2(0));

    ModelRef helper = Loader<MeshModel3D>::get("materialHelper").copy();
    helper->uniforms.add(ShaderUniform(&BD.back(), 32));
    helper->uniforms.add(ShaderUniform(&PSBD.back(), 33));
    helper->uniforms.add(ShaderUniform(&color.back(), 34));
    helper->uniforms.add(ShaderUniform(&SME.back(), 35));

    EntityModel model(EntityModel{newObjectGroup()}); 
    model->add(helper);
    ComponentModularity::addChild(*appRoot, newEntity("materialHelper", model));

    auto colorSelector = VulpineBlueprintUI::ColorSelectionScreen(
        "Material Color", [&, id](){return color[id];}, [&, id](vec3 c){color[id] = c;}
    );

    auto metalnessSelector = VulpineBlueprintUI::ValueInputSlider(
        "Metalness", 0, 1, 1, 
        [&, id](Entity *e, float v){SME[id].y = v;},
        [&, id](Entity *e){return SME[id].y;},
        [&, id](std::u32string t){SME[id].y = u32strtof2(t, SME[id].y);},
        [&, id](){return ftou32str(SME[id].y);}
    );

    auto smoothnessSelector = VulpineBlueprintUI::ValueInputSlider(
        "Smoothness", 0, 1, 15, 
        [&, id](Entity *e, float v){SME[id].x = v;},
        [&, id](Entity *e){return SME[id].x;},
        [&, id](std::u32string t){SME[id].x = u32strtof2(t, SME[id].x);},
        [&, id](){return ftou32str(SME[id].x);}
    );

    auto emmisiveSelector = VulpineBlueprintUI::ValueInputSlider(
        "Emmisive", 0, 1, 7, 
        [&, id](Entity *e, float v){SME[id].z = v;},
        [&, id](Entity *e){return SME[id].z;},
        [&, id](std::u32string t){SME[id].z = u32strtof2(t, SME[id].z);},
        [&, id](){return ftou32str(SME[id].z);}
    );

    auto papernessSelector = VulpineBlueprintUI::ValueInputSlider(
        "paperness", 0, 1, 1, 
        [&, id](Entity *e, float v){PSBD[id].x = v;},
        [&, id](Entity *e){return PSBD[id].x;},
        [&, id](std::u32string t){PSBD[id].x = u32strtof2(t, PSBD[id].x);},
        [&, id](){return ftou32str(PSBD[id].x);}
    );

    auto streakingSelector = VulpineBlueprintUI::ValueInputSlider(
        "streaking", 0, 1, 7, 
        [&, id](Entity *e, float v){PSBD[id].y = v;},
        [&, id](Entity *e){return PSBD[id].y;},
        [&, id](std::u32string t){PSBD[id].y = u32strtof2(t, PSBD[id].y);},
        [&, id](){return ftou32str(PSBD[id].y);}
    );

    auto bloodynessSelector = VulpineBlueprintUI::ValueInputSlider(
        "bloodyness", 0, 1, 3, 
        [&, id](Entity *e, float v){PSBD[id].z = v;},
        [&, id](Entity *e){return PSBD[id].z;},
        [&, id](std::u32string t){PSBD[id].z = u32strtof2(t, PSBD[id].z);},
        [&, id](){return ftou32str(PSBD[id].z);}
    );

    auto dirtynessSelector = VulpineBlueprintUI::ValueInputSlider(
        "dirtyness", 0, 1, 3, 
        [&, id](Entity *e, float v){PSBD[id].w = v;},
        [&, id](Entity *e){return PSBD[id].w;},
        [&, id](std::u32string t){PSBD[id].w = u32strtof2(t, PSBD[id].w);},
        [&, id](){return ftou32str(PSBD[id].w);}
    );

    auto bloodynessCurSelector = VulpineBlueprintUI::ValueInputSlider(
        "bloodyness", 0, 1, 4, 
        [&, id](Entity *e, float v){BD[id].x = v;},
        [&, id](Entity *e){return BD[id].x;},
        [&, id](std::u32string t){BD[id].x = u32strtof2(t, BD[id].x);},
        [&, id](){return ftou32str(BD[id].x);}
    );

    auto dirtynessCurSelector = VulpineBlueprintUI::ValueInputSlider(
        "dirtyness", 0, 1, 4, 
        [&, id](Entity *e, float v){BD[id].y = v;},
        [&, id](Entity *e){return BD[id].y;},
        [&, id](std::u32string t){BD[id].y = u32strtof2(t, BD[id].y);},
        [&, id](){return ftou32str(BD[id].y);}
    );

    auto finalMenu = newEntity("Material Helper Config Menu"
        , UI_BASE_COMP
        , WidgetBox(vec2(-1, 1), vec2(-0.85, 1))
        , WidgetStyle()
            .setautomaticTabbing(1)
        , EntityGroupInfo({
            colorSelector,
            newEntity("Material Misc Properties"
                , UI_BASE_COMP
                , WidgetBox()
                , WidgetStyle()
                    .setautomaticTabbing(9)
                , EntityGroupInfo({
                    VulpineBlueprintUI::NamedEntry(U"Metalness", metalnessSelector,          0.5),
                    VulpineBlueprintUI::NamedEntry(U"Smoothness", smoothnessSelector,        0.5), 
                    VulpineBlueprintUI::NamedEntry(U"Emmisive", emmisiveSelector,            0.5), 
                    VulpineBlueprintUI::NamedEntry(U"Paperness", papernessSelector,          0.5), 
                    VulpineBlueprintUI::NamedEntry(U"Streaking", streakingSelector,          0.5), 
                    VulpineBlueprintUI::NamedEntry(U"Blood Factor", bloodynessSelector,     0.5), 
                    VulpineBlueprintUI::NamedEntry(U"Blood Level", bloodynessCurSelector,   0.5), 
                    VulpineBlueprintUI::NamedEntry(U"Dirt Factor", dirtynessSelector,       0.5), 
                    VulpineBlueprintUI::NamedEntry(U"Dirt Level", dirtynessCurSelector,     0.5), 
                })
            )
        })
    );

    auto p = newEntity("Material Helper Config Menu + Title Parent"
        , UI_BASE_COMP
        , WidgetBox()
        , WidgetStyle()
            .setautomaticTabbing(1)
        , EntityGroupInfo({
            newEntity("Material Helper Config Menu"
                , UI_BASE_COMP
                , WidgetBox()
                , EntityGroupInfo({
                    newEntity("Material Helper Config Menu Title"
                        , UI_BASE_COMP
                        , WidgetBox(vec2(-1, 1), vec2(-1, -0.85))
                        , WidgetBackground()
                        , WidgetStyle()
                            .setbackgroundColor1(VulpineColorUI::LightBackgroundColor1)
                            .settextColor1(VulpineColorUI::DarkBackgroundColor1)
                            .setbackGroundStyle(UiTileType::SQUARE_ROUNDED)
                        , WidgetText(U"Material " + UFTconvert.from_bytes(std::to_string(id+1)))
                    ),
                    finalMenu
                })
            )
        })
    );

    if(id%4 == 0)
    {
        VulpineBlueprintUI::AddToSelectionMenu(titleTab, menuInfosTab,
            newEntity("Helper Group Tab"
                , UI_BASE_COMP
                , WidgetBox()
                , WidgetStyle().setautomaticTabbing(4)
            )
            , 
            "Material " + std::to_string(id+1) + " - " + std::to_string(id+4)
        );

        orbitController.distance = 12.f * pow((float)(1 + helpers.size()/4), 0.575);
    }

    ComponentModularity::addChild(*menuInfosTab->comp<EntityGroupInfo>().children[1 + id/4], p);


    helpers.push_back(helper);
}

Apps::MaterialViewerApp::MaterialViewerApp() : SubApps("Material Viewer")
{
    inputs.push_back(&
        InputManager::addEventInput(
            "Add Material Helper", GLFW_KEY_KP_ADD, 0, GLFW_PRESS, [&]() {
                spawnHelper();
                update();
            },
            InputManager::Filters::always, false)
    );

    for(auto &i : inputs)
        i->activated = false;
}

EntityRef Apps::MaterialViewerApp::UImenu()
{
    color.reserve(16);
    SME.reserve(16);
    PSBD.reserve(16);
    BD.reserve(16);
    helpers.reserve(16);

    titleTab = newEntity("Material View Title Tab"
        , UI_BASE_COMP
        , WidgetBox(vec2(-1, 1), vec2(-1, -0.94))
        , WidgetBackground()
        , WidgetStyle()
            .setautomaticTabbing(1)
            .setbackgroundColor1(VulpineColorUI::DarkBackgroundColor2)
            // .setbackgroundColor1(VulpineColorUI::HightlightColor2)
            .setbackGroundStyle(UiTileType::SQUARE_ROUNDED)
    );

    menuInfosTab = newEntity("Material View Menu Infos Tab"
        , VulpineBlueprintUI::UIcontext
        // , UI_BASE_COMP
        // , WidgetState()
        , WidgetBox(vec2(-1, 1), vec2(-0.94, 1))
        // , WidgetBackground()
        , WidgetStyle()
            // .setautomaticTabbing(1)
            .setbackgroundColor1(VulpineColorUI::DarkBackgroundColor1)
            // .setbackgroundColor1(VulpineColorUI::HightlightColor1)
            .setbackGroundStyle(UiTileType::SQUARE_ROUNDED)
        
    );

    refreshPalettesList();

    VulpineBlueprintUI::AddToSelectionMenu(titleTab, menuInfosTab,
        VulpineBlueprintUI::StringListSelectionMenu(
            "Palettes List", palettesList,
            [&](Entity *e, float v)
            {
                // TODO : debug
                color.clear();
                SME.clear();
                PSBD.clear();
                BD.clear();
                helpers.clear();

                MaterialPalette palette;

                VulpineTextBuffRef in(new VulpineTextBuff(
                    ("data/editor/palettes/" + e->comp<EntityInfos>().name + ".MaterialPalette").c_str()
                    )
                );
                palette = DataLoader<MaterialPalette>::read(in);

                color.reserve(16);
                SME.reserve(16);
                PSBD.reserve(16);
                BD.reserve(16);
                helpers.reserve(16);


                titleTab->comp<EntityGroupInfo>().children = 
                {
                   titleTab->comp<EntityGroupInfo>().children[0] 
                };

                menuInfosTab->comp<EntityGroupInfo>().children = 
                {
                   menuInfosTab->comp<EntityGroupInfo>().children[0] 
                };

                appRoot->comp<EntityGroupInfo>().children.clear();

                GG::ManageEntityGarbage();

                currentPalette =  e->comp<EntityInfos>().name;

                for(auto &i : palette)
                {
                    spawnHelper();

                    color.back() = i.color;
                    SME.back() = vec3(
                        i.smoothness, i.metalness, i.emmisive
                    );

                    PSBD.back() = vec4(
                        i.paperness, i.streaking,
                        i.bloodynessFactor, i.dirtynessFactor
                    );

                    BD.back() = vec2(
                        i.bloodyness, i.dirtyness
                    );
                }       

            },
            [&](Entity *e)
            {
                return currentPalette == e->comp<EntityInfos>().name ? 0.f : 1.f;
            }, 
            0.3
        )
        , "Palettes List"
    );

    /***** Setting up material helpers *****/
    spawnHelper();
    spawnHelper();
    spawnHelper();
    spawnHelper();
    // spawnHelper();

    titleTab->comp<EntityGroupInfo>().children[0]->comp<WidgetState>().statusToPropagate = ModelStatus::SHOW;

    auto root = newEntity("MATERIAL VIEW APP MENU"
        // , UI_BASE_COMP
        , VulpineBlueprintUI::UIcontext
        , WidgetBox()
        , WidgetStyle()
            .settextColor1(VulpineColorUI::HightlightColor1)
        // , WidgetText(U"...")
        // , WidgetBackground()
        , EntityGroupInfo({titleTab, menuInfosTab})
    );

    return root;
}


EntityRef Apps::MaterialViewerApp::UIcontrols()
{
    auto paletteNameEntry = VulpineBlueprintUI::TextInput(
        "Palette Name",
        [&](std::u32string &t){this->currentPalette = UFTconvert.to_bytes(t);},
        [&](){return UFTconvert.from_bytes(this->currentPalette);}
    );

    auto savePalette = VulpineBlueprintUI::Toggable("Save Palette", "", 
        [&](Entity *e, float v)
        {
            MaterialPalette palette;

            for(int i = 0; i < color.size(); i++)
            {
                MaterialViewInfo info;
                info.color = color[i];
                info.smoothness = SME[i].x;
                info.metalness  = SME[i].y;
                info.emmisive   = SME[i].z;
                info.paperness  = PSBD[i].x;
                info.streaking  = PSBD[i].y;
                info.bloodynessFactor = PSBD[i].z;
                info.dirtynessFactor  = PSBD[i].w;
                info.bloodyness = BD[i].x;
                info.dirtyness  = BD[i].y;

                palette.push_back(info);
            }

            palette.name = currentPalette;

            VulpineTextOutputRef out(new VulpineTextOutput(1 << 16));
            DataLoader<MaterialPalette>::write(palette, out);
            out->saveAs(("data/editor/palettes/" + currentPalette + ".MaterialPalette").c_str());
            refreshPalettesList();
        },
        [&](Entity *e)
        {
            return 0.f;
        }
    );

    auto addMaterial = VulpineBlueprintUI::Toggable("Add Material", "", 
        [&](Entity *e, float v)
        {
            spawnHelper();
        },
        [&](Entity *e)
        {
            return 0.f;
        }
    );

    return newEntity("Material Viwer Controls"
        , UI_BASE_COMP
        , WidgetBox()
        , WidgetStyle()
            .setautomaticTabbing(1)
        , EntityGroupInfo({
            VulpineBlueprintUI::NamedEntry(U"Palette Name", paletteNameEntry),
            savePalette,
            addMaterial
        })
    );
}

void Apps::MaterialViewerApp::init()
{
    /***** Preparing App Settings *****/
    {
        appRoot = newEntity("empty menu space");
        GG::currentConditions.readTxt("saves/gameConditions.txt");

        App::setController(&orbitController);

        globals.currentCamera->setPosition(vec3(-1, 0, 0));
        globals.currentCamera->getState().FOV = radians(40.f);
        orbitController.distance = 10;

        GG::sun->shadowCameraSize = vec2(64, 64);
    }

    // MaterialPalette palette;

    // VulpineTextBuffRef in(new VulpineTextBuff("data/editor/palettes/test.MaterialPalette"));
    // palette = DataLoader<MaterialPalette>::read(in);

    // // palette.push_back({vec3(0, 0, 0), 0, 0, 0, 0, 0, 0, 0});
    // // palette.push_back({vec3(1, 1, 1), 1, 2, 3, 4, 5, 6, 7});


    // VulpineTextOutputRef test(new VulpineTextOutput());
    // DataLoader<MaterialPalette>::write(palette, test);
    // test->saveAs("data/editor/palettes/test2.MaterialPalette");
}

void Apps::MaterialViewerApp::update()
{
    float cnt = 0;
    float size = helpers.size()-1;
    for(auto h : helpers)
    {   
        vec3 scale = h->getVao()->getAABBMax() - h->getVao()->getAABBMin();

        h->state.setPosition(vec3(
            0, 
            2.0  * (mod(size - cnt, 4.f) - 1.5f), 
            2.0 * (cnt - mod(cnt, 4.f) - 0.5f*(size - mod(size, 4.f)))
            ));
        h->state.update();
        cnt ++;
    }

    vec2 screenPos = globals.mousePosition();
    screenPos = (screenPos/vec2(globals.windowSize()))*2.f - 1.f;

    auto &box = EDITOR::MENUS::GameScreen->comp<WidgetBox>();
    vec2 cursor = ((screenPos-box.min)/(box.max - box.min));

    if(cursor.x < 0 || cursor.y < 0 || cursor.x > 1 || cursor.y > 1)
        globals.currentCamera->setMouseFollow(false);
    else
        globals.currentCamera->setMouseFollow(true);
}

void Apps::MaterialViewerApp::clean()
{
    appRoot 
    = menuInfosTab
    = titleTab
    = EntityRef();

    color.clear();
    SME.clear();
    PSBD.clear();
    BD.clear();
    helpers.clear();

    palettesList.clear();

    GG::sun->shadowCameraSize = vec2(0, 0);

    globals.currentCamera->setMouseFollow(false);
    globals.currentCamera->setPosition(vec3(0));
    globals.currentCamera->setDirection(vec3(-1, 0, 0));
    globals.currentCamera->getState().FOV = radians(90.f);
    App::setController(nullptr);
}
