#include <Subapps.hpp>
#include <EntityBlueprint.hpp>
#include <AssetManager.hpp>
#include <MathsUtils.hpp>

#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtx/string_cast.hpp>

#include <Inputs.hpp>
#include <Game.hpp>


void Apps::MaterialViewerApp::spawnHelper()
{
    int id = color.size();
    if(id >= 16)
        return;

    color.push_back(EDITOR::MENUS::COLOR::HightlightColor1);
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

    auto colorSelector = Blueprint::EDITOR_ENTITY::INO::ColorSelectionScreen(
        "Material Color", [&, id](){return color[id];}, [&, id](vec3 c){color[id] = c;}
    );

    auto metalnessSelector = Blueprint::EDITOR_ENTITY::INO::ValueInputSlider(
        "Metalness", 0, 1, 1, 
        [&, id](Entity *e, float v){SME[id].y = v;},
        [&, id](Entity *e){return SME[id].y;},
        [&, id](std::u32string t){SME[id].y = u32strtof2(t, SME[id].y);},
        [&, id](){return ftou32str(SME[id].y);}
    );

    auto smoothnessSelector = Blueprint::EDITOR_ENTITY::INO::ValueInputSlider(
        "Smoothness", 0, 1, 15, 
        [&, id](Entity *e, float v){SME[id].x = v;},
        [&, id](Entity *e){return SME[id].x;},
        [&, id](std::u32string t){SME[id].x = u32strtof2(t, SME[id].x);},
        [&, id](){return ftou32str(SME[id].x);}
    );

    auto emmisiveSelector = Blueprint::EDITOR_ENTITY::INO::ValueInputSlider(
        "Emmisive", 0, 1, 7, 
        [&, id](Entity *e, float v){SME[id].z = v;},
        [&, id](Entity *e){return SME[id].z;},
        [&, id](std::u32string t){SME[id].z = u32strtof2(t, SME[id].z);},
        [&, id](){return ftou32str(SME[id].z);}
    );

    auto papernessSelector = Blueprint::EDITOR_ENTITY::INO::ValueInputSlider(
        "paperness", 0, 1, 1, 
        [&, id](Entity *e, float v){PSBD[id].x = v;},
        [&, id](Entity *e){return PSBD[id].x;},
        [&, id](std::u32string t){PSBD[id].x = u32strtof2(t, PSBD[id].x);},
        [&, id](){return ftou32str(PSBD[id].x);}
    );

    auto streakingSelector = Blueprint::EDITOR_ENTITY::INO::ValueInputSlider(
        "streaking", 0, 1, 7, 
        [&, id](Entity *e, float v){PSBD[id].y = v;},
        [&, id](Entity *e){return PSBD[id].y;},
        [&, id](std::u32string t){PSBD[id].y = u32strtof2(t, PSBD[id].y);},
        [&, id](){return ftou32str(PSBD[id].y);}
    );

    auto bloodynessSelector = Blueprint::EDITOR_ENTITY::INO::ValueInputSlider(
        "bloodyness", 0, 1, 3, 
        [&, id](Entity *e, float v){PSBD[id].z = v;},
        [&, id](Entity *e){return PSBD[id].z;},
        [&, id](std::u32string t){PSBD[id].z = u32strtof2(t, PSBD[id].z);},
        [&, id](){return ftou32str(PSBD[id].z);}
    );

    auto dirtynessSelector = Blueprint::EDITOR_ENTITY::INO::ValueInputSlider(
        "dirtyness", 0, 1, 3, 
        [&, id](Entity *e, float v){PSBD[id].w = v;},
        [&, id](Entity *e){return PSBD[id].w;},
        [&, id](std::u32string t){PSBD[id].w = u32strtof2(t, PSBD[id].w);},
        [&, id](){return ftou32str(PSBD[id].w);}
    );

    auto bloodynessCurSelector = Blueprint::EDITOR_ENTITY::INO::ValueInputSlider(
        "bloodyness", 0, 1, 4, 
        [&, id](Entity *e, float v){BD[id].x = v;},
        [&, id](Entity *e){return BD[id].x;},
        [&, id](std::u32string t){BD[id].x = u32strtof2(t, BD[id].x);},
        [&, id](){return ftou32str(BD[id].x);}
    );

    auto dirtynessCurSelector = Blueprint::EDITOR_ENTITY::INO::ValueInputSlider(
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
                    Blueprint::EDITOR_ENTITY::INO::NamedEntry(U"Metalness", metalnessSelector,          0.5),
                    Blueprint::EDITOR_ENTITY::INO::NamedEntry(U"Smoothness", smoothnessSelector,        0.5), 
                    Blueprint::EDITOR_ENTITY::INO::NamedEntry(U"Emmisive", emmisiveSelector,            0.5), 
                    Blueprint::EDITOR_ENTITY::INO::NamedEntry(U"Paperness", papernessSelector,          0.5), 
                    Blueprint::EDITOR_ENTITY::INO::NamedEntry(U"Streaking", streakingSelector,          0.5), 
                    Blueprint::EDITOR_ENTITY::INO::NamedEntry(U"Blood Factor", bloodynessSelector,     0.5), 
                    Blueprint::EDITOR_ENTITY::INO::NamedEntry(U"Blood Level", bloodynessCurSelector,   0.5), 
                    Blueprint::EDITOR_ENTITY::INO::NamedEntry(U"Dirt Factor", dirtynessSelector,       0.5), 
                    Blueprint::EDITOR_ENTITY::INO::NamedEntry(U"Dirt Level", dirtynessCurSelector,     0.5), 
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
                            .setbackgroundColor1(EDITOR::MENUS::COLOR::LightBackgroundColor1)
                            .settextColor1(EDITOR::MENUS::COLOR::DarkBackgroundColor1)
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
        Blueprint::EDITOR_ENTITY::INO::AddToSelectionMenu(titleTab, menuInfosTab,
            newEntity("Helper Group Tab"
                , UI_BASE_COMP
                , WidgetBox()
                , WidgetStyle().setautomaticTabbing(4)
            )
            , 
            std::to_string(id+1) + " - " + std::to_string(id+4)
        );

        orbitController.distance = 12.f * pow((float)(1 + helpers.size()/4), 0.575);
    }

    ComponentModularity::addChild(*menuInfosTab->comp<EntityGroupInfo>().children[id/4], p);


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
        , WidgetBox(vec2(-1, 1), vec2(-1, -0.95))
        , WidgetBackground()
        , WidgetStyle()
            .setautomaticTabbing(1)
            .setbackgroundColor1(EDITOR::MENUS::COLOR::DarkBackgroundColor2)
            // .setbackgroundColor1(EDITOR::MENUS::COLOR::HightlightColor2)
            .setbackGroundStyle(UiTileType::SQUARE_ROUNDED)
    );

    menuInfosTab = newEntity("Material View Menu Infos Tab"
        , EDITOR::UIcontext
        // , UI_BASE_COMP
        // , WidgetState()
        , WidgetBox(vec2(-1, 1), vec2(-0.95, 1))
        // , WidgetBackground()
        , WidgetStyle()
            // .setautomaticTabbing(1)
            .setbackgroundColor1(EDITOR::MENUS::COLOR::DarkBackgroundColor1)
            // .setbackgroundColor1(EDITOR::MENUS::COLOR::HightlightColor1)
            .setbackGroundStyle(UiTileType::SQUARE_ROUNDED)
        
    );

    auto root = newEntity("MATERIAL VIEW APP MENU"
        // , UI_BASE_COMP
        , EDITOR::UIcontext
        , WidgetBox()
        , WidgetStyle()
            .settextColor1(EDITOR::MENUS::COLOR::HightlightColor1)
        // , WidgetText(U"...")
        // , WidgetBackground()
        , EntityGroupInfo({titleTab, menuInfosTab})
    );

    /***** Setting up material helpers *****/
    spawnHelper();
    spawnHelper();
    spawnHelper();
    spawnHelper();
    // spawnHelper();

    titleTab->comp<EntityGroupInfo>().children[0]->comp<WidgetState>().statusToPropagate = ModelStatus::SHOW;

    return root;
}

void Apps::MaterialViewerApp::init()
{
    /***** Preparing App Settings *****/
    {
        appRoot = newEntity();
        GG::currentConditions.readTxt("saves/gameConditions.txt");

        App::setController(&orbitController);

        globals.currentCamera->setPosition(vec3(-1, 0, 0));
        globals.currentCamera->getState().FOV = radians(40.f);
        orbitController.distance = 10;

        GG::sun->shadowCameraSize = vec2(64, 64);
    }
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
            2.0  * (mod(cnt, 4.f) - 1.5f), 
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

    GG::sun->shadowCameraSize = vec2(0, 0);

    globals.currentCamera->setMouseFollow(false);
    globals.currentCamera->setPosition(vec3(0));
    globals.currentCamera->setDirection(vec3(-1, 0, 0));
    globals.currentCamera->getState().FOV = radians(90.f);
    App::setController(nullptr);
}
