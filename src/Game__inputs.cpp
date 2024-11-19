#include <AssetManager.hpp>
#include <EntityBlueprint.hpp>
#include <Game.hpp>
#include <Helpers.hpp>

bool Game::userInput(GLFWKeyInfo input)
{
    if (baseInput(input))
        return true;

    if (globals.isTextInputsActive())
        return true;

    if (globals.mouseRightClickDown())
    {
        GG::playerEntity->comp<ActionState>().isTryingToBlock = true;
    }
    else
    {
        GG::playerEntity->comp<ActionState>().isTryingToBlock = false;
    }

    if (input.action == GLFW_PRESS)
    {
        switch (input.key)
        {
        // case GLFW_KEY_ESCAPE:
        //     state = quit;
        //     break;

        // case GLFW_KEY_F12:
        //     if (globals.getController() == &playerControl)
        //     {
        //         GG::playerEntity->comp<EntityModel>()->state.hide = ModelStatus::HIDE;

        //         for (auto &i : GG::playerEntity->comp<Items>().equipped)
        //             if (i.item.get() && i.item->hasComp<EntityModel>())
        //                 i.item->comp<EntityModel>()->state.hide = ModelStatus::HIDE;
        //         setController(&spectator);
        //     }
        //     else if (globals.getController() == &spectator)
        //     {
        //         setController(&playerControl);
        //         // playerControl.body->position = globals.currentCamera->getPosition();

        //         if (GG::playerEntity->hasComp<RigidBody>())
        //         {
        //             auto body = GG::playerEntity->comp<RigidBody>();
        //             if (body)
        //             {
        //                 body->setIsActive(true);
        //                 body->setTransform(rp3d::Transform(PG::torp3d(camera.getPosition() + vec3(0, 5, 0)),
        //                                                    rp3d::Quaternion::identity()));
        //             }
        //         }

        //         GG::playerEntity->comp<EntityModel>()->state.hide = ModelStatus::SHOW;
        //         for (auto &i : GG::playerEntity->comp<Items>().equipped)
        //             if (i.item.get() && i.item->hasComp<EntityModel>())
        //                 i.item->comp<EntityModel>()->state.hide = ModelStatus::SHOW;
        //     }
        //     break;

        // case GLFW_KEY_F11:
        //     globals.simulationTime.toggle();
        //     break;

        // case GLFW_KEY_F1:
        //     wireframe = !wireframe;
        //     break;

        // case GLFW_KEY_E:
        //     if (globals.getController() == &playerControl)
        //         setController(&dialogueControl);
        //     else if (globals.getController() == &dialogueControl)
        //         setController(&playerControl);
        //     break;

        // case GLFW_KEY_F:
        //     GG::playerEntity->comp<ActionState>().isTryingToBlock = true;
        //     GG::playerEntity->comp<ActionState>().setStance(ActionState::SPECIAL);
        //     break;

        // case GLFW_KEY_H:
        //     hideHUD = !hideHUD;
        //     break;

        // case GLFW_MOUSE_BUTTON_MIDDLE:
        //     GG::playerEntity->comp<ActionState>().isTryingToAttack = true;
        //     GG::playerEntity->comp<ActionState>().setStance(ActionState::SPECIAL);
        //     break;

//         case GLFW_KEY_F2:
//             globals.currentCamera->toggleMouseFollow();
//             break;

//         case GLFW_KEY_F3: {
//             editorModeEnable = editorModeEnable ? false : true;

//             if (editorModeEnable)
//                 gameScreenWidget->comp<WidgetBox>().set(vec2(-1. / 3., +1), vec2(-0.6 - 1. / 3., +0.4));
//             else
//                 gameScreenWidget->comp<WidgetBox>().set(vec2(-1, +1), vec2(-1, +1));
//         }
//         break;

//         case GLFW_KEY_1:
//             Bloom.toggle();
//             break;

//         case GLFW_KEY_2:
//             SSAO.toggle();
//             break;

//         case GLFW_KEY_F5:
// #ifdef _WIN32
//             system("cls");
// #else
//             system("clear");
// #endif

//             finalProcessingStage.reset();
//             Bloom.getShader().reset();
//             SSAO.getShader().reset();
//             depthOnlyMaterial->reset();
//             GG::PBR->reset();
//             GG::PBRstencil->reset();
//             skyboxMaterial->reset();
//             for (auto &m : Loader<MeshMaterial>::loadedAssets)
//                 m.second->reset();
//             break;

//         case GLFW_KEY_F8: {
//             auto myfile = std::fstream("saves/cameraState.bin", std::ios::out | std::ios::binary);
//             myfile.write((char *)&camera.getState(), sizeof(CameraState));
//             myfile.close();
//         }
//         break;

        // case GLFW_KEY_T:
        //     GG::entities.clear();
        //     break;

        // case GLFW_KEY_P: {
        //     auto e = Blueprint::TestManequin();
        //     e->set<DeplacementBehaviour>(FOLLOW_WANTED_DIR);
        //     e->set<AgentState>({AgentState::COMBAT_POSITIONING});
        //     e->set<Target>(Target{GG::playerEntity});
        // }

        // break;

        // case GLFW_KEY_M:
        //     if (GG::entities.size())
        //         GG::entities.pop_back();
        //     break;

        // case GLFW_KEY_9:
        //     GlobalComponentToggler<InfosStatsHelpers>::activated =
        //         !GlobalComponentToggler<InfosStatsHelpers>::activated;
        //     break;

        // case GLFW_KEY_0:
        //     GlobalComponentToggler<PhysicsHelpers>::activated = !GlobalComponentToggler<PhysicsHelpers>::activated;
        //     break;

        // case GLFW_MOUSE_BUTTON_LEFT: {
        //     // Effect testEffectZone;
        //     // testEffectZone.zone.setCapsule(0.25, vec3(-1, 1.5, 1), vec3(1, 1.5, 1));
        //     // testEffectZone.type = EffectType::Damage;
        //     // testEffectZone.valtype = DamageType::Pure;
        //     // testEffectZone.value = 20;
        //     // testEffectZone.maxTrigger = 1;
        //     // GG::playerEntity->set<Effect>(testEffectZone);

        //     // animTime = 0.f;
        //     if (globals.currentCamera->getMouseFollow())
        //         GG::playerEntity->comp<ActionState>().isTryingToAttack = true;
        // }
        // break;

        // case GLFW_KEY_LEFT:
        //     if (globals.currentCamera->getMouseFollow())
        //         GG::playerEntity->comp<ActionState>().setStance(ActionState::Stance::LEFT);
        //     break;

        // case GLFW_KEY_RIGHT:
        //     if (globals.currentCamera->getMouseFollow())
        //         GG::playerEntity->comp<ActionState>().setStance(ActionState::Stance::RIGHT);
        //     break;

        // case GLFW_KEY_UP:
        //     if (globals.currentCamera->getMouseFollow())
        //         GG::playerEntity->comp<ActionState>().setStance(ActionState::Stance::SPECIAL);
        //     break;

        // case GLFW_KEY_BACKSPACE:
        //     if (globals.currentCamera->getMouseFollow())
        //         GG::playerEntity->comp<EntityStats>().alive = false;
        //     break;

        // case GLFW_KEY_RIGHT_SHIFT:
        //     if (globals.currentCamera->getMouseFollow())
        //         GG::playerEntity->comp<ActionState>().stun = true;
        //     break;

        // case GLFW_KEY_N:
        //     physicsMutex.lock();
        //     for (int i = 0; i < 20; i++)
        //     {
        //         float cubeSize = 1;

        //         RigidBody body = PG::world->createRigidBody(
        //             rp3d::Transform(rp3d::Vector3(0 + (i % 10) * 0.75 * cubeSize, 15 + i * 2.1 * cubeSize, 0),
        //                             rp3d::Quaternion::identity()));

        //         auto e = newEntity("physictest", body, EntityState3D());
        //         GG::entities.push_back(e);

        //         Blueprint::Assembly::AddEntityBodies(body, e.get(),
        //                                              {{
        //                                                  PG::common.createCapsuleShape(cubeSize * 0.5, cubeSize * 0.75),
        //                                                  rp3d::Transform::identity(),
        //                                              }},
        //                                              {});

        //         e->comp<EntityState3D>().usequat = true;

        //         e->set<PhysicsHelpers>(PhysicsHelpers());
        //     }
        //     physicsMutex.unlock();

        //     break;

        // case GLFW_KEY_KP_1:
        //     PG::doPhysicInterpolation = !PG::doPhysicInterpolation;
        //     break;

        // case GLFW_KEY_F6:
        //     doAutomaticShaderRefresh = !doAutomaticShaderRefresh;
        //     break;

        // case GLFW_KEY_KP_ADD:
        //     ComponentModularity::addChild(
        //         *EDITOR::MENUS::GlobalInfos,
        //         newEntity("Info Stat Helper", EDITOR::MENUS::AppMenu->comp<WidgetUI_Context>(), WidgetState(),
        //                   WidgetBox(vec2(0), vec2(0)), WidgetBackground(), WidgetSprite("VulpineIcon"), WidgetStyle(),
        //                   WidgetButton(WidgetButton::Type::CHECKBOX, WidgetButton::InteractFunc([](float v) {
        //                                    GlobalComponentToggler<InfosStatsHelpers>::activated =
        //                                        !GlobalComponentToggler<InfosStatsHelpers>::activated;
        //                                }),
        //                                WidgetButton::UpdateFunc([]() {
        //                                    return GlobalComponentToggler<InfosStatsHelpers>::activated ? 0.f : 1.f;
        //                                }))));

        //     break;

        // case GLFW_KEY_KP_SUBTRACT:
        //     if (EDITOR::MENUS::GlobalInfos->comp<EntityGroupInfo>().children.size())
        //         EDITOR::MENUS::GlobalInfos->comp<EntityGroupInfo>().children.pop_back();

        //     ManageGarbage<EntityModel>();
        //     ManageGarbage<PhysicsHelpers>();
        //     ManageGarbage<WidgetBackground>();
        //     ManageGarbage<WidgetSprite>();
        //     ManageGarbage<WidgetText>();
        //     break;

        default:
            break;
        }
    }

    if (input.action == GLFW_RELEASE)
    {
        switch (input.key)
        {
        case GLFW_MOUSE_BUTTON_RIGHT:
            // GG::playerEntity->removeComp<Effect>();
            break;

        default:
            break;
        }
    }

    return true;
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
            if (globals.getController() == &playerControl)
            {
                GG::playerEntity->comp<EntityModel>()->state.hide = ModelStatus::HIDE;

                for (auto &i : GG::playerEntity->comp<Items>().equipped)
                    if (i.item.get() && i.item->hasComp<EntityModel>())
                        i.item->comp<EntityModel>()->state.hide = ModelStatus::HIDE;
                setController(&spectator);
            }
            else if (globals.getController() == &spectator)
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

    Inputs::kickKey = InputManager::addEventInput(
        "kick key", GLFW_KEY_F, 0, GLFW_PRESS, [&]() {
            GG::playerEntity->comp<ActionState>().isTryingToBlock = true;
            GG::playerEntity->comp<ActionState>().setStance(ActionState::SPECIAL);
        },
        InputManager::Filters::always, false);

    Inputs::kickClick = InputManager::addEventInput(
        "kick click", GLFW_MOUSE_BUTTON_MIDDLE, 0, GLFW_PRESS, [&]() {
            GG::playerEntity->comp<ActionState>().isTryingToAttack = true;
            GG::playerEntity->comp<ActionState>().setStance(ActionState::SPECIAL);
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

    Inputs::startCombat = InputManager::addEventInput(
        "start combat", GLFW_KEY_P, 0, GLFW_PRESS, [&]() {
            auto e = Blueprint::TestManequin();
            e->set<DeplacementBehaviour>(FOLLOW_WANTED_DIR);
            e->set<AgentState>({AgentState::COMBAT_POSITIONING});
            e->set<Target>(Target{GG::playerEntity});
        },
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

    Inputs::attack = InputManager::addEventInput(
        "attack", GLFW_MOUSE_BUTTON_LEFT, 0, GLFW_PRESS, [&]() {
            // Effect testEffectZone;
            // testEffectZone.zone.setCapsule(0.25, vec3(-1, 1.5, 1), vec3(1, 1.5, 1));
            // testEffectZone.type = EffectType::Damage;
            // testEffectZone.valtype = DamageType::Pure;
            // testEffectZone.value = 20;
            // testEffectZone.maxTrigger = 1;
            // GG::playerEntity->set<Effect>(testEffectZone);

            // animTime = 0.f;
            if (globals.currentCamera->getMouseFollow())
                GG::playerEntity->comp<ActionState>().isTryingToAttack = true;
        },
        InputManager::Filters::always, false);
    
    Inputs::leftStance = InputManager::addEventInput(
        "left stance", GLFW_KEY_LEFT, 0, GLFW_PRESS, [&]() {
            GG::playerEntity->comp<ActionState>().setStance(ActionState::Stance::LEFT);
        },
        []() {return globals.currentCamera->getMouseFollow();}, false);

    Inputs::rightStance = InputManager::addEventInput(
        "right stance", GLFW_KEY_RIGHT, 0, GLFW_PRESS, [&]() {
            GG::playerEntity->comp<ActionState>().setStance(ActionState::Stance::RIGHT);
        },
        []() {return globals.currentCamera->getMouseFollow();}, false);

    Inputs::specialStance = InputManager::addEventInput(
        "special stance", GLFW_KEY_UP, 0, GLFW_PRESS, [&]() {
            GG::playerEntity->comp<ActionState>().setStance(ActionState::Stance::SPECIAL);
        },
        []() {return globals.currentCamera->getMouseFollow();}, false);
    Inputs::die = InputManager::addEventInput(
        "die", GLFW_KEY_BACKSPACE, 0, GLFW_PRESS, [&]() {
            GG::playerEntity->comp<EntityStats>().alive = false;
        },
        []() {return globals.currentCamera->getMouseFollow();}, false);

    Inputs::stun = InputManager::addEventInput(
        "stun", GLFW_KEY_RIGHT_SHIFT, 0, GLFW_PRESS, [&]() {
            GG::playerEntity->comp<ActionState>().stun = true;
        },
        []() {return globals.currentCamera->getMouseFollow();}, false);

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