#include <AssetManager.hpp>
#include <EntityBlueprint.hpp>
#include <Game.hpp>
#include <Helpers.hpp>

bool Game::userInput(GLFWKeyInfo input)
{
    return baseInput(input);
};

void Game::initInput()
{
    Inputs::toggleHud = InputManager::addEventInput(
        "toggle hud", GLFW_KEY_H, 0, GLFW_PRESS, [&]() { hideHUD = !hideHUD; },
        InputManager::Filters::always, false);
        
    Inputs::quitGame = InputManager::addEventInput(
        "quit game", GLFW_KEY_ESCAPE, 0, GLFW_PRESS, [&]() { state = quit; },
        InputManager::Filters::always, false);

    Inputs::toggleFreeCam = InputManager::addEventInput(
        "toggle free cam", GLFW_KEY_F12, 0, GLFW_PRESS, [&]() { 
            if (globals.getController() == &playerControl && GG::playerEntity)
            {
                GG::playerEntity->comp<EntityModel>()->state.hide = ModelStatus::HIDE;

                for (auto &i : GG::playerEntity->comp<Items>().equipped)
                    if (i.item.get() && i.item->hasComp<EntityModel>())
                        i.item->comp<EntityModel>()->state.hide = ModelStatus::HIDE;
                setController(&spectator);
            }
            else if (globals.getController() == &spectator && GG::playerEntity)
            {
                setController(&playerControl);
                // playerControl.body->position = globals.currentCamera->getPosition();

                if (GG::playerEntity->hasComp<RigidBody>())
                {
                    auto body = GG::playerEntity->comp<RigidBody>();
                    if (body)
                    {
                        body->setIsActive(true);
                        body->setTransform(rp3d::Transform(PG::torp3d(camera.getPosition() + vec3(0, 5, 0)),
                                                           rp3d::Quaternion::identity()));
                    }
                }

                GG::playerEntity->comp<EntityModel>()->state.hide = ModelStatus::SHOW;
                for (auto &i : GG::playerEntity->comp<Items>().equipped)
                    if (i.item.get() && i.item->hasComp<EntityModel>())
                        i.item->comp<EntityModel>()->state.hide = ModelStatus::SHOW;
            }
        });

    Inputs::toggleSimTime = InputManager::addEventInput(
        "toggle sim time", GLFW_KEY_F11, 0, GLFW_PRESS, [&]() { globals.simulationTime.toggle(); },
        InputManager::Filters::always, false);

    Inputs::toggleWireframe = InputManager::addEventInput(
        "toggle wireframe", GLFW_KEY_F1, 0, GLFW_PRESS, [&]() { wireframe = !wireframe; },
        InputManager::Filters::always, false);


    Inputs::interact = InputManager::addEventInput(
        "interact", GLFW_KEY_E, 0, GLFW_PRESS, [&]() {
            if (globals.getController() == &playerControl)
                setController(&dialogueControl);
            else if (globals.getController() == &dialogueControl)
                setController(&playerControl);
        },
        InputManager::Filters::always, false);

    Inputs::toggleFreeMouse = InputManager::addEventInput(
        "toggle free mouse", GLFW_KEY_F2, 0, GLFW_PRESS, [&]() { globals.currentCamera->toggleMouseFollow(); },
        InputManager::Filters::always, false);

    Inputs::toggleEditorMode = InputManager::addEventInput(
        "toggle editor mode", GLFW_KEY_F3, 0, GLFW_PRESS, [&]() {
            editorModeEnable = editorModeEnable ? false : true;

            if (editorModeEnable)
                gameScreenWidget->comp<WidgetBox>().set(vec2(-1. / 3., +1), vec2(-0.6 - 1. / 3., +0.4));
            else
                gameScreenWidget->comp<WidgetBox>().set(vec2(-1, +1), vec2(-1, +1));
        },
        InputManager::Filters::always, false);

    Inputs::toggleBloom = InputManager::addEventInput(
        "toggle bloom", GLFW_KEY_1, 0, GLFW_PRESS, [&]() { Bloom.toggle(); },
        InputManager::Filters::always, false);

    Inputs::toggleSSAO = InputManager::addEventInput(
        "toggle ssao", GLFW_KEY_2, 0, GLFW_PRESS, [&]() { SSAO.toggle(); },
        InputManager::Filters::always, false);

    Inputs::reset = InputManager::addEventInput(
        "reset", GLFW_KEY_F5, 0, GLFW_PRESS, [&]() {
            #ifdef _WIN32
            system("cls");
#else
            system("clear");
#endif

            finalProcessingStage.reset();
            Bloom.getShader().reset();
            SSAO.getShader().reset();
            depthOnlyMaterial->reset();
            GG::PBR->reset();
            GG::PBRstencil->reset();
            skyboxMaterial->reset();
            for (auto &m : Loader<MeshMaterial>::loadedAssets)
                m.second->reset();
        },
        InputManager::Filters::always, false);

    Inputs::saveCamState = InputManager::addEventInput(
        "save cam state", GLFW_KEY_F8, 0, GLFW_PRESS, [&]() {
            auto myfile = std::fstream("saves/cameraState.bin", std::ios::out | std::ios::binary);
            myfile.write((char *)&camera.getState(), sizeof(CameraState));
            myfile.close();
        },
        InputManager::Filters::always, false);

    Inputs::clearEntities = InputManager::addEventInput(
        "clear entities", GLFW_KEY_T, 0, GLFW_PRESS, [&]() { GG::entities.clear(); },
        InputManager::Filters::always, false);

    Inputs::clearOneEntity = InputManager::addEventInput(
        "clear one entity", GLFW_KEY_M, 0, GLFW_PRESS, [&]() {
            if (GG::entities.size())
                GG::entities.pop_back();
        },
        InputManager::Filters::always, false);

    Inputs::toggleInfoStatHelper = InputManager::addEventInput(
        "toggle info stat helper", GLFW_KEY_9, 0, GLFW_PRESS, [&]() {
            GlobalComponentToggler<InfosStatsHelpers>::activated =
                !GlobalComponentToggler<InfosStatsHelpers>::activated;
        },
        InputManager::Filters::always, false);

    Inputs::togglePhysicsHelper = InputManager::addEventInput(
        "toggle physics helper", GLFW_KEY_0, 0, GLFW_PRESS, [&]() {
            GlobalComponentToggler<PhysicsHelpers>::activated = !GlobalComponentToggler<PhysicsHelpers>::activated;
        },
        InputManager::Filters::always, false);

    Inputs::spawnDebugPhysicsTest = InputManager::addEventInput(
        "spawn debug physics test", GLFW_KEY_N, 0, GLFW_PRESS, [&]() {
            physicsMutex.lock();
            for (int i = 0; i < 20; i++)
            {
                float cubeSize = 1;

                RigidBody body = PG::world->createRigidBody(
                    rp3d::Transform(rp3d::Vector3(0 + (i % 10) * 0.75 * cubeSize, 15 + i * 2.1 * cubeSize, 0),
                                    rp3d::Quaternion::identity()));

                auto e = newEntity("physictest", body, EntityState3D());
                GG::entities.push_back(e);

                Blueprint::Assembly::AddEntityBodies(body, e.get(),
                                                     {{
                                                         PG::common.createCapsuleShape(cubeSize * 0.5, cubeSize * 0.75),
                                                         rp3d::Transform::identity(),
                                                     }},
                                                     {});

                e->comp<EntityState3D>().usequat = true;

                e->set<PhysicsHelpers>(PhysicsHelpers());
            }
            physicsMutex.unlock();
        },
        InputManager::Filters::always, false);

    Inputs::togglePhysicsInterpolation = InputManager::addEventInput(
        "toggle physics interpolation", GLFW_KEY_KP_1, 0, GLFW_PRESS, [&]() {
            PG::doPhysicInterpolation = !PG::doPhysicInterpolation;
        },
        InputManager::Filters::always, false);

    Inputs::toggleAutoShaderRefresh = InputManager::addEventInput(
        "toggle auto shader refresh", GLFW_KEY_F6, 0, GLFW_PRESS, [&]() {
            doAutomaticShaderRefresh = !doAutomaticShaderRefresh;
        },
        InputManager::Filters::always, false);

    Inputs::addWidget = InputManager::addEventInput(
        "add widget", GLFW_KEY_KP_ADD, 0, GLFW_PRESS, [&]() {
            ComponentModularity::addChild(
                *EDITOR::MENUS::GlobalInfos,
                newEntity("Info Stat Helper", EDITOR::MENUS::AppMenu->comp<WidgetUI_Context>(), WidgetState(),
                          WidgetBox(vec2(0), vec2(0)), WidgetBackground(), WidgetSprite("VulpineIcon"), WidgetStyle(),
                          WidgetButton(WidgetButton::Type::CHECKBOX, WidgetButton::InteractFunc([](float v) {
                                           GlobalComponentToggler<InfosStatsHelpers>::activated =
                                               !GlobalComponentToggler<InfosStatsHelpers>::activated;
                                       }),
                                       WidgetButton::UpdateFunc([]() {
                                           return GlobalComponentToggler<InfosStatsHelpers>::activated ? 0.f : 1.f;
                                       }))));
        },
        InputManager::Filters::always, false);

    Inputs::removeWidget = InputManager::addEventInput(
        "remove widget", GLFW_KEY_KP_SUBTRACT, 0, GLFW_PRESS, [&]() {
            if (EDITOR::MENUS::GlobalInfos->comp<EntityGroupInfo>().children.size())
                EDITOR::MENUS::GlobalInfos->comp<EntityGroupInfo>().children.pop_back();

            ManageGarbage<Items>();
            ManageGarbage<EntityModel>();
            ManageGarbage<PhysicsHelpers>();
            ManageGarbage<WidgetBackground>();
            ManageGarbage<WidgetSprite>();
            ManageGarbage<WidgetText>();
        },
        InputManager::Filters::always, false);
}