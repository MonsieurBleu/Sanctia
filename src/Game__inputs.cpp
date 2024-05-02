#include <Game.hpp>
#include <AssetManager.hpp>
#include <EntityBlueprint.hpp>

bool Game::userInput(GLFWKeyInfo input)
{
    if (baseInput(input))
        return true;

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
                setController(&spectator);
            }
            else
            if(globals.getController() == &spectator)
                {
                    setController(&playerControl);
                    playerControl.body->position = globals.currentCamera->getPosition();
                    GG::playerEntity->comp<EntityModel>()->state.hide = SHOW;
                }
            break;

        case GLFW_KEY_E :
            if(globals.getController() == &playerControl)
                setController(&dialogueControl);
            else 
            if(globals.getController() == &dialogueControl)
                setController(&playerControl);
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

        case GLFW_KEY_F :
            GG::entities.clear();
            break;

        case GLFW_KEY_P :
            Blueprint::TestManequin();
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

        case GLFW_MOUSE_BUTTON_RIGHT : 
        {
            // Effect testEffectZone;
            // testEffectZone.zone.setCapsule(0.25, vec3(-1, 1.5, 1), vec3(1, 1.5, 1));
            // testEffectZone.type = EffectType::Damage;
            // testEffectZone.valtype = DamageType::Pure;
            // testEffectZone.value = 20;
            // testEffectZone.maxTrigger = 1;
            // GG::playerEntity->set<Effect>(testEffectZone);
            
            // animTime = 0.f;
            GG::playerEntity->comp<EntityActionState>().isTryingToAttack = true;
        }
            break;

        case GLFW_KEY_LEFT :
            GG::playerEntity->comp<EntityActionState>().stance = EntityActionState::Stance::LEFT;
            break;

        case GLFW_KEY_RIGHT :
            GG::playerEntity->comp<EntityActionState>().stance = EntityActionState::Stance::RIGHT;
            break;

        case GLFW_KEY_UP :
            GG::playerEntity->comp<EntityActionState>().stance = EntityActionState::Stance::SPECIAL;
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

