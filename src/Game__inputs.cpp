#include <Game.hpp>
#include <AssetManager.hpp>
#include <EntityBlueprint.hpp>
#include <Helpers.hpp>

bool Game::userInput(GLFWKeyInfo input)
{
    if (baseInput(input))
        return true;

    if(globals.mouseRightClickDown())
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
        case GLFW_KEY_ESCAPE:
            state = quit;
            break;

        case GLFW_KEY_F12 :
            if(globals.getController() == &playerControl)
            {
                GG::playerEntity->comp<EntityModel>()->state.hide = HIDE;

                for(auto &i : GG::playerEntity->comp<Items>().equipped)
                    if(i.item.get() && i.item->hasComp<EntityModel>())
                        i.item->comp<EntityModel>()->state.hide = HIDE;
                setController(&spectator);
            }
            else
            if(globals.getController() == &spectator)
            {
                setController(&playerControl);
                // playerControl.body->position = globals.currentCamera->getPosition();

                if(GG::playerEntity->hasComp<RigidBody>())
                {
                    auto body = GG::playerEntity->comp<RigidBody>();
                    if(body)
                    {
                        body->setIsActive(true);
                        body->setTransform(rp3d::Transform(PG::torp3d(camera.getPosition() + vec3(0, 5, 0)), rp3d::Quaternion::identity()));
                    }
                }

                GG::playerEntity->comp<EntityModel>()->state.hide = SHOW;
                for(auto &i : GG::playerEntity->comp<Items>().equipped)
                    if(i.item.get() && i.item->hasComp<EntityModel>())
                        i.item->comp<EntityModel>()->state.hide = SHOW;
            }
            break;
        
        case GLFW_KEY_F11 :
            globals.simulationTime.toggle();
            break;

        case GLFW_KEY_E :
            if(globals.getController() == &playerControl)
                setController(&dialogueControl);
            else 
            if(globals.getController() == &dialogueControl)
                setController(&playerControl);
            break;
        
        case GLFW_KEY_F :
            GG::playerEntity->comp<ActionState>().isTryingToBlock = true;
            GG::playerEntity->comp<ActionState>().setStance(ActionState::SPECIAL);
            break;

        case GLFW_KEY_H :
            hideHUD = !hideHUD;
            break;

        case GLFW_MOUSE_BUTTON_MIDDLE :
            GG::playerEntity->comp<ActionState>().isTryingToAttack = true;
            GG::playerEntity->comp<ActionState>().setStance(ActionState::SPECIAL);
            break;

        case GLFW_KEY_F2:
            globals.currentCamera->toggleMouseFollow();
            break;

        case GLFW_KEY_1:
            Bloom.toggle();
            break;

        case GLFW_KEY_2:
            SSAO.toggle();
            break;
        
        case GLFW_KEY_F5:
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
            for(auto &m : Loader<MeshMaterial>::loadedAssets)
                m.second->reset();
            break;

        case GLFW_KEY_F8 :
            {
                auto myfile = std::fstream("saves/cameraState.bin", std::ios::out | std::ios::binary);
                myfile.write((char*)&camera.getState(), sizeof(CameraState));
                myfile.close();
            }
                break;

        case GLFW_KEY_T :
            GG::entities.clear();
            break;

        case GLFW_KEY_P :
        {   auto e = Blueprint::TestManequin();
            e->set<DeplacementBehaviour>(FOLLOW_WANTED_DIR);
            e->set<AgentState>({AgentState::COMBAT_POSITIONING});
            e->set<Target>(Target{GG::playerEntity});
        }


            break;
        
        case GLFW_KEY_M :
            if(GG::entities.size()) GG::entities.pop_back();
            break;
        
        case GLFW_KEY_9 : 
            GlobalComponentToggler<InfosStatsHelpers>::activated = !GlobalComponentToggler<InfosStatsHelpers>::activated;
            break;

        case GLFW_KEY_0 : 
            GlobalComponentToggler<PhysicsHelpers>::activated = !GlobalComponentToggler<PhysicsHelpers>::activated;
            break;

        case GLFW_MOUSE_BUTTON_LEFT : 
        {
            // Effect testEffectZone;
            // testEffectZone.zone.setCapsule(0.25, vec3(-1, 1.5, 1), vec3(1, 1.5, 1));
            // testEffectZone.type = EffectType::Damage;
            // testEffectZone.valtype = DamageType::Pure;
            // testEffectZone.value = 20;
            // testEffectZone.maxTrigger = 1;
            // GG::playerEntity->set<Effect>(testEffectZone);
            
            // animTime = 0.f;
            GG::playerEntity->comp<ActionState>().isTryingToAttack = true;
        }
            break;

        case GLFW_KEY_LEFT :
            GG::playerEntity->comp<ActionState>().setStance(ActionState::Stance::LEFT);
            break;

        case GLFW_KEY_RIGHT :
            GG::playerEntity->comp<ActionState>().setStance(ActionState::Stance::RIGHT);
            break;

        case GLFW_KEY_UP :
            GG::playerEntity->comp<ActionState>().setStance(ActionState::Stance::SPECIAL);
            break;

        case GLFW_KEY_BACKSPACE :
            GG::playerEntity->comp<EntityStats>().alive = false;
            break;

        case GLFW_KEY_RIGHT_SHIFT :
            GG::playerEntity->comp<ActionState>().stun = true;
            break;
        
        case GLFW_KEY_N :
            physicsMutex.lock();
            for(int i = 0; i < 20; i++)
            {
                float cubeSize = 1;

                rp3d::RigidBody *body = PG::world->createRigidBody(rp3d::Transform(rp3d::Vector3(0 + (i%10)*0.75*cubeSize, 15 + i*2.1*cubeSize, 0), rp3d::Quaternion::identity()));

                auto e = newEntity("physictest", body, EntityState3D());
                GG::entities.push_back(e);

                Blueprint::Assembly::AddEntityBodies(body, e.get(), {
                    {PG::common.createCapsuleShape(cubeSize*0.5, cubeSize*0.75), rp3d::Transform::identity()}
                }, {});

                e->comp<EntityState3D>().usequat = true;
            }
            physicsMutex.unlock();
            
            break;

        default:
            break;
        }
    }

    if (input.action == GLFW_RELEASE)
    {
        switch (input.key)
        {
        case GLFW_MOUSE_BUTTON_RIGHT : 
            // GG::playerEntity->removeComp<Effect>();
            break;

        default:
            break;
        }
    }

    return true;
};

