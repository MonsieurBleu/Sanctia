#include <Subapps.hpp>
#include <EntityBlueprint.hpp>
#include <AssetManager.hpp>
#include <MathsUtils.hpp>

#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtx/string_cast.hpp>

#include <Inputs.hpp>

#include <App.hpp>

Apps::SceneMergeApp::SceneMergeApp() : SubApps("Scene Merge Test")
{
    inputs.push_back(&
        InputManager::addEventInput(
            "merge scene", GLFW_KEY_SEMICOLON, 0, GLFW_PRESS, [&]() {
                
                std::cout <<"MERGING OMG\n";
                physicsMutex.lock();

                ComponentModularity::mergeChildren(*appRoot);

                ManageGarbage<Items>();
                ManageGarbage<WidgetBackground>();
                ManageGarbage<WidgetSprite>();
                ManageGarbage<WidgetText>();
                ManageGarbage<EntityModel>();
                ManageGarbage<PhysicsHelpers>();

                physicsMutex.unlock();
            },
            InputManager::Filters::always, false)
    );    

    for(auto &i : inputs)
        i->activated = false;
};

EntityRef Apps::SceneMergeApp::UImenu()
{
    return newEntity("SCENE MERGE APP MENU"
        , UI_BASE_COMP
        , WidgetBox()
        , WidgetStyle()
            .settextColor1(EDITOR::MENUS::COLOR::HightlightColor1)
        , WidgetBackground()
    );
}

void Apps::SceneMergeApp::init()
{
    /***** Preparing App Settings *****/
    {
        appRoot = newEntity("AppRoot");
        App::setController(&orbitController);

        globals.currentCamera->setPosition(normalize(vec3(-0.5, 0.5, 0)));
        globals.currentCamera->getState().FOV = radians(90.f);
        // orbitController.distance = 150;
        orbitController.distance = 5;
    }

    /***** Creatign Terrain *****/
    // ComponentModularity::addChild(*appRoot,
    //     Blueprint::Terrain("ressources/maps/testPlayground.hdr",
    //                     // "ressources/maps/RuggedTerrain.hdr",
    //                     // "ressources/maps/generated_512x512.hdr",
    //                     // "ressources/maps/RT512.hdr",
    //                     // vec3(512, 64, 512),
    //                     vec3(256, 64, 256), vec3(0), 128)
    // );

    /***** Creating Scene To Stress Test *****/
    int size = 5;

    for(int i = 0; i < size; i++)
    for(int j = 0; j < size; j++)
    {
        ModelRef tableMesh = Loader<MeshModel3D>::get("SceneTest_Table").copy();
        EntityModel model(EntityModel{newObjectGroup()}); 
        model->add(tableMesh);

        // model->state.setPosition(
        //     vec3(i, j)
        // );

        auto table = newEntity("table ;_;"
            , model
            , EntityState3D(true)
        );

        // table->comp<EntityState3D>().position = vec3(4.0 * (i-size/2), 0, 4 * (j-size/2));

        rp3d::RigidBody *body = PG::world->createRigidBody(
            rp3d::Transform(
                PG::torp3d(
                    vec3(4.0 * (i-size/2), 
                    -10, 
                    4 * (j-size/2))
                ),
                DEFQUAT)
        );

        Blueprint::Assembly::AddEntityBodies(body, table.get(),
        {
            {
                PG::common.createBoxShape(rp3d::Vector3(1.5, 0.55, 0.75)),
                rp3d::Transform(rp3d::Vector3(0, 0.55, 0), DEFQUAT)
            }
        },
        {});

        body->setType(rp3d::BodyType::STATIC);

        table->set<RigidBody>(body);

        /* Adding zweihander */
        {
            auto z = Blueprint::Zweihander();

            z->comp<RigidBody>()->setTransform(rp3d::Transform(
                PG::torp3d(
                    // table->comp<EntityState3D>().position + 
                    vec3(0, 2, 0)
                ),
                DEFQUAT));

            z->comp<RigidBody>()->setType(rp3d::BodyType::DYNAMIC);
            ComponentModularity::addChild(*table, z);
        }

        ComponentModularity::addChild(*appRoot, table);
    }

    // ComponentModularity::mergeChildren(*appRoot);

    globals.simulationTime.resume();
}

void Apps::SceneMergeApp::update()
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
}


void Apps::SceneMergeApp::clean()
{
    globals.simulationTime.pause();

    globals.currentCamera->setMouseFollow(false);
    globals.currentCamera->setPosition(vec3(0));
    globals.currentCamera->setDirection(vec3(-1, 0, 0));
    globals.currentCamera->getState().FOV = radians(90.f);
    appRoot = EntityRef();
    App::setController(nullptr);
}

