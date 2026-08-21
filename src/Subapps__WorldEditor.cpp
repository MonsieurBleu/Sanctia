#include <App.hpp>
#include <Subapps.hpp>
#include <EntityBlueprint.hpp>
#include <Scripting/ScriptInstance.hpp>
#include <AssetManager.hpp>

#include <JoltIntegration/PhysicsDebugRenderer.hpp>

EntityRef gizmo;
EntityRef gizmoPos;
EntityRef gizmoRot;
EntityRef gizmoScale;

uint gizmoHistoricMaxSize = 1e4;
std::list<State3D> gizmoHistoric;
std::list<State3D>::iterator gizmoHistoricCurrent;

Apps::WorldEditorApp::WorldEditorApp() : SubApps("WorldEditor")
{
    inputs.push_back(&
        InputManager::addEventInput(
            "input exemple", GLFW_KEY_SPACE, 0, GLFW_PRESS, [&]() {
                
                NOTIF_MESSAGE("SPACE BAR PRESSED")

            },
            InputManager::Filters::always, false)
    );    

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
            "GIZMO : Scale Mode", GLFW_KEY_S, 0, GLFW_PRESS, [&]() {
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

    WARNING_MESSAGE(
        PRINTVAR(GLFW_KEY_Z),
        PRINTVAR(GLFW_KEY_W),
        PRINTVAR(glfwGetKeyScancode(GLFW_KEY_Z)),
        PRINTVAR(glfwGetKeyName(GLFW_KEY_Z, 0)),
        PRINTVAR(glfwGetKeyName(-1, glfwGetKeyScancode(GLFW_KEY_Z))),
        PRINTVAR(glfwGetKeyScancode(GLFW_KEY_W)),
        PRINTVAR(glfwGetKeyName(GLFW_KEY_W, 0)),
        PRINTVAR(glfwGetKeyName(-1, glfwGetKeyScancode(GLFW_KEY_W)))
    )

    for(auto &i : inputs)
        i->activated = false;
};

EntityRef Apps::WorldEditorApp::UImenu()
{
    return newEntity("WorldEditor APP MENU"
        , UI_BASE_COMP
        , WidgetBox()
    );
}

void Apps::WorldEditorApp::init()
{
    /***** Preparing App Settings *****/
    {
        appRoot = newEntity("AppRoot");
        App::setController(&orbitController);
    }

    gizmoPos = newEntity("Gizmo Pos", State3D(), KynematicFlag(), HierarchyState3D(HierarchyState3D::ALL));
    gizmoRot = newEntity("Gizmo Rot", State3D(), KynematicFlag(), HierarchyState3D(HierarchyState3D::SCALE_AND_POS));
    gizmoScale = newEntity("Gizmo Scale", State3D(), KynematicFlag(), HierarchyState3D(HierarchyState3D::POS_AND_ROT));
    gizmoPos->comp<HierarchyState3D>().isActive = ModelStatus::SHOW;
    gizmoRot->comp<HierarchyState3D>().isActive = ModelStatus::HIDE;
    gizmoScale->comp<HierarchyState3D>().isActive = ModelStatus::HIDE;
    gizmo = newEntity("Gizmo", State3D(), KynematicFlag(), HierarchyState3D(), EntityGroupInfo({gizmoPos, gizmoRot, gizmoScale}));

    #define ADD_GIZMO_ELEM(parent, name, pos, rot, color) { \
        EntityRef e = spawnEntity(name, pos, rot); \
        auto mesh = e->comp<EntityModel>()->getChildren()[0]->getMeshes()[0]; \
        mesh->uniforms.add(ShaderUniform(vec3(color), 20)); \
        mesh->depthWrite = false; \
        mesh->sorted = false; \
        ComponentModularity::addChild(*parent, e); \
        e->set<HierarchyState3D>(HierarchyState3D()); \
    }

    ADD_GIZMO_ELEM(gizmoPos, "Gizmo Arrow", vec3(0), quat(radians(vec3(0., 0., 0.))), VulpineColorUI::HightlightColor1);
    ADD_GIZMO_ELEM(gizmoPos, "Gizmo Arrow", vec3(0), quat(radians(vec3(0., -90., 0.))), VulpineColorUI::HightlightColor2);
    ADD_GIZMO_ELEM(gizmoPos, "Gizmo Arrow", vec3(0), quat(radians(vec3(0., 0., 90.))), VulpineColorUI::HightlightColor3);

    ADD_GIZMO_ELEM(gizmoPos, "Gizmo Plane", vec3(0, 0.45, 0.45), quat(radians(vec3(0., 0., 0.))), VulpineColorUI::HightlightColor1);
    ADD_GIZMO_ELEM(gizmoPos, "Gizmo Plane", vec3(0.45, 0.45, 0), quat(radians(vec3(0., -90., 0.))), VulpineColorUI::HightlightColor2);
    ADD_GIZMO_ELEM(gizmoPos, "Gizmo Plane", vec3(0.45, 0, 0.45), quat(radians(vec3(0., 0., 90.))), VulpineColorUI::HightlightColor3);

    ADD_GIZMO_ELEM(gizmoRot, "Gizmo Disc", vec3(0), quat(radians(vec3(0., 0., 0.))), VulpineColorUI::HightlightColor1);
    ADD_GIZMO_ELEM(gizmoRot, "Gizmo Disc", vec3(0), quat(radians(vec3(0., -90., 0.))), VulpineColorUI::HightlightColor2);
    ADD_GIZMO_ELEM(gizmoRot, "Gizmo Disc", vec3(0), quat(radians(vec3(0., 0., 90.))), VulpineColorUI::HightlightColor3);

    ADD_GIZMO_ELEM(gizmoScale, "Gizmo Arrow", vec3(0), quat(radians(vec3(0., 0., 0.))), VulpineColorUI::HightlightColor4);
    ADD_GIZMO_ELEM(gizmoScale, "Gizmo Arrow", vec3(0), quat(radians(vec3(0., -90., 0.))), VulpineColorUI::HightlightColor5);
    ADD_GIZMO_ELEM(gizmoScale, "Gizmo Arrow", vec3(0), quat(radians(vec3(0., 0., 90.))), VulpineColorUI::HightlightColor6);

    ADD_GIZMO_ELEM(gizmoScale, "Gizmo Plane", vec3(0, 0.45, 0.45), quat(radians(vec3(0., 0., 0.))), VulpineColorUI::HightlightColor4);
    ADD_GIZMO_ELEM(gizmoScale, "Gizmo Plane", vec3(0.45, 0.45, 0), quat(radians(vec3(0., -90., 0.))), VulpineColorUI::HightlightColor5);
    ADD_GIZMO_ELEM(gizmoScale, "Gizmo Plane", vec3(0.45, 0, 0.45), quat(radians(vec3(0., 0., 90.))), VulpineColorUI::HightlightColor6);

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

    Loader<ScriptInstance>::get("Gizmo Update").run(cursor);
    float d = distance(globals.currentCamera->getPosition(), gizmo->comp<State3D>().position)*0.25;
    gizmoPos->comp<State3D>().scale = vec3(d);
    gizmoRot->comp<State3D>().scale = vec3(d);
    gizmoScale->comp<State3D>().scale = vec3(d);
    // system("clear");
    ComponentModularity::synchronizeChildren(gizmo); // Update all child of the gizmo after it has been moved


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

    gizmo = gizmoPos = gizmoRot = gizmoScale = EntityRef();
    appRoot = EntityRef();
    App::setController(nullptr);

    GG::sun->shadowCameraSize = vec2(0, 0);
}

